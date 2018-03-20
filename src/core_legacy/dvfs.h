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

#ifndef __core_dvfs_h
#define __core_dvfs_h

#include "base/base.h"
#include "runtime/common/controllers.h"

/*
 * TODO
 * Instead of multiple algorithms, should have a single algorithm
 * with parameters
 */

typedef enum {
    //sets the frequency of active cores to the maximum and inactive cores to the minimum
    DVFS_DEFAULT,

    //sets the frequency to the values specified manually during mapping
    DVFS_MANUAL,

    //sets the frequency according to load thresholds.
    DVFS_ONDEMAND,

    //same as above, but assumes cores are grouped into clusters with the same frequency domain
    //when all cores within a domain have no tasks, the entire cluster is power gated
    //note: both DVFS_ONDEMAND_PG also work for non-clustered architectures (e.g. one freq. domain per core),
    //but in general real multi-core platforms does not have this fine-grained power gating
    DVFS_ONDEMAND_PG,

    //same as the DVFS_ONDEMAND_PG, but reduces the maximum frequency that can be set
    //according to delay degradation
    DVFS_ONDEMAND_PG_AGINGAWARE,

    //updates frequency in a sinusoidal pattern for controller training
    DVFS_WAVE,

	//uses closed-loop controller to set frequency
	DVFS_CTRL,
} dvfs_algorithm_t;

typedef SISOController<double> Controller;

struct model_freq_domain_struct {
    //static domain info
	freq_domain_info_t *info;

    //current frequency
    core_freq_t freq;

    //stores the value returned by the most recent call to vitamins_dvfs_predict_next_freq
    core_freq_t last_pred_freq;

    //this is used by the DVFS_MANUAL policy
    core_freq_t manual_freq;

    freq_domain_sensed_data_t sensed_data;

    pred_checker_freq_domain_t pred_checker;

    //this is used by the DVFS_CTRL policy
    Controller ctrl_freq;
    //this is used by the CTRL_CACHE mapper
    Controller ctrl_cache;
};
typedef struct model_freq_domain_struct model_freq_domain_t;

uint32_t vitamins_freq_to_mVolt_map(core_arch_t arch, core_freq_t freq);

void vitamins_dvfs_set_global_policy(dvfs_algorithm_t policy);
dvfs_algorithm_t vitamins_dvfs_get_global_policy(void);

//returns the core's frequency domain current frequency
core_freq_t vitamins_dvfs_get_freq(model_core_t *core);

//returns the maximum/minimum possible frequencies according to the current DVFS policy
core_freq_t vitamins_dvfs_get_maximum_freq(model_core_t *core);
core_freq_t vitamins_dvfs_get_minimum_freq(model_core_t *core);

//returns the predicted frequency for the core according to the load set by the load tracker
//used by the load tracker in a feedback loop to predict the core load as well
core_freq_t vitamins_dvfs_predict_next_freq_initial(model_core_t *core);
core_freq_t vitamins_dvfs_predict_next_freq(model_core_t *core);

//defines the frequency to be set when the DVFS_MANUAL policy is used
void vitamins_dvfs_manual_freq(model_core_t *core,core_freq_t freq);

//sets the mapping and dvfs epoch being used. Required by these policies: DVFS_ONDEMAND
void vitamins_dvfs_set_dvfs_epoch(uint32_t dvfs_epoch_us);
void vitamins_dvfs_set_map_epoch(uint32_t map_epoch_us);
uint32_t vitamins_dvfs_get_dvfs_to_map_ratio(void);

//effectivelly sets the core freq to the value predicted by vitamins_dvfs_predict_next_freq
void vitamins_dvfs_set_predicted_freq(model_core_t *core);

//returns the frequency that should be set.
//uses tasks' sensed data to compute the core's load during the last dvfs epoch and sets the frequency accordingly
core_freq_t vitamins_dvfs_compute_next_freq(model_core_t *core);

//effectivelly sets the core freq to the value computed by vitamins_dvfs_compute_next_freq
void vitamins_dvfs_set_freq(model_core_t *core);

bool vitamins_dvfs_is_power_gated(model_core_t *core);

//wakeup a power-gated core and sets the frequecy to the minimum
void vitamins_dvfs_wakeup_core(model_core_t *core);


#endif
