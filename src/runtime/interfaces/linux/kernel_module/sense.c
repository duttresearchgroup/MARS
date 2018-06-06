/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * Copyright (C) 2018 Bryan Donyanavard <bdonyana@uci.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/sched/rt.h>
#include <linux/math64.h>
#include <trace/events/sched.h>

#include "sense.h"
#include "pal.h"
#include "helpers.h"
#include "sense_data.h"
#include "sensing_window.h"
#include "setup.h"
#include "user_if.h"

//per cpu flush kthreads to ensure tasks are sensed at the end of the epoch
DEFINE_PER_CPU(struct task_struct*,vitamins_flush_tasks);
DEFINE_PER_CPU(struct semaphore*,vitamins_flush_tasks_run);
DEFINE_PER_CPU(bool,vitamins_flush_tasks_done);
static volatile bool vitamins_flush_tasks_stopping;


static DEFINE_SPINLOCK(new_task_created_lock);
static int tasks_being_created = 0;
static void new_task_created(struct task_struct *tsk,struct task_struct *parent_tsk){
    //allocates the hook data
    tracked_task_data_t *hooks;
    private_hook_data_t *priv_hooks;
    private_hook_data_t *parent;
    sys_info_t *sys;
    int task_idx;
    unsigned long flags;

    if((unsigned)vitsdata->created_tasks_cnt >= MAX_CREATED_TASKS){
    	pinfo("Task buffer full ! New task ignored\n");
    	return;
    }

    spin_lock_irqsave(&new_task_created_lock,flags);
		task_idx = vitsdata->__created_tasks_cnt_tmp;
		vitsdata->__created_tasks_cnt_tmp += 1;
		tasks_being_created += 1;
    spin_unlock_irqrestore(&new_task_created_lock,flags);

    hooks = &(vitsdata->created_tasks[task_idx]);
    priv_hooks = &(priv_hook_created_tasks[task_idx]);
    sys = system_info();

    clear_object(priv_hooks,hashmap);

    priv_hooks->hook_data = hooks;
    priv_hooks->this_task = tsk;
    priv_hooks->beats = nullptr;
    hooks->this_task_pid = tsk->pid;
    hooks->task_idx = task_idx;
    BUG_ON(TASK_NAME_SIZE > TASK_COMM_LEN);
    memcpy(hooks->this_task_name,tsk->comm,TASK_NAME_SIZE);


    priv_hooks->sen_data_lock[0] = __SPIN_LOCK_UNLOCKED(priv_hooks->sen_data_lock[0]);
    priv_hooks->sen_data_lock[1] = __SPIN_LOCK_UNLOCKED(priv_hooks->sen_data_lock[1]);

    perf_data_reset_task(vitsdata,task_idx);

    hooks->num_beat_domains = 0;
    //check if parent has beat domain
    parent = hook_hashmap_get(parent_tsk);
    if (parent && (parent->hook_data != nullptr))
    	hooks->parent_has_beats = (parent->hook_data->num_beat_domains > 0) || parent->hook_data->parent_has_beats;
    else
    	hooks->parent_has_beats = false;

    hooks->task_finished = false;

    hooks->tsk_model = nullptr;//initialized on demand at user level

    hook_hashmap_add(tsk,priv_hooks);
    BUG_ON(hook_hashmap_get(tsk) != priv_hooks);


    spin_lock_irqsave(&new_task_created_lock,flags);
		tasks_being_created -= 1;
    	if(tasks_being_created==0)
    		vitsdata->created_tasks_cnt = vitsdata->__created_tasks_cnt_tmp;
    spin_unlock_irqrestore(&new_task_created_lock,flags);
}

private_hook_data_t* add_created_task(struct task_struct *tsk)
{
	//supports tasks already added
	private_hook_data_t *task = hook_hashmap_get(tsk);
	if(task != nullptr) return task;
	else{
		new_task_created(tsk,tsk->parent);
		return hook_hashmap_get(tsk);
	}
}

static void flush_tasks_on_cpu(int cpu, int tgt_cpu, bool mustwait){
    unsigned long int aux;
    if(cpu != tgt_cpu){ //the main vitamins work kthread has already flushed this one

        per_cpu(vitamins_flush_tasks_done,tgt_cpu) = false; smp_mb();

        up(per_cpu(vitamins_flush_tasks_run,tgt_cpu));

        aux = ktime_to_ns(ktime_get());
        while((per_cpu(vitamins_flush_tasks_done,tgt_cpu) == false) && mustwait) {
            smp_mb();
            if((ktime_to_ns(ktime_get()) - aux) >= 1 ){
                break;
            }
        }
    }
}

