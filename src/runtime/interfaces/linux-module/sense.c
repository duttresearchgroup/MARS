#include "../linux-module/sense.h"

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/sched/rt.h>
#include <linux/math64.h>
#include <trace/events/sched.h>

#include "../common/sense_defs.h"
#include "../linux-module/helpers.h"
#include "../linux-module/pal/pal.h"
#include "../linux-module/sense_data.h"
#include "../linux-module/sensing_window.h"
#include "../linux-module/setup.h"
#include "../linux-module/user_if.h"


//per cpu flush kthreads to ensure tasks are sensed at the end of the epoch
DEFINE_PER_CPU(struct task_struct*,vitamins_flush_tasks);
DEFINE_PER_CPU(struct semaphore*,vitamins_flush_tasks_run);
DEFINE_PER_CPU(bool,vitamins_flush_tasks_done);
static volatile bool vitamins_flush_tasks_stopping;


static inline void reset_perf_counters(perf_data_perf_counters_t *sen_data){
	int cnt;
	for(cnt = 0; cnt < MAX_PERFCNTS; ++cnt) sen_data->perfcnts[cnt] = 0;
	sen_data->nivcsw= 0;
	sen_data->nvcsw = 0;
	sen_data->time_busy_ms = 0;
	sen_data->time_total_ms = 0;
	smp_mb();
}

static inline void reset_task_counters(int cpu,perf_data_task_t *sen_data){
	int cnt;
	reset_perf_counters(&(sen_data->perfcnt));
	for(cnt = 0; cnt < MAX_BEAT_DOMAINS; ++cnt) sen_data->beats[cnt] = 0;
	sen_data->last_cpu_used = cpu;
}

static inline void reset_cpu_counters(perf_data_cpu_t *sen_data){
	int cnt;
	for(cnt = 0; cnt < MAX_BEAT_DOMAINS; ++cnt) sen_data->beats[cnt] = 0;
	reset_perf_counters(&(sen_data->perfcnt));
}

static inline void reset_freq_counters(perf_data_freq_domain_t *sen_data){
	sen_data->avg_freq_mhz_acc = 0;
	sen_data->time_ms_acc = 0;
	sen_data->last_update_time_ms = jiffies_to_msecs(jiffies);
	smp_mb();
}


static DEFINE_SPINLOCK(new_task_created_lock);
static int tasks_being_created = 0;
static void new_task_created(int at_cpu, struct task_struct *tsk,struct task_struct *parent_tsk){
    //allocates the hook data
    tracked_task_data_t *hooks;
    private_hook_data_t *priv_hooks;
    private_hook_data_t *parent;
    sys_info_t *sys;
    int i;
    int task_idx;

    if((unsigned)vitsdata->created_tasks_cnt >= MAX_CREATED_TASKS){
    	pinfo("Task buffer full ! New task ignored\n");
    	return;
    }

    spin_lock(&new_task_created_lock);
		task_idx = vitsdata->__created_tasks_cnt_tmp;
		vitsdata->__created_tasks_cnt_tmp += 1;
		tasks_being_created += 1;
    spin_unlock(&new_task_created_lock);

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

    //if we want to pin this task
    if(at_cpu >= 0){
    	BUG_ON(at_cpu >= sys->core_list_size);
    	cpus_clear(tsk->cpus_allowed);
    	cpu_set(at_cpu, tsk->cpus_allowed);
    }

    priv_hooks->sen_data_lock = __SPIN_LOCK_UNLOCKED(priv_hooks->sen_data_lock);

    for(i=0;i<sensing_window_cnt;++i){
    	reset_task_counters(at_cpu,&(vitsdata->sensing_windows[i]._acc.tasks[task_idx]));
    	reset_task_counters(at_cpu,&(vitsdata->sensing_windows[i].curr.tasks[task_idx]));
    	reset_task_counters(at_cpu,&(vitsdata->sensing_windows[i].aggr.tasks[task_idx]));
    }
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


    spin_lock(&new_task_created_lock);
		tasks_being_created -= 1;
    	if(tasks_being_created==0)
    		vitsdata->created_tasks_cnt = vitsdata->__created_tasks_cnt_tmp;
    spin_unlock(&new_task_created_lock);
}

private_hook_data_t* add_created_task(struct task_struct *tsk)
{
	//supports tasks already added
	private_hook_data_t *task = hook_hashmap_get(tsk);
	if(task != nullptr) return task;
	else{
		new_task_created(-1,tsk,tsk->parent);
		return hook_hashmap_get(tsk);
	}
}

