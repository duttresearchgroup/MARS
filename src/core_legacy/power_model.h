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

#ifndef __core_defs_power_model_h
#define __core_defs_power_model_h

#include "base/base.h"
#include "dvfs.h"
#include "cfs.h"
#include "map.h"
#include "idlepower.h"

/*
 * Functions to compute perf. and power when doing mapping
 *
 * All computations are base on the task-to-core mapping returned by the task_next_core() function
 */


/*
 * task_next_active_ips/power
 *    ips/power only for the task active phase at the core set to be its next mapping
 */
static inline
uint32_t
task_active_ips(model_task_t *task){
    return task->ips_active[task_next_core_type(task)][task_next_core_pred_freq(task)];
}

/*
 * task_max_ips
 *    total and maximum total ips for the task at the core set to be its next mapping
 */
static inline
uint32_t
task_max_total_ips(model_task_t *task){
    return CONV_scaledINTany_INTany(task->ips_active[task_next_core_type(task)][task_next_core_pred_freq(task)]
                                    * task->tlc[task_next_core_type(task)][task_next_core_pred_freq(task)]);
}


static inline
uint32_t
_task_total_load(model_task_t *task, bool with_overhead){
	uint32_t taskLoad = with_overhead ? vitamins_load_tracker_task_load_with_overhead(task)
			                          : vitamins_load_tracker_task_load(task);
	//it wouldn't make sense to use this function otherwise
	BUG_ON((vitamins_load_tracker_get() == LT_CFS) && (taskLoad > task->tlc[task_next_core_type(task)][task_next_core_pred_freq(task)]));
	return taskLoad;
}
static inline
uint32_t
task_total_load(model_task_t *task){
	return _task_total_load(task,true);
}
static inline
uint32_t
task_total_load_nooverhead(model_task_t *task){
	return _task_total_load(task,false);
}


static inline
uint32_t
task_total_ips(model_task_t *task){
    return CONV_scaledINTany_INTany(
    		task->ips_active[task_next_core_type(task)][task_next_core_pred_freq(task)] * task_total_load(task)
    		);

}

/*
 * task_max_power
 *    the total amount of power the core may consume given its next mapping
 */

static inline
uint32_t
core_total_load(model_core_t *core)
{
	return core->load_tracking.common.load;
}


static inline
uint32_t
systask_total_ips(model_core_t *core)
{
	return CONV_scaledINTany_INTany(//remove scaling drom tlc
			core->systask->ips_active[core->info->arch][core->info->freq->this_domain->last_pred_freq]
	      * core->systask->tlc[core->info->arch][core->info->freq->this_domain->last_pred_freq]);
}

static inline
uint32_t
core_total_power(model_core_t *core){
    model_task_t *task;
    uint32_t core_idle;
    uint32_t core_power = 0;

    //the core will be power gated
    if(core->info->freq->this_domain->last_pred_freq == COREFREQ_0000MHz){
    	BUG_ON(core->load_tracking.common.task_cnt != 0);
    	return 0;//power gated core has 0 power
    }

    for_each_in_internal_list(core,mapped_tasks,task,mapping){
    	uint32_t task_load = task_total_load_nooverhead(task);
        core_power += task->power_active[task_next_core_type(task)][task_next_core_pred_freq(task)] * task_load;
    }

    //account for sys power
    core_power += core->systask->power_active[core->info->arch][core->info->freq->this_domain->last_pred_freq]
                * core->systask->tlc[core->info->arch][core->info->freq->this_domain->last_pred_freq];

    //account for idle power
    core_idle = CONV_INTany_scaledINTany(1) - core->load_tracking.common.load;
    core_power += core_idle * arch_idle_power_scaled(core->info->arch, core->info->freq->this_domain->last_pred_freq);

    core_power = CONV_scaledINTany_INTany(core_power);//remove tge scalig from load
    return core_power;
}


//calc perf/power using CFS model and applying optional correction function



static inline
uint32_t
system_total_ips(model_sys_t *sys)
{
    int core;
    model_core_t *currCore;
    model_task_t *currTask;
    uint32_t totalIPS = 0;
    for (core = 0; core < sys->info->core_list_size; ++core){
    	currCore = sys->info->core_list[core].this_core;
    	for_each_in_internal_list(currCore,mapped_tasks,currTask,mapping){
    		totalIPS += task_total_ips(currTask);
    	}
    	//sys IPS
    	totalIPS += systask_total_ips(currCore);
    }
    return totalIPS;
}

static inline
uint32_t
domain_total_power(model_power_domain_t *domain)
{
	core_info_t *currCore;
	uint32_t domainPower = 0;
	for_each_in_internal_list(domain->info,cores,currCore,power_domain){
		domainPower += core_total_power(currCore->this_core);
	}
	return domainPower;
}


static inline
uint32_t
system_total_power(model_sys_t *sys)
{
    int domain;
    uint32_t totalPower = 0;
    for(domain = 0; domain < sys->info->power_domain_list_size; ++domain){
    	totalPower += domain_total_power(sys->info->power_domain_list[domain].this_domain);
    }
    return totalPower;
}


#endif
