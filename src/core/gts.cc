/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
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

//#define HASDEBUG
#include "core.h"

#define LOAD_UP_THR   CONV_DOUBLE_scaledUINT32(0.9)
#define LOAD_DOWN_THR CONV_DOUBLE_scaledUINT32(0.4)

define_vitamins_list(static model_core_t,cores[SIZE_COREARCH]);

static core_arch_t fastest_type;
static core_arch_t slowest_type;

define_vitamins_list(static model_task_t,up_thr);
define_vitamins_list(static model_task_t,down_thr);

static void
init_core_arch_lists(core_info_t *core_list, int core_list_size)
{
    int core,type;

    for (type = 0; type < SIZE_COREARCH; ++type) clear_list(cores[type]);

    for(core = 0; core < core_list_size; ++core)
        add_to_list_default(cores[core_list[core].arch], core_list[core].this_core);

    //fastest and slowest ones available
    fastest_type = (core_arch_t)(SIZE_COREARCH-1);
    slowest_type = (core_arch_t)0;
    for (type = 0; type <= SIZE_COREARCH-1; ++type) {
        if(vitamins_list_head(cores[type]) != nullptr){
            if((core_arch_t)type < fastest_type) fastest_type = (core_arch_t)type;
            if((core_arch_t)type > slowest_type) slowest_type = (core_arch_t)type;
        }
    }

    //BUG_ON(fastest_type < COREARCH_GEM5_BIG_BIG);
    //BUG_ON(slowest_type > COREARCH_GEM5_LITTLE_LITTLE);
    BUG_ON(vitamins_list_head(cores[slowest_type]) == nullptr);
    BUG_ON(vitamins_list_head(cores[fastest_type]) == nullptr);

    //pdebug("Core type %d is the fastest\n", fastest_type);
    //pdebug("Core type %d is the slowest\n", slowest_type);
}


static void
init_task_lists(model_sys_t *sys, bool agingaware)
{
    int _task;
    model_task_t *task;
    model_core_t *core;

    //mark all as free
    clear_next_map(sys);

    clear_list(up_thr);
    clear_list(down_thr);

    //create the UP/DOWN thresholds lists and mark busy cores
    for(_task = 0; _task < sys->task_list_size; ++_task){
        task = sys->task_list[_task];
        core = task_curr_core(task);

        task_next_core_map(task,core);
        //pdebug("Task %d has tlc of %ld\n",task->id,task->sensed_data.proc_time_share);

        if (task->sensed_data.proc_time_share_avg > LOAD_UP_THR){
            //migration candidate unless there are no faster core for this task
            if(core->info->arch > fastest_type){
                add_to_list_default(up_thr, task);
                //pdebug("Task %d above thr; should go to faster core\n",task->id);
            }
            else{
                //pdebug("Task %d above thr; but no core is faster then current\n",task->id);
            }
        }
        else if(task->sensed_data.proc_time_share_avg < LOAD_DOWN_THR){
            //migration candidate unless there are no slower core for this task
            if(core->info->arch < slowest_type){
                add_to_list_default(down_thr, task);
                //pdebug("Task %d below thr; should go to slower core\n",task->id);
            }
            else{
                //pdebug("Task %d below thr; but no core is slower then current\n",task->id);
            }
        }
        else{
           //pdebug("Task %d is fine; should should stay at core %d \n",task->id,task->curr_mapping);
        }
    }

}

static model_core_t*
search_empty_core(core_arch_t type)
{
    //pdebug("search_available_core type=%d\n",type);
    model_core_t *core;
    for_each_in_list_default(cores[type],core){
        //pdebug("\tCore %d, available=%d\n",next->position,next->available);
        if(core->load_tracking.common.task_cnt == 0) return core;
    }
    return core;
}

static model_task_t*
search_task_at_type(model_task_t *head, core_arch_t arch)
{
    model_task_t *task;
    for_each_in_list_head_default(head,task){
        if(task_curr_core_type(task) == arch) break;
    }
    return task;
}


