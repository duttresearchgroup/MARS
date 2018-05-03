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

#include "map_initial.h"
#include "map.h"

void vitamins_initial_map_task(model_sys_t *sys)
{
	int t;
	vitamins_load_tracker_set(LT_DEFAULT);
	for(t = 0; t < sys->task_list_size; ++t){
		model_task_t *task = sys->task_list[t];
		int core;
		model_core_t* lowestCore = 0;
		uint32_t lowestLoad = CONV_DOUBLE_scaledUINT32(1);
		for(core = 0; core < sys->info->core_list_size; ++core){
			BUG_ON(sys->info->core_list[core].this_core->load_tracking.common.load > CONV_DOUBLE_scaledUINT32(1));
			if(sys->info->core_list[core].this_core->load_tracking.common.load < lowestLoad){
				lowestLoad = sys->info->core_list[core].this_core->load_tracking.common.load;
				lowestCore = sys->info->core_list[core].this_core;
			}
		}

		BUG_ON(task_curr_core(task) != nullptr);
		BUG_ON(task_next_core(task) != nullptr);

		//assumes the task can initially fully load the core for mapping
		task->sensed_data.proc_time_share_avg = CONV_DOUBLE_scaledUINT32(0.99);
		task_next_core_map(task,lowestCore);
		task_commit_map(task);
	}
	//tasks mapped, revert the side effect
	for(t = 0; t < sys->task_list_size; ++t){
		sys->task_list[t]->sensed_data.proc_time_share_avg = 0;
	}
}
