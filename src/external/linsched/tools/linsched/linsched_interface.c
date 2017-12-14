
#include "linsched.h"
#include <linsched_interface.h>
#include <malloc.h>
#include <assert.h>

static struct linsched_topology topo;
static int currNumCPUS = 0;
static int currNumTasks = 0;
struct task_struct* tasks[LINSCHED_MAX_TASKS];
struct task_data* tasks_data[LINSCHED_MAX_TASKS];

static int currMAX_CPUS = 0;

void linsched_setup(int maxCPUS) {
    int i;

    //one extra "buffer" cpu is used to remove noise
    currMAX_CPUS = maxCPUS+1;
    assert(currMAX_CPUS <= NR_CPUS);

    topo.nr_cpus = currMAX_CPUS;
    for (i = 0; i < currMAX_CPUS; ++i) {
        topo.core_map[i] = i;
        topo.coregroup_map[i] = i;
        topo.node_map[i] = 0;
    }
    topo.node_distances[0][0] = 10;

    for(i = 0; i < LINSCHED_MAX_TASKS; ++i){
        tasks[i] = NULL;
        tasks_data[i] = NULL;
    }

    linsched_init(&topo);
}

static void reset_task(struct task_struct* task, struct task_data* data, unsigned int busy, unsigned int sleep, int nice, int cpu)
{
    struct cpumask affinity;
    struct sleep_run_task *d = data->data;
    d->busy = busy;
    d->sleep = sleep;
    task->se.sum_exec_runtime = 0;
    task->sched_info.run_delay = 0;
    task->sched_info.pcount = 0;
    task->nvcsw = 0;
    task->nivcsw = 0;
    set_user_nice(task,nice);

    cpumask_clear(&affinity);
    if(cpu == -1) cpumask_set_cpu(currMAX_CPUS-1, &affinity);
    else          cpumask_set_cpu(cpu, &affinity);
    set_cpus_allowed(task, affinity);


}

void linsched_reset(int numcpus) {
    int i;
    assert(numcpus < currMAX_CPUS);

    currNumCPUS = numcpus;

    for(i = 0; i < currNumTasks; ++i){
        reset_task(tasks[i],tasks_data[i],0,0,20,-1);
    }

    currNumTasks = 0;

    //flush things
    linsched_run_sim(10000);
}

#define TICKS_PER_SEC 1000

int linsched_create_task(double run, double sleep, int core, int niceval){

    struct task_data* data;
    struct task_struct* task;
    int _sleep = (int)(sleep*1000);
    int _busy = (int)(run*1000);
    assert(currNumTasks < LINSCHED_MAX_TASKS);
    assert(core < currNumCPUS);


    if(tasks[currNumTasks] == NULL){
        data = linsched_create_sleep_run(_sleep,_busy);//in ms
        task = linsched_create_normal_task(data,niceval);
        tasks[currNumTasks] = task;
        tasks_data[currNumTasks] = data;
    }
    else{
        task =  tasks[currNumTasks];
        data = tasks_data[currNumTasks];

    }
    reset_task(task,data,_busy,_sleep,niceval,core);

    return currNumTasks++;
}

void linsched_sim(double time){
    linsched_run_sim((int)(time*TICKS_PER_SEC));
}

void linsched_task_info(int task, struct task_sched_info *info){
    assert(info->task_id == task);
    info->run_time = tasks[task]->se.sum_exec_runtime / 1000000000.0;
    info->run_delay = tasks[task]->sched_info.run_delay / 1000000000.0;
    info->pcount = tasks[task]->sched_info.pcount;
    info->nvcsw = tasks[task]->nvcsw;
    info->nivcsw = tasks[task]->nivcsw;
}

void linsched_print_info(void){
    linsched_print_task_stats();
    linsched_show_schedstat();
}