static void
map_to_slower(model_task_t *task, bool allow_sharing, bool agingaware)
{
    int core_type;
    model_core_t *next_core = nullptr;
    model_task_t *other_task = nullptr;
    model_core_t *curr_core = task_curr_core(task);

    //pdebug("Trying to map task %d to a slower core. Currently on core %d, type %d\n",task->id,task->curr_mapping,curr_core->type.arch);

    //search a  candidate across all slower cores
    for (core_type = (int)(curr_core->info->arch) + 1; core_type <= (int)slowest_type; ++core_type) {

        //pdebug("\tTrying core type %d\n",core_type);

        //if we have a free core, map and we are done
        next_core = search_empty_core((core_arch_t)core_type);
        if(next_core != nullptr) {
            remove_from_list_default(down_thr,task);

            task_next_core_unmap(task);//it's already mapped to the previous one
            task_next_core_map(task,next_core);
            //pdebug("\tMapped task %d core %d of type %d\n",task->id,next_core->position,next_core->type.arch);
            return;
        }

        //try and see if I have a task in the other list that can switch with me
        other_task = search_task_at_type(vitamins_list_head(up_thr), (core_arch_t)core_type);
        if (other_task != nullptr) {
            //we can map two task now !
            remove_from_list_default(down_thr,task);
            remove_from_list_default(up_thr,other_task);

            //switch
            task_next_core_unmap(task);//both already mapped to the previous one
            task_next_core_unmap(other_task);

            task_next_core_map(task,task_curr_core(other_task));
            task_next_core_map(other_task,task_curr_core(task));

            //pdebug("\tMapped task %d core %d of type %d\n",task->id,task->next_mapping,core_list[task->next_mapping].type.arch);
            //pdebug("\t\tswitched with task %d now on core %d of type %d\n",other_task->id,other_task->next_mapping,core_list[other_task->next_mapping].type.arch);

            return;
        }

        //just map to the head core; will load balance later
        if(allow_sharing && (vitamins_list_head(cores[core_type]) != nullptr)){
            task_next_core_unmap(task);//it's already mapped to the previous one
            task_next_core_map(task,vitamins_list_head(cores[core_type]));
            remove_from_list_default(down_thr,task);
            return;
        }

        //else will attemp to map to a even faster core type
    }
    pdebug("\tCouldn't map task %d\n",task->id);

}

static void
map_to_faster(model_task_t *task, bool allow_sharing, bool agingaware)
{
    int core_type;
    model_core_t *next_core = nullptr;
    model_task_t *other_task = nullptr;
    model_core_t *curr_core = task_curr_core(task);

    //pdebug("Trying to map task %d to a faster core. Currently on core %d, type %d\n",task->id,task->curr_mapping,curr_core->type.arch);

    //search a  candidate across all faster cores
    for (core_type = (int)(curr_core->info->arch) - 1; core_type >= (int)fastest_type; --core_type) {

        //pdebug("\tTrying core type %d\n",core_type);

        //if we have a free core, map and we are done
        next_core = search_empty_core((core_arch_t)core_type);
        if(next_core != nullptr) {
            remove_from_list_default(up_thr,task);

            task_next_core_unmap(task);//it's already mapped to the previous one
            task_next_core_map(task,next_core);
            //pdebug("\tMapped task %d core %d of type %d\n",task->id,next_core->position,next_core->type.arch);
            return;
        }

        //try and see if I have a task in the other list that can switch with me
        other_task = search_task_at_type(vitamins_list_head(down_thr), (core_arch_t)core_type);
        if (other_task != nullptr) {
            //we can map two task now !
            remove_from_list_default(up_thr,task);
            remove_from_list_default(down_thr,other_task);

            //switch
            task_next_core_unmap(task);//both already mapped to the previous one
            task_next_core_unmap(other_task);

            task_next_core_map(task,task_curr_core(other_task));
            task_next_core_map(other_task,task_curr_core(task));

            //pdebug("\tMapped task %d core %d of type %d\n",task->id,task->next_mapping,core_list[task->next_mapping].type.arch);
            //pdebug("\t\tswitched with task %d now on core %d of type %d\n",other_task->id,other_task->next_mapping,core_list[other_task->next_mapping].type.arch);

            return;
        }

        //just map to the head core; will load balance later
        if(allow_sharing && (vitamins_list_head(cores[core_type]) != nullptr)){
            task_next_core_unmap(task);//it's already mapped to the previous one
            task_next_core_map(task,vitamins_list_head(cores[core_type]));
            remove_from_list_default(up_thr,task);
            return;
        }

        //else will attemp to map to a even faster core type
    }
    pdebug("\tCouldn't map task %d\n",task->id);
}

static void
balance_types(model_sys_t *sys, bool agingaware)
{
    int core_type;
    for (core_type = 0; core_type < SIZE_COREARCH; ++core_type) {
        vitamins_vanilla_load_balance(sys->task_list,sys->task_list_size,vitamins_list_head(cores[core_type]),agingaware);
    }
}

static
void
_vitamins_gts_map(model_sys_t *sys, bool allow_sharing, bool agingaware)
{
    model_task_t *curr_task;

    vitamins_load_tracker_set(LT_AGINGAWARE(LT_DEFAULT,agingaware));

    //init cores and creates lists according to load thresholds
    init_core_arch_lists(sys->info->core_list,sys->info->core_list_size);
    init_task_lists(sys,agingaware);

    //try to map tasks above the threshold to faster cores
    for_each_in_list_default(up_thr,curr_task){
        map_to_faster(curr_task,allow_sharing,agingaware);
    }

    //try to map tasks below the threshold to slower cores
    for_each_in_list_default(down_thr,curr_task){
        map_to_slower(curr_task,allow_sharing,agingaware);
    }

    if(allow_sharing){

        //load balance between cores of the same type
        balance_types(sys,agingaware);
    }
}

void
vitamins_gts_map(model_sys_t *sys)
{
    _vitamins_gts_map(sys,false,false);
}

void
vitamins_gts_shared_map(model_sys_t *sys)
{
    _vitamins_gts_map(sys,true,false);
}

void
vitamins_gts_shared_agingaware_map(model_sys_t *sys)
{
    _vitamins_gts_map(sys,true,true);
}
