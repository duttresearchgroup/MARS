//#define HASDEBUG
#include "core.h"

void
vitamins_vanilla_map(model_sys_t *sys)
{
    /*
     * since we limit the number of tasks per core to 1,
     * having a task mapped to a free core is enough to
     * have the system balanced according to the vanilla
     * Linux approach
     *
     * So this does nothing, except seting the next mapping to curr
     */
    int i;
    vitamins_load_tracker_set(LT_DEFAULT);
    clear_next_map(sys);
    for (i = 0; i < sys->task_list_size; ++i) task_next_core_map(sys->task_list[i],task_curr_core(sys->task_list[i]));
}



/*
 * core_list_head: the head of a list of cores
 *
 * Sums up the proc_time_share_avg of all tasks mapped to the same core in the core's
 * sensed_data.load
 *
 * most_loaded and least_loaded are set based on the value of sensed_data.load
 *
 * returns the sum of the difference between most_loaded->sensed_data.load and sensed_data.load of every other core
 *
 */
static
int64_t
track_load_imbalance(
        model_task_t **task_list, int task_list_size,
        model_core_t *core_list_head, model_core_t **most_loaded,model_core_t **least_loaded, bool agingaware)
{
    model_core_t *core;
    int64_t imbalance;
    int64_t highest_load = 0;
    int64_t lowest_load = CONV_DOUBLE_scaledUINT32(1.0)*task_list_size;

    *most_loaded = nullptr;
    *least_loaded = core_list_head;

    pdebug("track_load_imbalance\n");

    for_each_in_list_head_default(core_list_head,core) {
        pdebug("\ttrack_load_imbalance: checking core %d load=%u task_cnt=%u\n",core->position,core->load_tracking.common.load,core->load_tracking.common.task_cnt);
        if(core->load_tracking.common.load >= highest_load){
            pdebug("\t\tHas highest load\n");
            //the most loaded core can only have 0 tasks and load >0 if we are using aging-ware load tracker
            BUG_ON((core->load_tracking.common.task_cnt == 0) && (core->load_tracking.common.load > 0) && !agingaware);
            if(core->load_tracking.common.task_cnt > 0){
                highest_load = core->load_tracking.common.load;
                *most_loaded = core;
                pdebug("\t\t\tcore set\n");
            }
        }
        if(core->load_tracking.common.load <= lowest_load){
            lowest_load = core->load_tracking.common.load;
            *least_loaded = core;
        }
    }

    if(*most_loaded == nullptr){
        //couldn't find a most loaded core, so all cores in the list have 0 tasks and there is 0 imbalance
        *most_loaded = *least_loaded;
        return 0;
    }

    imbalance = 0;
    for_each_in_list_head_default(core_list_head,core){
        imbalance += highest_load - core->load_tracking.common.load;
    }

    return imbalance;
}

static
model_task_t*
smallest_load_task(
        model_task_t **task_list, int task_list_size,
        model_core_t *core)
{
    int task;

    int64_t smallest_load = CONV_DOUBLE_scaledUINT32(1.0);
    model_task_t* found_task = nullptr;

    for(task = 0; task < task_list_size; ++task)
        if((task_next_core(task_list[task]) == core) && (task_list[task]->sensed_data.proc_time_share_avg <= smallest_load)){
            smallest_load = task_list[task]->sensed_data.proc_time_share_avg;
            found_task = task_list[task];
        }

    BUG_ON(found_task == nullptr);

    return found_task;
}

void
vitamins_vanilla_load_balance(
        model_task_t **task_list, int task_list_size,
        model_core_t *core_list_head,
        bool agingaware)
{
    model_core_t *most_loaded,*least_loaded;

    //try to balance until no longer able to reduce imbalance
    int64_t imbalance_before = track_load_imbalance(task_list,task_list_size,core_list_head,&most_loaded,&least_loaded,agingaware);
    int64_t imbalance_after = 0;
    while(most_loaded != least_loaded){
        //take the task with the smallest load from the core with
        //the highest load and move to the core with the smallest load
        model_task_t *task = smallest_load_task(task_list,task_list_size,most_loaded);
        model_core_t *prevmapping = task_next_core(task);
        task_next_core_unmap(task);
        task_next_core_map(task,least_loaded);

        pdebug("core %d load=%u id most loaded; core %d load=%u is least loaded\n",most_loaded->position,most_loaded->load_tracking.common.load,least_loaded->position,least_loaded->load_tracking.common.load);
        pdebug("Imbalance is %ld \n",imbalance_before);
        pdebug("task %d moved to core %d\n",task->id, least_loaded->position);

        //recalculate imbalance
        imbalance_after = track_load_imbalance(task_list,task_list_size,core_list_head,&most_loaded,&least_loaded,agingaware);

        //no longer able to improve; revert and
        if(imbalance_after >= imbalance_before){
            pdebug("New imbalance of %ld is larger. Stop \n",imbalance_after);
            task_next_core_unmap(task);
            task_next_core_map(task,prevmapping);
            break;
        }
        imbalance_before = imbalance_after;
    }
}

define_vitamins_list(static model_core_t,cores);


static inline void
init_shared_once(core_info_t *core_list, int core_list_size)
{
    int core;

    clear_list(cores);

    for(core = 0; core < core_list_size; ++core)
        add_to_list_default(cores, core_list[core].this_core);
}

static
void
_vitamins_vanilla_shared_map(model_sys_t *sys, bool agingaware)
{
    int task;

    vitamins_load_tracker_set(LT_AGINGAWARE(LT_DEFAULT,agingaware));
    init_shared_once(sys->info->core_list,sys->info->core_list_size);

    clear_next_map(sys);

    for(task=0;task<sys->task_list_size;++task)
        task_next_core_map(sys->task_list[task],task_curr_core(sys->task_list[task]));

    vitamins_vanilla_load_balance(sys->task_list,sys->task_list_size,vitamins_list_head(cores),agingaware);
}

void
vitamins_vanilla_shared_map(model_sys_t *sys)
{
    _vitamins_vanilla_shared_map(sys,false);
}

void
vitamins_vanilla_shared_agingaware_map(model_sys_t *sys)
{
    _vitamins_vanilla_shared_map(sys,true);
}