static void __vitamins_flush_tasks_kthread(int cpu){
    //this is just a dummy high priority thread that
    //flushes out tasks from the cpu and make sure the
    //counters are up-to date before sensing
    per_cpu(vitamins_flush_tasks_done,cpu) = true; smp_mb();
}

static int vitamins_flush_tasks_kthread(void *data){
    int cpu = (long)data;

    while (!kthread_should_stop()) {
        if(down_interruptible(per_cpu(vitamins_flush_tasks_run,cpu))) break;
        if(vitamins_flush_tasks_stopping) break;
        __vitamins_flush_tasks_kthread(cpu);
    }

    return 0;

}

static int vitamins_flush_tasks_kthread_create(int tgt_cpu){
    void *cpu_ptr = (void *)(long)tgt_cpu;
    struct sched_param sparam = {.sched_priority = MAX_RT_PRIO };
    struct task_struct *vthread;

    vthread = kthread_create(vitamins_flush_tasks_kthread, cpu_ptr, "vitamins_kthread_flush_task/%d", tgt_cpu);
    if (IS_ERR(vthread))
        return -1;

    per_cpu(vitamins_flush_tasks_run,tgt_cpu) = (struct semaphore*) kmalloc(sizeof(struct semaphore), GFP_ATOMIC);
    if(!per_cpu(vitamins_flush_tasks_run,tgt_cpu))
        return -1;
    sema_init(per_cpu(vitamins_flush_tasks_run,tgt_cpu),0);

    kthread_bind(vthread, tgt_cpu);

    /* Must be high prio: stop_machine expects to yield to it. */
    sched_setscheduler(vthread, SCHED_FIFO, &sparam);

    per_cpu(vitamins_flush_tasks,tgt_cpu) = vthread;

    wake_up_process(vthread);

    return 0;

}

//Cpu counters are modified by it's own cpu when a task leaves a cpu
//and read/reset by cpu0 at the end of the epoch.
//This spin lock protects them. Since those counters are double-buffered we
//have one lock for each buffer
static spinlock_t vitamins_cpu_counters_acc_lock[NR_CPUS][2];

static inline void perf_data_commit_cpu_window_acc_lock(int cpu, int acc_idx, unsigned long *flags)
{ spin_lock_irqsave(&(vitamins_cpu_counters_acc_lock[cpu][acc_idx]), (*flags)); }

static inline void perf_data_commit_cpu_window_acc_unlock(int cpu, int acc_idx, unsigned long *flags)
{ spin_unlock_irqrestore(&(vitamins_cpu_counters_acc_lock[cpu][acc_idx]), (*flags)); }


static void sense_cpus(sys_info_t *sys, int wid)
{
	perf_data_commit_cpu_window(sys,
	        vitsdata,wid,ktime_to_ms(ktime_get()),
	        &perf_data_commit_cpu_window_acc_lock,
	        &perf_data_commit_cpu_window_acc_unlock);
}


static inline void perf_data_commit_task_window_acc_lock(int task, int acc_idx, unsigned long *flags)
{
    private_hook_data_t *task_priv_hook = &(priv_hook_created_tasks[task]);
    spin_lock_irqsave(&(task_priv_hook->sen_data_lock[acc_idx]), (*flags));
}

static inline void perf_data_commit_task_window_acc_unlock(int task, int acc_idx, unsigned long *flags)
{
    private_hook_data_t *task_priv_hook = &(priv_hook_created_tasks[task]);
    spin_unlock_irqrestore(&(task_priv_hook->sen_data_lock[acc_idx]), (*flags));
}

static inline void sense_tasks(sys_info_t *sys,int wid)
{
    perf_data_commit_tasks_window(sys,
            vitsdata,wid,ktime_to_ms(ktime_get()),
            &perf_data_commit_task_window_acc_lock,
            &perf_data_commit_task_window_acc_unlock);
}

bool update_task_beat_info(pid_t pid)
{
	int i;
	private_hook_data_t* task = hook_hashmap_get_pid(pid);

	if(task == nullptr){
		pinfo("update_task_beat_info(pid=%d) couldn't find task!!!\n",pid);
		return false;
	}
	if(task->beats == nullptr){
		pinfo("update_task_beat_info(pid=%d) task has not beat info!!!\n",pid);
		return false;
	}
	for(i = 0; i < task->beats->num_beat_domains; ++i){
		task->hook_data->beats[i].tgt_rate = task->beats->beat_data[i].tgt_rate;
		task->hook_data->beats[i].type = task->beats->beat_data[i].type;
	}
	smp_mb();
	task->hook_data->num_beat_domains = task->beats->num_beat_domains;

	return true;
}

