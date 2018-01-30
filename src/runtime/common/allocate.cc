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

#include "allocate.h"
#include "helpers.h"


void vitamins_predict_allocate(model_sys_t *sys)
{
	//int i;
	//for(i = 0; i < sys->freq_domain_list_size; ++i){
	//	pinfo("fd %d freq=%u\n",i,sys->freq_domain_list[i].sensed_data.avg_freqMHz);
	//}

	//vitamins_dumb_exynos_predictor(sys);
    vitamins_bin_predictor(sys);

    vitamins_sparta_map(sys);

    vitamins_bin_predictor_commit(sys);

    //for(i = 0; i < sys->freq_domain_list_size; ++i){
    //	pinfo("fd %d pred_freq=%u\n",i,sys->freq_domain_list[i].pred_checker.prev_pred_freq);
    //}
    //pinfo("c 0 pred_load=%u\n",sys->core_list[0].pred_checker.prev_pred_load);
    //pinfo("c 4 pred_load=%u\n",sys->core_list[4].pred_checker.prev_pred_load);

}

void vitamins_maping_test(model_sys_t *sys)
{

	vitamins_bin_predictor(sys);

	vitamins_odroid_test_map(sys);

	vitamins_bin_predictor_commit(sys);
}

void vitamins_remap(model_sys_t *sys)
{
    int t;
    model_task_t *task;

	for(t = 0; t < sys->task_list_size; ++t){
		task = sys->task_list[t];
		assert_false(task_curr_core(task) == nullptr);
		assert_false(task_next_core(task) == nullptr);

		if(task_curr_core(task) != task_next_core(task)){
			bool not_migrated = actuate_change_cpu(task->id,task_next_core_idx(task));
			if(!not_migrated){
				task_commit_map(task);
			}
			else{
				pinfo("vitamins_remap: failed to migrate task %d\n",task->id);
			}
		}
	}

}

static int overheadtest_curr_core = -1;

void vitamins_remap_overheadtest(model_sys_t *sys,int core0, int core1)
{
	int t;
    model_task_t *task;

	if(overheadtest_curr_core == core0)
		overheadtest_curr_core = core1;
	else
		overheadtest_curr_core = core0;

	for(t = 0; t < sys->task_list_size; ++t){
		task = sys->task_list[t];

		//TODO ugly
		task->_next_mapping_ = sys->info->core_list[overheadtest_curr_core].this_core;

		assert_false(task_curr_core(task) == nullptr);
		assert_false(task_next_core(task) == nullptr);

		if(task_curr_core(task) != task_next_core(task)){
			bool not_migrated = actuate_change_cpu(task->id,task_next_core_idx(task));
			if(!not_migrated){
				task_commit_map(task);
			}
			else{
				pinfo("vitamins_remap: failed to migrate task %d\n",task->id);
			}
		}
	}
}

