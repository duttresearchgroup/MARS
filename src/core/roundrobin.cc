//#define HASDEBUG
#include "core.h"

static uint64_t _repeat = 0;
static uint64_t _looped = 0;
static bool _wave_up = true;
static bool _wave_up_freq = true;

const int CHIP_SZ_CORES = 4;

void
_set_fixed_freq(model_sys_t * sys) {
	for (int i = 0; i < sys->info->core_list_size; i++) {
		if (sys->info->core_list[i].this_core->__vitaminslist_head_mapped_tasks == nullptr) {
			vitamins_dvfs_manual_freq(sys->info->core_list[i].this_core,COREFREQ_0000MHz);
		}
//		else {
//			vitamins_dvfs_manual_freq(sys->info->core_list[i].this_core,COREFREQ_1300MHZ);
//		}
	}
}

model_core_t*
_vitamins_roundrobin_map_1_wave(model_sys_t * sys, model_task_t *task)
{

	model_core_t *last_core = task_curr_core(task);
	int next_core_ind = last_core->info->position;

	if (_repeat % (task->id + 11) == 0) {
		if((last_core->info->position - 1*CHIP_SZ_CORES) < 0) {
			_wave_up = true;
			next_core_ind = (last_core->info->position + 1*CHIP_SZ_CORES);
		} else if((last_core->info->position + 1*CHIP_SZ_CORES) > (sys->info->core_list_size - 1)) {
			_wave_up = false;
			next_core_ind = (last_core->info->position - 1*CHIP_SZ_CORES);
		} else {
			next_core_ind = _wave_up ?
					(last_core->info->position + 1*CHIP_SZ_CORES) : (last_core->info->position - 1*CHIP_SZ_CORES);
		}
	}

	return sys->info->core_list[next_core_ind].this_core;
}

model_core_t*
_vitamins_roundrobin_wave_map_1_fix_freq(model_sys_t * sys, model_task_t *task)
{

	model_core_t *last_core = task_curr_core(task);
	int next_core_ind = last_core->info->position;

	if (_repeat % (task->id + 11) == 0) {
		if((last_core->info->position - 1*CHIP_SZ_CORES) < 0) {
			_wave_up = true;
			next_core_ind = (last_core->info->position + 1*CHIP_SZ_CORES);
		} else if((last_core->info->position + 1*CHIP_SZ_CORES) > (sys->info->core_list_size - 1)) {
			_wave_up = false;
			next_core_ind = (last_core->info->position - 1*CHIP_SZ_CORES);
		} else {
			next_core_ind = _wave_up ?
					(last_core->info->position + 1*CHIP_SZ_CORES) : (last_core->info->position - 1*CHIP_SZ_CORES);
		}
	}

	vitamins_dvfs_manual_freq(sys->info->core_list[next_core_ind].this_core,COREFREQ_1300MHZ);

	return sys->info->core_list[next_core_ind].this_core;
}

//model_core_t*
//_vitamins_roundrobin_map_1_wave(model_sys_t *sys)
//{
//	BUG_ON(sys->task_list_size != 1);
//
//	model_core_t *last_core = task_curr_core(sys->task_list[0]);
//	int next_core_ind = last_core->info->position;
//
//	if (_repeat == 0) {
//		_repeat = 115;
//		if(last_core->info->position == 0) {
//			_wave_up = true;
//			next_core_ind = (last_core->info->position + 1);
//		} else if(last_core->info->position == (sys->info->core_list_size - 1)) {
//			_wave_up = false;
//			next_core_ind = (last_core->info->position - 1);
//		} else {
//			next_core_ind = _wave_up ?
//					(last_core->info->position + 1) : (last_core->info->position - 1);
//		}
//	} else {
//		_repeat--;
//	}
//
//	return sys->info->core_list[next_core_ind].this_core;
//}