static bool sensing_window_tasks_flushed = false;

//used to check the current frequency (could also be used to set a frequency)
void minimum_sensing_window(sys_info_t *sys)
{
    int i;

    uint64_t curr_time = ktime_to_ms(ktime_get());
    //sense freq
	for(i = 0; i < sys->freq_domain_list_size; ++i){
    	//gets the frequency using one of the cores in this domain's core list
    	//TODO this is ugly
    	int curr_freq_MHz = kern_cpu_get_freq_mhz(sys->freq_domain_list[i].__vitaminslist_head_cores->position);

    	perf_data_freq_domain_t *data_cnt = &(vitsdata->__acc_freq_domains[i]);
    	uint64_t time_elapsed = curr_time - data_cnt->last_update_time_ms;
    	data_cnt->last_update_time_ms = curr_time;
    	data_cnt->time_ms_acc += time_elapsed;
    	data_cnt->avg_freq_mhz_acc += time_elapsed * curr_freq_MHz;
    }

    sensing_window_tasks_flushed = false;
    vitsdata->num_of_minimum_periods += 1;
}

static int sensing_window_reading_errs[MAX_WINDOW_CNT];

void sense_window(sys_info_t *sys, int wid)
{
	int cpu;

	if(vitsdata->sensing_windows[wid].___reading){
	    if(++sensing_window_reading_errs[wid] < 5)
	        pinfo("WARNING: updating window %d, but daemon might still be reading it.!!!\n",wid);
	}

	vitsdata->sensing_windows[wid].___updating = true;

	if(!sensing_window_tasks_flushed){
		for_each_online_cpu(cpu){
			flush_tasks_on_cpu(smp_processor_id(),cpu,true);
		}
		sensing_window_tasks_flushed = true;
	}

	sense_cpus(sys,wid);

	sense_tasks(sys,wid);

	vitsdata->sensing_windows[wid].num_of_samples += 1;

	vitsdata->sensing_windows[wid].___updating = false;
}

static inline int is_userspace(struct task_struct *tsk){
    //tsk->mm is null for kernel-level processes
    return tsk->mm != 0;
}

static bool per_task_sensing = false;

void set_per_task_sensing(bool val){
	per_task_sensing = val;
}

static inline void vitamins_task_created_probe(struct task_struct *parent, struct task_struct *tsk)
{
	//the task will be sensed if:
	//  -per task sensing is enabled
	//  -it's a user-level task
	if(per_task_sensing && is_userspace(tsk)){
		//does not pin if pin_task_to_cpu==-1
		new_task_created(tsk,parent);
	}
}


// stores the counters when a task enters the cpu
static perf_data_cpu_t cpu_counters_begin[NR_CPUS];//update every context switch
static volatile bool first_sense[NR_CPUS];


static inline void vitamins_sensing_begin_probe(int cpu, struct task_struct *tsk)
{
	private_hook_data_t *p;
	perf_data_cpu_t *data = &(cpu_counters_begin[cpu]);
	int i;

	//if its the first time sensing we don't need to do anything at begin
	first_sense[cpu] = false;

	//do not execute this for the idle task
	if(tsk->pid == 0) return;

	start_perf_sense(cpu);

    for(i = 0; i < vitsdata->perfcnt_mapped_cnt; ++i) data->perfcnt.perfcnts[i] = read_perfcnt(cpu,vitsdata->idx_to_perfcnt_map[i]);
    data->perfcnt.time_busy_ms = ktime_to_ms(ktime_get());

    //beats data
    p = hook_hashmap_get(tsk);
    if (p && (p->beats != nullptr))
    	for(i = 0; i < MAX_BEAT_DOMAINS; ++i) data->beats[i] = p->beats->beat_data[i].__curr_beat_cnt;

	smp_mb();
}

