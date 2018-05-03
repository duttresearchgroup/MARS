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

#ifndef __core_defs_map_h
#define __core_defs_map_h

#include "base/base.h"
#include "dvfs.h"
#include "cfs.h"

static inline void __task_next_core_map(model_task_t *task, model_core_t *core)
{
    BUG_ON(core == nullptr);
    task->_next_mapping_ = core;
    add_to_internal_list(core, mapped_tasks,task,mapping);
    vitamins_load_tracker_map_changed(core);
}
static inline void task_next_core_map(model_task_t *task, model_core_t *core)
{
	__task_next_core_map(task,core);
    //core cannot be power gated after having a task on it
	BUG_ON(core->info->freq->this_domain == nullptr);
    BUG_ON(core->info->freq->this_domain->last_pred_freq == COREFREQ_0000MHz);
}
static inline void task_next_core_unmap(model_task_t *task)
{
    model_core_t *core = task->_next_mapping_;
    BUG_ON(core == nullptr);
    remove_from_internal_list(core,mapped_tasks,task,mapping);
    task->_next_mapping_ = nullptr;
    task->_next_mapping_load = INVALID_METRIC_VAL;
    vitamins_load_tracker_map_changed(core);
}

static inline void task_commit_map(model_task_t *task)
{
	task->_curr_mapping_ = task->_next_mapping_;
}

//Should be always called by the mapping algorithm
//at some point in the beggining of the process.
static inline void clear_next_map(model_sys_t *sys){
    int ctrl;
    for(ctrl = 0; ctrl < sys->task_list_size; ++ctrl){
    	sys->task_list[ctrl]->_next_mapping_ = nullptr;
    	sys->task_list[ctrl]->_next_mapping_load = INVALID_METRIC_VAL;
        clear_object(sys->task_list[ctrl],mapping);
    }
    for(ctrl = 0; ctrl < sys->info->core_list_size; ++ctrl){
    	BUG_ON(sys->info->core_list[ctrl].this_core==nullptr);
        clear_internal_list(sys->info->core_list[ctrl].this_core,mapped_tasks);
    }
    //must first clear all cores first because of clustering
    for(ctrl = 0; ctrl < sys->info->core_list_size; ++ctrl){
        vitamins_load_tracker_map_changed(sys->info->core_list[ctrl].this_core);
    }
}

static inline model_core_t* task_curr_core(model_task_t *task) { return task->_curr_mapping_; }
static inline model_core_t* task_next_core(model_task_t *task) { return task->_next_mapping_; }

static inline int task_curr_core_idx(model_task_t *task) {
	BUG_ON(task->_curr_mapping_==nullptr);
	return task->_curr_mapping_->info->position;
}
static inline int task_next_core_idx(model_task_t *task) {
	BUG_ON(task->_next_mapping_==nullptr);
	return task->_next_mapping_->info->position;
}

static inline core_arch_t task_curr_core_type(model_task_t *task) {
	BUG_ON(task->_curr_mapping_==nullptr);
	return task->_curr_mapping_->info->arch;
}
static inline core_arch_t task_next_core_type(model_task_t *task) {
	BUG_ON(task->_next_mapping_==nullptr);
	return task->_next_mapping_->info->arch;
}

static inline core_freq_t task_curr_core_freq(model_task_t *task) {
	BUG_ON(task->_curr_mapping_==nullptr);
	return vitamins_dvfs_get_freq(task->_curr_mapping_);
}
static inline core_freq_t task_next_core_pred_freq(model_task_t *task) {
	BUG_ON(task->_next_mapping_==nullptr);
	BUG_ON(task->_next_mapping_->info->freq->this_domain==nullptr);
	BUG_ON(task->_next_mapping_->info->freq->this_domain->last_pred_freq == COREFREQ_0000MHz);
	return task->_next_mapping_->info->freq->this_domain->last_pred_freq;
}

#endif