model_core_t*
_vitamins_roundrobin_map_1_wave_dvfs(model_sys_t * sys, model_task_t *task)
{
	model_core_t *last_core = task_curr_core(task);
	int next_core_ind = last_core->info->position;
	core_freq_t last_freq = last_core->info->freq->this_domain->freq;

		if (last_freq == vitamins_dvfs_get_maximum_freq(last_core)) {
			if (_looped == 0) {
				//switch core
				if (_repeat % (task->id + 1) == 0) {
					if((last_core->info->position - 1*CHIP_SZ_CORES) < 0) {
						_wave_up = true;
						next_core_ind = (last_core->info->position + 1*CHIP_SZ_CORES);
					} else if((last_core->info->position + 1*CHIP_SZ_CORES) > (sys->info->core_list_size - 1)) {
						_wave_up = false;
						next_core_ind = (last_core->info->position - 1*CHIP_SZ_CORES);
					} else {
						next_core_ind = _wave_up ?
								(last_core->info->position + 1*CHIP_SZ_CORES) : (last_core->info->position - 1*CHIP_SZ_CORES);
					}
				}
			}
		}
		last_core = sys->info->core_list[next_core_ind].this_core;
		last_freq = last_core->info->freq->this_domain->freq;
		//switch freq
		if(last_freq == COREFREQ_0000MHz) {
			vitamins_dvfs_manual_freq(last_core,vitamins_dvfs_get_maximum_freq(last_core));
			_wave_up_freq = false;
		} else if(last_freq == vitamins_dvfs_get_minimum_freq(last_core)) {
			vitamins_dvfs_manual_freq(last_core,(core_freq_t)(last_freq-1));
			_wave_up_freq = true;
		} else if(last_freq == vitamins_dvfs_get_maximum_freq(last_core)) {
			vitamins_dvfs_manual_freq(last_core,(core_freq_t)(last_freq+1));
			_wave_up_freq = false;
		} else {
			vitamins_dvfs_manual_freq(last_core,_wave_up_freq ? (core_freq_t)(last_freq-1) : (core_freq_t)(last_freq+1));
		}

	return sys->info->core_list[next_core_ind].this_core;
}

void
vitamins_roundrobin_map(model_sys_t *sys)
{
    /*
     * just round robin core scheduling
     */

	if (_looped == 0) {
		_looped = 1;
	} else {
		_looped--;
	}

//    if (_repeat == 0) {
//    	_repeat = 20;
//    } else {
//    	_repeat--;
//    }
	_repeat++;

    vitamins_load_tracker_set(LT_DEFAULT);
    clear_next_map(sys);
    for (int i = 0; i < sys->task_list_size; i++) {
    	model_core_t *next_core = _vitamins_roundrobin_wave_map_1_fix_freq(sys, sys->task_list[i]);
    	task_next_core_map(sys->task_list[i],next_core);
    }

	_set_fixed_freq(sys);
}


model_core_t*
_vitamins_ctrl_cache_map_1(model_sys_t *sys)
{
	BUG_ON(sys->task_list_size != 1);

	model_core_t *last_core = task_curr_core(sys->task_list[0]);

	uint64_t last_ips = last_core->sensed_data.last_dvfs_epoch_ips;
	double l_ips = (double)last_ips/1000;
	double last_p = last_core->sensed_data.last_dvfs_epoch_avg_power;

//	core_arch_t next_core_type = (core_arch_t)last_core->info->freq->this_domain->ctrl_cache.nextInputVal(last_p);
	core_arch_t next_core_type;
	double next_core_ind = last_core->info->freq->this_domain->ctrl_cache.nextInputVal(last_p);

	//TODO: need to use arch types not position
	if (next_core_ind <=  1.5) {
		next_core_type = COREARCH_Exynos5422_BIG_MEDIUM;
	} else if (next_core_ind <=  3) {
		next_core_type = COREARCH_Exynos5422_BIG_BIG;
	} else {
		next_core_type = COREARCH_Exynos5422_BIG_HUGE;
	}

	for (int i = 0; i < sys->info->core_list_size; i++) {
		if (sys->info->core_list[i].arch == next_core_type) {
			next_core_ind = sys->info->core_list[i].position;
			vitamins_dvfs_manual_freq(sys->info->core_list[i].this_core,COREFREQ_1300MHZ);
		} else {
			vitamins_dvfs_manual_freq(sys->info->core_list[i].this_core,COREFREQ_0000MHz);
		}
	}

	printf("LAST IPS %lu (%f) LAST P %f NEXT CORE %d (%d)\n", last_ips, l_ips, last_p, (int)next_core_ind, next_core_type);

	if (_repeat == 0) {
		last_core->info->freq->this_domain->ctrl_cache.referenceOutput(2.0);
	} else {
		_repeat--;
	}

	return sys->info->core_list[(int)next_core_ind].this_core;
}

void
vitamins_ctrl_cache_map(model_sys_t *sys)
{
    /*
     * SISO controller scheduling for variable cache size
     */
    vitamins_load_tracker_set(LT_DEFAULT);
    clear_next_map(sys);
//    for (i = 0; i < sys->task_list_size; ++i) task_next_core_map(sys->task_list[i],task_curr_core(sys->task_list[i]));

    model_core_t *next_core = _vitamins_ctrl_cache_map_1(sys);
    task_next_core_map(sys->task_list[0],next_core);
}