static inline void vitamins_sensing_end_probe(int cpu, struct task_struct *tsk)
{
	private_hook_data_t *p;
	uint64_t perfcnts[MAX_PERFCNTS];
	uint64_t time_busy_ms;
	uint64_t beat_cnt[MAX_BEAT_DOMAINS];
	int i;
	perf_data_cpu_t *data_cnt;
	unsigned long flags;
	int acc_buffer;

	//true if the task is leaving the cpu voluntarly
	bool vcsw = tsk->state && !(preempt_count() & PREEMPT_ACTIVE);

    perf_data_cpu_t *data_begin = &(cpu_counters_begin[cpu]);

	//if its the first time sensing we skip this call until we get another call to begin (which sets vitamins_first_sense = false)
    if(first_sense[cpu]) return;

	//do not execute this for the idle task
    if(tsk->pid == 0) return;

    for(i = 0; i < vitsdata->perfcnt_mapped_cnt; ++i)
    	perfcnts[i] = counter_diff_32(read_perfcnt(cpu,vitsdata->idx_to_perfcnt_map[i]), data_begin->perfcnt.perfcnts[i]);
    time_busy_ms = counter_diff_32(ktime_to_ms(ktime_get()), data_begin->perfcnt.time_busy_ms);

    //beats data
    p = hook_hashmap_get(tsk);
    if (p && (p->beats != nullptr))
    	for(i = 0; i < MAX_BEAT_DOMAINS; ++i) beat_cnt[i] = p->beats->beat_data[i].__curr_beat_cnt - data_begin->beats[i];
    else
    	for(i = 0; i < MAX_BEAT_DOMAINS; ++i) beat_cnt[i] = 0;


    stop_perf_sense(cpu);


    // Gets the avalailable _acc_cpus[cpu] buffer
    // One of the buffers is always available so this should never really block
    while(1){
        acc_buffer = 0;
        if(spin_trylock_irqsave(&(vitamins_cpu_counters_acc_lock[cpu][0]),flags)) break;
        acc_buffer = 1;
        if(spin_trylock_irqsave(&(vitamins_cpu_counters_acc_lock[cpu][1]),flags)) break;
        acc_buffer = 0; //should definitely get it this time
        if(spin_trylock_irqsave(&(vitamins_cpu_counters_acc_lock[cpu][0]),flags)) break;
        pinfo("c%d: unexpected contention at __acc_cpus!\n",cpu);
    }

    data_cnt = &(vitsdata->__acc_cpus[cpu][acc_buffer]);
    for(i = 0; i < vitsdata->perfcnt_mapped_cnt; ++i) data_cnt->perfcnt.perfcnts[i] += perfcnts[i];
    data_cnt->perfcnt.time_busy_ms += time_busy_ms;
    if(vcsw) data_cnt->perfcnt.nvcsw += 1;
    else	 data_cnt->perfcnt.nivcsw += 1;
    for(i = 0; i < MAX_BEAT_DOMAINS; ++i) data_cnt->beats[i] += beat_cnt[i];

    spin_unlock_irqrestore(&(vitamins_cpu_counters_acc_lock[cpu][acc_buffer]),flags);

    vitsdata->num_of_csw_periods[cpu] += 1;

    if (p) {
        vitsdata->__acc_tasks_last_cpu[p->hook_data->task_idx] = cpu;
        // Gets the avalailable __acc_tasks[cpu] buffer
        // One of the buffers is always available so this should never really block
        while(1){
            acc_buffer = 0;
            if(spin_trylock_irqsave(&(p->sen_data_lock[0]),flags)) break;
            acc_buffer = 1;
            if(spin_trylock_irqsave(&(p->sen_data_lock[1]),flags)) break;
            acc_buffer = 0;//should definitely get it this time
            if(spin_trylock_irqsave(&(p->sen_data_lock[0]),flags)) break;
            pinfo("c%d: unexpected contention at __acc_tasks[%d/%s]!\n",cpu,tsk->pid,tsk->comm);
        }

        data_cnt = &(vitsdata->__acc_tasks[p->hook_data->task_idx][acc_buffer]);
    	for(i = 0; i < vitsdata->perfcnt_mapped_cnt; ++i) data_cnt->perfcnt.perfcnts[i] += perfcnts[i];
    	data_cnt->perfcnt.time_busy_ms += time_busy_ms;
    	if(vcsw) data_cnt->perfcnt.nvcsw += 1;
    	else	 data_cnt->perfcnt.nivcsw += 1;
    	for(i = 0; i < MAX_BEAT_DOMAINS; ++i) data_cnt->beats[i] += beat_cnt[i];

    	spin_unlock_irqrestore(&(p->sen_data_lock[acc_buffer]),flags);
    }

    smp_mb();
}

static void vitamins_context_switch_probe(void *nope, struct task_struct *prev,struct task_struct *next){
	int cpu = task_thread_info(next)->cpu;
	vitamins_sensing_end_probe(cpu,prev);
	vitamins_sensing_begin_probe(cpu,next);
}

static void vitamins_sched_process_fork_probe(void *nope, struct task_struct *parent, struct task_struct *p){
	vitamins_task_created_probe(parent,p);
}