static void flush_tasks_on_cpu(int cpu, int tgt_cpu, bool mustwait){
    unsigned long int aux;
    if(cpu != tgt_cpu){ //the main vitamins work kthread has already flushed this one

        per_cpu(vitamins_flush_tasks_done,tgt_cpu) = false; smp_mb();

        up(per_cpu(vitamins_flush_tasks_run,tgt_cpu));

        aux = jiffies;
        while((per_cpu(vitamins_flush_tasks_done,tgt_cpu) == false) && mustwait) {
            smp_mb();
            if((jiffies - aux) >= 1 ){
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

//cpu counters are modified by it's own cpu when a task leaves a cpu
//and read/reset by cpu0 at the end of the epoch
//this spin lock protects them
static spinlock_t vitamins_cpu_counters_acc_lock[NR_CPUS];

static void sense_cpus(sys_info_t *sys, int wid)
{
	int i,cnt;
	uint64_t time_total_ms,curr_time_ms;

	curr_time_ms = jiffies_to_msecs(jiffies);
    time_total_ms = counter_diff_32(curr_time_ms, vitsdata->sensing_windows[wid].curr_sample_time_ms);

    vitsdata->sensing_windows[wid].prev_sample_time_ms = vitsdata->sensing_windows[wid].curr_sample_time_ms;
    vitsdata->sensing_windows[wid].curr_sample_time_ms = curr_time_ms;

	for_each_online_cpu(i){
	    perf_data_cpu_t *last_total = &(vitsdata->sensing_windows[wid].aggr.cpus[i]);
	    perf_data_cpu_t *curr_epoch = &(vitsdata->sensing_windows[wid].curr.cpus[i]);
	    perf_data_cpu_t *data_cnt = &(vitsdata->sensing_windows[wid]._acc.cpus[i]);

	    spin_lock(&(vitamins_cpu_counters_acc_lock[i]));

		for(cnt = 0; cnt < MAX_PERFCNTS; ++cnt)
			curr_epoch->perfcnt.perfcnts[cnt] = data_cnt->perfcnt.perfcnts[cnt] -  last_total->perfcnt.perfcnts[cnt];

		curr_epoch->perfcnt.nvcsw = data_cnt->perfcnt.nvcsw -  last_total->perfcnt.nvcsw;
		curr_epoch->perfcnt.nivcsw = data_cnt->perfcnt.nivcsw -  last_total->perfcnt.nivcsw;
		curr_epoch->perfcnt.time_busy_ms = data_cnt->perfcnt.time_busy_ms -  last_total->perfcnt.time_busy_ms;
		curr_epoch->perfcnt.time_total_ms = time_total_ms;

		for(cnt = 0; cnt < MAX_BEAT_DOMAINS; ++cnt)
			curr_epoch->beats[cnt] = data_cnt->beats[cnt] -  last_total->beats[cnt];

		//this is not updated at the sensing hooks
		data_cnt->perfcnt.time_total_ms += curr_epoch->perfcnt.time_total_ms;

		*last_total = *data_cnt;

		spin_unlock(&(vitamins_cpu_counters_acc_lock[i]));
	}

    //freq sense
    for(i = 0; i < sys->freq_domain_list_size; ++i){

    	perf_data_freq_domain_t *last_total = &(vitsdata->sensing_windows[wid].aggr.freq_domains[i]);
    	perf_data_freq_domain_t *curr_epoch = &(vitsdata->sensing_windows[wid].curr.freq_domains[i]);
    	perf_data_freq_domain_t *data_cnt = &(vitsdata->sensing_windows[wid]._acc.freq_domains[i]);

    	curr_epoch->avg_freq_mhz_acc = data_cnt->avg_freq_mhz_acc - last_total->avg_freq_mhz_acc;
    	curr_epoch->time_ms_acc = data_cnt->time_ms_acc -  last_total->time_ms_acc;

    	*last_total = *data_cnt;
    }
}

/////////////////////////////
// perf conter maping funcs

static void vit_reset_mapped_perfcnt(void)
{
	int i;
	plat_reset_perfcnts();
	for(i = 0; i < MAX_PERFCNTS;++i) vitsdata->idx_to_perfcnt_map[i] = -1;
	for(i = 0; i < SIZE_PERFCNT;++i) vitsdata->perfcnt_to_idx_map[i] = -1;
	vitsdata->perfcnt_mapped_cnt = 0;
}
static bool vit_map_perfcnt(perfcnt_t perfcnt)
{
	BUG_ON(vitsdata->perfcnt_mapped_cnt > MAX_PERFCNTS);
	if(vitsdata->perfcnt_mapped_cnt >= MAX_PERFCNTS){
		pinfo("Cannot use more then %d pmmu counters!",MAX_PERFCNTS);
		return false;
	}
	plat_enable_perfcnt(perfcnt);
	vitsdata->idx_to_perfcnt_map[vitsdata->perfcnt_mapped_cnt] = perfcnt;
	vitsdata->perfcnt_to_idx_map[perfcnt] = vitsdata->perfcnt_mapped_cnt;
	vitsdata->perfcnt_mapped_cnt += 1;
	return true;
}

///////////////////////////////

static inline void sense_tasks(sys_info_t *sys,int wid)
{
	int p;

	uint64_t time_total_ms = counter_diff_32(jiffies_to_msecs(jiffies), vitsdata->sensing_windows[wid].prev_sample_time_ms);

	for(p = 0; p < vitsdata->created_tasks_cnt; ++p){
		int cnt;
		private_hook_data_t *task_priv_hook = &(priv_hook_created_tasks[p]);
		perf_data_task_t *last_total = &(vitsdata->sensing_windows[wid].aggr.tasks[p]);
		perf_data_task_t *curr_epoch = &(vitsdata->sensing_windows[wid].curr.tasks[p]);
		perf_data_task_t *data_cnt = &(vitsdata->sensing_windows[wid]._acc.tasks[p]);

		spin_lock(&(task_priv_hook->sen_data_lock));
			for(cnt = 0; cnt < MAX_PERFCNTS; ++cnt)
				curr_epoch->perfcnt.perfcnts[cnt] = data_cnt->perfcnt.perfcnts[cnt] -  last_total->perfcnt.perfcnts[cnt];

			curr_epoch->perfcnt.nvcsw = data_cnt->perfcnt.nvcsw -  last_total->perfcnt.nvcsw;
			curr_epoch->perfcnt.nivcsw = data_cnt->perfcnt.nivcsw -  last_total->perfcnt.nivcsw;
			curr_epoch->perfcnt.time_busy_ms = data_cnt->perfcnt.time_busy_ms -  last_total->perfcnt.time_busy_ms;
			curr_epoch->perfcnt.time_total_ms = time_total_ms;

			for(cnt = 0; cnt < MAX_BEAT_DOMAINS; ++cnt)
				curr_epoch->beats[cnt] = data_cnt->beats[cnt] -  last_total->beats[cnt];

			//this is not updated at the sensing hooks
			data_cnt->perfcnt.time_total_ms += curr_epoch->perfcnt.time_total_ms;

			*last_total = *data_cnt;

			curr_epoch->last_cpu_used = data_cnt->last_cpu_used;
			last_total->last_cpu_used = data_cnt->last_cpu_used;

		spin_unlock(&(task_priv_hook->sen_data_lock));
	}
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
    int i,j;

    uint64_t curr_time = jiffies_to_msecs(jiffies);
    //sense freq
	for(i = 0; i < sys->freq_domain_list_size; ++i){
    	//gets the frequency using one of the cores in this domain's core list
    	//TODO this is ugly
    	int curr_freq_MHz = kern_cpu_get_freq_mhz(sys->freq_domain_list[i].__vitaminslist_head_cores->position);

    	for(j=0;j<sensing_window_cnt;++j){
    		perf_data_freq_domain_t *data_cnt = &(vitsdata->sensing_windows[j]._acc.freq_domains[i]);
    		uint64_t time_elapsed = curr_time - data_cnt->last_update_time_ms;
    		data_cnt->last_update_time_ms = curr_time;
    		data_cnt->time_ms_acc += time_elapsed;
    		data_cnt->avg_freq_mhz_acc += time_elapsed * curr_freq_MHz;
    	}
    }

    sensing_window_tasks_flushed = false;
    vitsdata->num_of_minimum_periods += 1;
}

void sense_window(sys_info_t *sys, int wid)
{
	int cpu;

	if(!sensing_window_tasks_flushed){
		for_each_online_cpu(cpu){
			flush_tasks_on_cpu(smp_processor_id(),cpu,true);
		}
		sensing_window_tasks_flushed = true;
	}

	sense_cpus(sys,wid);

	sense_tasks(sys,wid);

	vitsdata->sensing_windows[wid].num_of_samples += 1;
}

static inline int is_userspace(struct task_struct *tsk){
	//has parent and parent pid is > 1 and name does not begin with kthread or vitamins
	return tsk->parent &&
		  (tsk->parent->pid > 1) &&
		  !((tsk->comm[0] == 'k') && (tsk->comm[6] == 'd')) &&
		  !((tsk->comm[0] == 'v') && (tsk->comm[7] == 's'));
}

static bool per_task_sensing = false;
static int pin_task_to_cpu = -1;

void set_per_task_sensing(bool val){
	per_task_sensing = val;
}
void set_pin_task_to_cpu(int cpu){
	pin_task_to_cpu = cpu;
}

static inline void vitamins_task_created_probe(struct task_struct *parent, struct task_struct *tsk)
{
	//the task will be sensed if:
	//  -we are not in measuring mode
	//  -its a user task (forked from init) && its cpu mask is not set (currently not used)
	if(per_task_sensing && is_userspace(tsk)){
		//does not pin if pin_task_to_cpu==-1
		new_task_created(pin_task_to_cpu,tsk,parent);
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
    data->perfcnt.time_busy_ms = jiffies_to_msecs(jiffies);

    //beats data
    p = hook_hashmap_get(tsk);
    if (p && (p->beats != nullptr))
    	for(i = 0; i < MAX_BEAT_DOMAINS; ++i) data->beats[i] = p->beats->beat_data[i].__curr_beat_cnt;

	smp_mb();
}

static inline void vitamins_sensing_end_probe(int cpu, struct task_struct *tsk)
{
	private_hook_data_t *p;
	tracked_task_data_t *_p;
	uint64_t perfcnts[MAX_PERFCNTS];
	uint64_t time_busy_ms;
	uint64_t beat_cnt[MAX_BEAT_DOMAINS];
	int i,wid;

	//true if the task is leaving the cpu voluntarly
	bool vcsw = tsk->state && !(preempt_count() & PREEMPT_ACTIVE);

    perf_data_cpu_t *data_begin = &(cpu_counters_begin[cpu]);

	//if its the first time sensing we skip this call until we get another call to begin (which sets vitamins_first_sense = false)
    if(first_sense[cpu]) return;

	//do not execute this for the idle task
    if(tsk->pid == 0) return;

    for(i = 0; i < vitsdata->perfcnt_mapped_cnt; ++i)
    	perfcnts[i] = counter_diff_32(read_perfcnt(cpu,vitsdata->idx_to_perfcnt_map[i]), data_begin->perfcnt.perfcnts[i]);
    time_busy_ms = counter_diff_32(jiffies_to_msecs(jiffies), data_begin->perfcnt.time_busy_ms);

    //beats data
    p = hook_hashmap_get(tsk);
    if (p && (p->beats != nullptr))
    	for(i = 0; i < MAX_BEAT_DOMAINS; ++i) beat_cnt[i] = p->beats->beat_data[i].__curr_beat_cnt - data_begin->beats[i];
    else
    	for(i = 0; i < MAX_BEAT_DOMAINS; ++i) beat_cnt[i] = 0;


    stop_perf_sense(cpu);

    spin_lock(&(vitamins_cpu_counters_acc_lock[cpu]));
    for(wid=0;wid<sensing_window_cnt;++wid){
    	perf_data_cpu_t *data_cnt = &(vitsdata->sensing_windows[wid]._acc.cpus[cpu]);
    	for(i = 0; i < vitsdata->perfcnt_mapped_cnt; ++i) data_cnt->perfcnt.perfcnts[i] += perfcnts[i];
    	data_cnt->perfcnt.time_busy_ms += time_busy_ms;
    	if(vcsw) data_cnt->perfcnt.nvcsw += 1;
    	else	 data_cnt->perfcnt.nivcsw += 1;
    	for(i = 0; i < MAX_BEAT_DOMAINS; ++i) data_cnt->beats[i] += beat_cnt[i];
    }
    spin_unlock(&(vitamins_cpu_counters_acc_lock[cpu]));

    vitsdata->num_of_csw_periods[cpu] += 1;

    if (p) {
    	spin_lock(&(p->sen_data_lock));
    	_p = p->hook_data;
    	for(wid=0;wid<sensing_window_cnt;++wid){
    		vitsdata->sensing_windows[wid]._acc.tasks[_p->task_idx].last_cpu_used = cpu;
    		for(i = 0; i < vitsdata->perfcnt_mapped_cnt; ++i)
    			vitsdata->sensing_windows[wid]._acc.tasks[_p->task_idx].perfcnt.perfcnts[i] += perfcnts[i];
    		vitsdata->sensing_windows[wid]._acc.tasks[_p->task_idx].perfcnt.time_busy_ms += time_busy_ms;
    		if(vcsw) vitsdata->sensing_windows[wid]._acc.tasks[_p->task_idx].perfcnt.nvcsw += 1;
    		else	 vitsdata->sensing_windows[wid]._acc.tasks[_p->task_idx].perfcnt.nivcsw += 1;
    		for(i = 0; i < MAX_BEAT_DOMAINS; ++i) vitsdata->sensing_windows[wid]._acc.tasks[_p->task_idx].beats[i] += beat_cnt[i];
    	}
    	spin_unlock(&(p->sen_data_lock));
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
    	vitamins_cpu_counters_acc_lock[i] = __SPIN_LOCK_UNLOCKED(vitamins_cpu_counters_acc_lock[i]);
    }

    res = trace_perf_counter_reset();
    BUG_ON(res==false);
}

static void vitamins_sense_cleanup_counters(sys_info_t *sys)
{
    int i,wid;

    for(wid=0;wid<sensing_window_cnt;++wid){
    	vitsdata->sensing_windows[wid].curr_sample_time_ms = jiffies_to_msecs(jiffies);
    	vitsdata->sensing_windows[wid].prev_sample_time_ms = vitsdata->sensing_windows[wid].curr_sample_time_ms;
    }
    for_each_online_cpu(i){
    	reset_cpu_counters(&(cpu_counters_begin[i]));
    	first_sense[i] = true;
    	vitsdata->num_of_csw_periods[i] = 0;
    	for(wid=0;wid<sensing_window_cnt;++wid){
    		reset_cpu_counters(&(vitsdata->sensing_windows[wid]._acc.cpus[i]));
    		reset_cpu_counters(&(vitsdata->sensing_windows[wid].curr.cpus[i]));
    		reset_cpu_counters(&(vitsdata->sensing_windows[wid].aggr.cpus[i]));
    	}
    }

    for(wid=0;wid<sensing_window_cnt;++wid){
    	vitsdata->sensing_windows[wid].num_of_samples = 0;
    	vitsdata->sensing_windows[wid].wid = wid;
    }

    vitsdata->num_of_minimum_periods = 0;

    for(wid=0;wid<sensing_window_cnt;++wid){
    	for(i = 0; i < sys->freq_domain_list_size; ++i){
    		reset_freq_counters(&(vitsdata->sensing_windows[wid]._acc.freq_domains[i]));
    		reset_freq_counters(&(vitsdata->sensing_windows[wid].curr.freq_domains[i]));
    		reset_freq_counters(&(vitsdata->sensing_windows[wid].aggr.freq_domains[i]));
    	}
    }

    smp_mb();
}

bool trace_perf_counter(perfcnt_t pc){
	if(pc >= SIZE_PERFCNT) return false;
	else {
		return vit_map_perfcnt(pc);
	}
}

bool trace_perf_counter_reset(void){
	if(sensing_running()){
		pinfo("Trying to reset pmmu counters while tracing is running!\n");
		return false;
	}
	else {
		vit_reset_mapped_perfcnt();
		return true;
	}
}

void sense_begin(sys_info_t *sys)
{
    vitamins_sense_cleanup_counters(sys);

    vitsdata->starttime_ms = jiffies_to_msecs(jiffies);
    vitsdata->stoptime_ms = vitsdata->starttime_ms;

    //carefull that vit_plat_enabled_perfcnts may show fixed enabled counter event before any is made available (e.g. PERFCNT_BUSY_CY on armv7)
    BUG_ON(vitsdata->perfcnt_mapped_cnt != plat_enabled_perfcnts());

    register_trace_sched_switch(vitamins_context_switch_probe,0);
    register_trace_sched_process_fork(vitamins_sched_process_fork_probe,0);
}

void sense_stop(sys_info_t *sys)
{
    vitsdata->stoptime_ms = jiffies_to_msecs(jiffies);

    unregister_trace_sched_process_fork(vitamins_sched_process_fork_probe,0);
    unregister_trace_sched_switch(vitamins_context_switch_probe,0);
	tracepoint_synchronize_unregister();
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