bool sense_create_mechanisms(sys_info_t *sys){
	int i;

	for(i = 0; i < hook_data_hashmap_struct_size; ++i) {
		rwlock_init(&(hook_hashmap_lock[i]));
	}

	if(alloc_sensed_data(sys) == nullptr)
		return false;

	//the flush kthread is done only once by one of the cpus
	vitamins_flush_tasks_stopping = false; smp_mb();
	for_each_online_cpu(i){
		BUG_ON(vitamins_flush_tasks_kthread_create(i) < 0);
	}
	return true;
}

void sense_init(sys_info_t *sys)
{
    int i;
    bool res;
    for(i = 0; i < hook_data_hashmap_struct_size; ++i) {
    	clear_list(hook_hashmap[i]);
    }
    for(i = 0; i < MAX_CREATED_TASKS; ++i) {
    	priv_hook_created_tasks[i].beats = nullptr;
    }

    for_each_online_cpu(i){
    	vitamins_cpu_counters_acc_lock[i][0] = __SPIN_LOCK_UNLOCKED(vitamins_cpu_counters_acc_lock[i][0]);
    	vitamins_cpu_counters_acc_lock[i][1] = __SPIN_LOCK_UNLOCKED(vitamins_cpu_counters_acc_lock[i][1]);
    }

    res = trace_perf_counter_reset();
    BUG_ON(res==false);
}

static void vitamins_sense_cleanup_counters(sys_info_t *sys)
{
    int i;

    for_each_online_cpu(i){
        _perf_data_reset_cpu_counters(&(cpu_counters_begin[i]));
    	first_sense[i] = true;
    }

    perf_data_cleanup_counters(sys,vitsdata,ktime_to_ms(ktime_get()));
}

bool trace_perf_counter(perfcnt_t pc){
	if(pc >= SIZE_PERFCNT)
	    return false;
	else if(perf_data_map_perfcnt(vitsdata,pc)) {
	    plat_enable_perfcnt(pc);
	    return true;
	}
	return false;
}

bool trace_perf_counter_reset(void){
	if(sensing_running()){
		pinfo("Trying to reset pmmu counters while tracing is running!\n");
		return false;
	}
	else {
	    plat_reset_perfcnts();
	    perf_data_reset_mapped_perfcnt(vitsdata);
		return true;
	}
}

void sense_begin(sys_info_t *sys)
{
    int i;

    vitamins_sense_cleanup_counters(sys);

    for(i = 0; i < MAX_WINDOW_CNT; ++i)
        sensing_window_reading_errs[i] = 0;

    vitsdata->starttime_ms = ktime_to_ms(ktime_get());
    vitsdata->stoptime_ms = vitsdata->starttime_ms;

    //carefull that vit_plat_enabled_perfcnts may show fixed enabled counter event before any is made available (e.g. PERFCNT_BUSY_CY on armv7)
    BUG_ON(vitsdata->perfcnt_mapped_cnt != plat_enabled_perfcnts());

    register_trace_sched_switch(vitamins_context_switch_probe,0);
    register_trace_sched_process_fork(vitamins_sched_process_fork_probe,0);
}

void sense_stop(sys_info_t *sys)
{
    int i;

    vitsdata->stoptime_ms = ktime_to_ms(ktime_get());

    unregister_trace_sched_process_fork(vitamins_sched_process_fork_probe,0);
    unregister_trace_sched_switch(vitamins_context_switch_probe,0);
	tracepoint_synchronize_unregister();

	for(i = 0; i < MAX_WINDOW_CNT; ++i)
	    if(sensing_window_reading_errs[i] > 0)
	        pinfo("WARNING: window %d might've been updated %d times with daemon still reading it !!!\n",i, sensing_window_reading_errs[i]);
}

void sense_destroy_mechanisms(sys_info_t *sys){
	int cpu;
    vitamins_flush_tasks_stopping = true; smp_mb();
    for_each_online_cpu(cpu){
        up(per_cpu(vitamins_flush_tasks_run,cpu));
        kthread_stop(per_cpu(vitamins_flush_tasks,cpu));
    }
    for_each_online_cpu(cpu){
        kfree(per_cpu(vitamins_flush_tasks_run,cpu));
    }

    dealloc_sensed_data();
}

void sense_cleanup(sys_info_t *sys)
{
	int i;
	for(i = 0; i < hook_data_hashmap_struct_size; ++i) clear_list(hook_hashmap[i]);

    for(i = 0; i < MAX_CREATED_TASKS; ++i) {
    	if(priv_hook_created_tasks[i].beats != nullptr) dealloc_task_beat_data(&(priv_hook_created_tasks[i]));
    }
}

