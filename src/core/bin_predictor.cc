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

static const uint32_t MAX_ALLOWED_TLC = CONV_DOUBLE_scaledUINT32(1);
static const uint32_t MIN_ALLOWED_TLC = CONV_DOUBLE_scaledUINT32(0.001);

#ifndef UINT32_MAX
	#define UINT32_MAX             (4294967295U)
#endif


bin_pred_ptr_t bin_predictors_ipcpower = nullptr;


void
vitamins_bin_predictor_commit(model_sys_t *sys)
{
    int i;
    model_task_t *task;
    model_core_t *core;
    model_freq_domain_t *fd;
    model_power_domain_t *pd;
    for(i = 0; i < sys->task_list_size; ++i){
        task = sys->task_list[i];
        task->pred_checker.prev_pred_ips_active = task_active_ips(task);
        task->pred_checker.prev_pred_load = task_total_load(task);
        task->pred_checker.prev_pred_valid = true;
    }
    for(i = 0; i < sys->info->core_list_size; ++i){
    	core = sys->info->core_list[i].this_core;
    	core->pred_checker.prev_pred_load = core_total_load(core);
    	core->pred_checker.prev_pred_valid = true;
    }
    for(i = 0; i < sys->info->freq_domain_list_size; ++i){
    	fd = sys->info->freq_domain_list[i].this_domain;
    	fd->pred_checker.prev_pred_freq = (fd->last_pred_freq==COREFREQ_0000MHz)?0:freqToValMHz_i(fd->last_pred_freq);
    	fd->pred_checker.prev_pred_valid = true;
    }
    for(i = 0; i < sys->info->power_domain_list_size; ++i){
    	pd = sys->info->power_domain_list[i].this_domain;
    	pd->pred_checker.prev_pred_power = domain_total_power(pd);
    	pd->pred_checker.prev_pred_valid = true;
    }

}

//return the correction factor
inline static
int32_t
_add_up_error(pred_error_t *err, uint32_t pred_val, uint32_t val)
{
	int32_t correction = 0;
	if((pred_val != val) && (val != 0)){
		int32_t error_abs = (int32_t)pred_val - (int32_t)val;
		int32_t error_rel = CONV_INTany_scaledINTany(error_abs) / (int32_t)val;
		int32_t error_rel_inv = error_rel*-1;
		err->pred_error_acc_s += error_rel;
		err->pred_error_acc_u += error_rel < 0 ? error_rel_inv : error_rel;
		correction = error_rel_inv/3;
	}
	else if((pred_val != val) && (val == 0)){
		//assume 100% in this case, otherwise any attempt would yield infinity
		err->pred_error_acc_s += CONV_INTany_scaledINTany(1);
		err->pred_error_acc_u += CONV_INTany_scaledINTany(1);
	}
	//else error is 0
	err->pred_error_cnt += 1;
	return correction;
}
#define add_up_error(container,field,correction_field,val)\
	do{\
		if(container->pred_checker.prev_pred_valid)\
			container->pred_checker.correction_##correction_field += \
				_add_up_error(&(container->pred_checker.error_##field),container->pred_checker.prev_pred_##field,val);\
	}while(0)

pred_checker_task_t all_taks_error_acc;

inline static
void acc_task_error(model_task_t *task)
{
	all_taks_error_acc.error_ips_active.pred_error_acc_u += task->pred_checker.error_ips_active.pred_error_acc_u;
	all_taks_error_acc.error_ips_active.pred_error_cnt += task->pred_checker.error_ips_active.pred_error_cnt;
	all_taks_error_acc.error_load.pred_error_acc_u += task->pred_checker.error_load.pred_error_acc_u;
	all_taks_error_acc.error_load.pred_error_cnt += task->pred_checker.error_load.pred_error_cnt;
}


uint32_t
vitamins_pred_correction(int32_t pred_correction, uint32_t predVal, uint32_t min, uint32_t max){
    int32_t result;
	int32_t pred = (int32_t)predVal;
    int32_t correction = pred * pred_correction;
    correction = CONV_scaledINTany_INTany(correction);//correction was scaled
    BUG_ON(pred < 0);
    result = pred+correction;

    if(result <= 0)
    	return min;

    if((uint32_t)result > max)
    	return max;

    return result;
}

inline static
uint32_t
ips_correction(model_task_t *task, uint32_t predVal, core_arch_t core_type)
{
	//return vitamins_pred_correction(task->pred_checker.correction_ips_active[core_type],predVal,1,UINT32_MAX);
	return vitamins_pred_correction(task->pred_checker.correction_ips_active,predVal,1,UINT32_MAX);
}


static
void
vitamins_bin_predictor_calc_error(model_sys_t *sys)
{
    int i;
    model_task_t *task;
    model_core_t *core;
    model_freq_domain_t *fd;
    model_power_domain_t *pd;
    for(i = 0; i < sys->task_list_size; ++i){
        task = sys->task_list[i];
        //add_up_error(task,ips_active,ips_active[task_curr_core_type(task)],task->sensed_data.ips_active);
        add_up_error(task,ips_active,ips_active,task->sensed_data.ips_active);
        add_up_error(task,load,load,task->sensed_data.proc_time_share);
        acc_task_error(task);
    }
    for(i = 0; i < sys->info->core_list_size; ++i){
    	core = sys->info->core_list[i].this_core;
    	add_up_error(core,load,load,core->sensed_data.avg_load);
    }
    for(i = 0; i < sys->info->freq_domain_list_size; ++i){
    	fd = sys->info->freq_domain_list[i].this_domain;
    	add_up_error(fd,freq,freq,fd->sensed_data.avg_freqMHz);
    }
    for(i = 0; i < sys->info->power_domain_list_size; ++i){
    	pd = sys->info->power_domain_list[i].this_domain;
    	add_up_error(pd,power,power,pd->sensed_data.avg_power);
    }
}


static inline int find_bin(bin_pred_layer_t *currLayer, model_task_t* task)
{
    int idx = 0;
    for(idx = 0; idx < currLayer->num_of_bins; ++idx){
        if((currLayer->metric)(&(task->sensed_data)) <=  currLayer->bins[idx]) break;
    }
    BUG_ON(idx >= currLayer->num_of_bins);
    BUG_ON((currLayer->metric)(&(task->sensed_data)) > currLayer->bins[idx]);
    return idx;
}

bool
vitamins_bin_predictor_exists(core_arch_t arch)
{
    int srcCore;

    if(bin_predictors_ipcpower == nullptr) return false;

    for(srcCore = 0; srcCore < SIZE_COREARCH; ++srcCore){
    	if(bin_predictors_ipcpower[srcCore][arch] != nullptr)
    		return true;
    }
    return false;
}

static
bin_pred_result_t*
_vitamins_bin_predict(model_task_t* task,
                 core_arch_t srcCoreType,core_freq_t srcCoreFreq,core_arch_t tgtCoreType,core_freq_t tgtCoreFreq,
                 bin_pred_ptr_t bin_predictors)
{
	bin_pred_layer_t *currLayer;
	int idx;

	BUG_ON(bin_predictors == nullptr);
	BUG_ON(bin_predictors[srcCoreType] == nullptr);
	BUG_ON(bin_predictors[srcCoreType][srcCoreFreq] == nullptr);
    BUG_ON(bin_predictors[srcCoreType][srcCoreFreq][tgtCoreType] == nullptr);
    currLayer = bin_predictors[srcCoreType][srcCoreFreq][tgtCoreType][tgtCoreFreq];

    if(currLayer == nullptr) return nullptr;

    while(currLayer->bin_result == nullptr){
        BUG_ON(currLayer->next_layer == nullptr);
        currLayer = currLayer->next_layer[find_bin(currLayer,task)];
        BUG_ON(currLayer == nullptr);
    }
    BUG_ON(currLayer->next_layer != nullptr);

    idx = find_bin(currLayer,task);

    return &(currLayer->bin_result[idx]);
}


static inline uint32_t linear_interpolation(int32_t x, int32_t x0, int32_t y0, int32_t x1, int32_t y1){
	int32_t result = y0 + ((y1-y0)*(x-x0))/(x1-x0);
	BUG_ON(result <= 0);
	return result;
}

/*
static
bin_pred_result_t
vitamins_bin_interpolate_predict(vitamins_task_t* task,
                 vitamins_core_arch_t srcCoreType,vitamins_core_arch_t tgtCoreType,vitamins_core_freq_t tgtCoreFreq,
                 bin_pred_ptr_t bin_predictors)
{
	//frquency is unnavailable. At least two frequencies should be available to use the predictor
	//interpolate between those
	bin_pred_result_t _result_tmp;
	int freq;
	bin_pred_result_t *result_lower;
	bin_pred_result_t *result_upper;
	vitamins_core_freq_t lower_freq = SIZE_COREFREQ;
	vitamins_core_freq_t upper_freq = SIZE_COREFREQ;

	//find the lower closest available frequency
	for(freq = (int)tgtCoreFreq+1; freq < SIZE_COREFREQ; ++freq)
		if(vitamins_arch_freq_available(tgtCoreType,(vitamins_core_freq_t)freq)
			&&
			(bin_predictors[srcCoreType][tgtCoreType][freq] != nullptr )){
			lower_freq = freq;
			break;
		}
	//find the upper closest available frequency
	for(freq = (int)tgtCoreFreq-1; freq >= 0; --freq)
		if(vitamins_arch_freq_available(tgtCoreType,(vitamins_core_freq_t)freq)
			&&
			(bin_predictors[srcCoreType][tgtCoreType][freq] != nullptr )){
			upper_freq = freq;
			break;
		}

	BUG_ON(lower_freq == SIZE_COREFREQ);
	BUG_ON(upper_freq == SIZE_COREFREQ);

	result_lower = _vitamins_bin_predict(task,srcCoreType,tgtCoreType,lower_freq,bin_predictors);
	result_upper = _vitamins_bin_predict(task,srcCoreType,tgtCoreType,upper_freq,bin_predictors);
	BUG_ON(result_lower == nullptr);
	BUG_ON(result_upper == nullptr);

	_result_tmp.ipcActive = linear_interpolation(freqToValMHz_i(tgtCoreFreq),
			freqToValMHz_i(lower_freq),result_lower->ipcActive,
			freqToValMHz_i(upper_freq),result_upper->ipcActive);
	_result_tmp.powerActive = linear_interpolation(freqToValMHz_i(tgtCoreFreq),
			freqToValMHz_i(lower_freq),result_lower->powerActive,
			freqToValMHz_i(upper_freq),result_upper->powerActive);

	return _result_tmp;
}
*/

static inline
bin_pred_result_t*
vitamins_bin_predict(model_task_t* task,
		core_arch_t srcCoreType, core_freq_t srcCoreFreq, core_arch_t tgtCoreType,core_freq_t tgtCoreFreq,
		bin_pred_ptr_t bin_predictors)
{
	bin_pred_result_t *result = _vitamins_bin_predict(task,srcCoreType,srcCoreFreq,tgtCoreType,tgtCoreFreq,bin_predictors);
	BUG_ON(result == nullptr);
	return result;
}

static
void
vitamins_bin_predict_for_task(model_task_t* task)
{

    core_arch_t currCoreType = task_curr_core_type(task);
    core_freq_t currCoreFreq = task->sensed_data.avg_freq;

    uint32_t avgTLC = vitamins_estimate_tlc(task);
    uint32_t avgIPS = task->sensed_data.ips_active;//used as a reference for TLC prediciton only

    for_enum(core_arch_t,coreType,0,SIZE_COREARCH,+1){
    	for_enum(core_freq_t,coreFreq,0,SIZE_COREFREQ,+1){
            if(vitamins_arch_freq_available(coreType,coreFreq)){
            	uint32_t tgtIPC,tgtPower,tgtIPSactive,tgtTLC;
            	bin_pred_result_t *result = vitamins_bin_predict(task,currCoreType,currCoreFreq,coreType,coreFreq,bin_predictors_ipcpower);

            	//predict the active IPC and power
            	tgtIPC = result->ipcActive;
                tgtPower = result->powerActive;

                //IPS active
                tgtIPSactive = tgtIPC * freqToValMHz_i(coreFreq);
                tgtIPSactive = CONV_scaledINTany_INTany(tgtIPSactive);//ips is not scaled

                //apply correction
                tgtIPSactive = ips_correction(task,tgtIPSactive,coreType);

                //Predict TLC
                tgtTLC = vitamins_predict_tlc(task,avgIPS,avgTLC,tgtIPSactive);
                //correct
                if(tgtTLC > MAX_ALLOWED_TLC) tgtTLC = MAX_ALLOWED_TLC;
                if(tgtTLC < MIN_ALLOWED_TLC) tgtTLC = MIN_ALLOWED_TLC;

                //task->ipc_active[coreType] = tgtIPC;
                task->ips_active[coreType][coreFreq] = tgtIPSactive;
                task->tlc[coreType][coreFreq] = tgtTLC;
                task->power_active[coreType][coreFreq] = tgtPower;
            }
        }
    }
}

/*
static
void
vitamins_dumb_exynos_predict_for_task(vitamins_task_t* task)
{

	vitamins_core_arch_t coreType;
	vitamins_core_freq_t coreFreq;

	vitamins_core_arch_t currCoreType = task_curr_core_type(task);

	uint32_t avgTLC = vitamins_estimate_tlc(task);
	uint32_t avgIPS = task->sensed_data.ips_active;//used as a reference for TLC prediciton only
	uint32_t avgIPC = task->sensed_data.ipc_active;

	for(coreType = 0; coreType < SIZE_COREARCH; ++coreType){
		for(coreFreq = 0; coreFreq < SIZE_COREFREQ; ++coreFreq){
			if(vitamins_arch_freq_available(coreType,coreFreq)){
				uint32_t tgtIPC,tgtPower,tgtIPSactive,tgtTLC;

				BUG_ON((coreType!=COREARCH_Exynos5422_BIG) && (coreType!=COREARCH_Exynos5422_LITTLE));

				if(currCoreType == coreType)
					tgtIPC = avgIPC;
				else if(coreType == COREARCH_Exynos5422_BIG){
					tgtIPC = avgIPC*CONV_DOUBLE_scaledUINT32(1.4);
					tgtIPC = CONV_scaledINTany_INTany(tgtIPC);
					BUG_ON(tgtIPC <= avgIPC);
				}
				else if(coreType == COREARCH_Exynos5422_LITTLE){
					tgtIPC = CONV_INTany_scaledINTany(avgIPC) / CONV_DOUBLE_scaledUINT32(1.4);
					BUG_ON(tgtIPC >= avgIPC);
				}
				else BUG_ON(true);

				if(coreType == COREARCH_Exynos5422_BIG){
					tgtPower = arch_idle_power_scaled(coreType, coreFreq) * 8;
				}
				else if(coreType == COREARCH_Exynos5422_LITTLE){
					tgtPower = arch_idle_power_scaled(coreType, coreFreq) * 5;
				}
				else BUG_ON(true);

				BUG_ON(tgtPower <= arch_idle_power_scaled(coreType, coreFreq));

				//IPS active
				tgtIPSactive = tgtIPC * freqToValMHz_i(coreFreq);
				tgtIPSactive = CONV_scaledINTany_INTany(tgtIPSactive);//ips is not scaled

				//Predict TLC
				tgtTLC = vitamins_predict_tlc(task,avgIPS,avgTLC,tgtIPSactive);
				//correct
				if(tgtTLC > MAX_ALLOWED_TLC) tgtTLC = MAX_ALLOWED_TLC;
				if(tgtTLC < MIN_ALLOWED_TLC) tgtTLC = MIN_ALLOWED_TLC;

				//task->ipc_active[coreType] = tgtIPC;
				task->ips_active[coreType][coreFreq] = tgtIPSactive;
				task->tlc[coreType][coreFreq] = tgtTLC;
				task->power_active[coreType][coreFreq] = tgtPower;
			}
		}
	}
}


static
void
vitamins_halfdumb_exynos_predict_for_task(vitamins_task_t* task)
{
    vitamins_core_arch_t coreType;
    vitamins_core_freq_t coreFreq;

    vitamins_core_arch_t currCoreType = task_curr_core_type(task);
    vitamins_core_freq_t currCoreFreq = task->sensed_data.avg_freq;

    uint32_t avgTLC = vitamins_estimate_tlc(task);
    //uint32_t avgFreqValMHz = task->sensed_data.avg_freq_mhz;//used as a reference for TLC prediciton only
    uint32_t avgIPS = task->sensed_data.ips_active;//used as a reference for TLC prediciton only

    for(coreType = 0; coreType < SIZE_COREARCH; ++coreType){
        for(coreFreq = 0; coreFreq < SIZE_COREFREQ; ++coreFreq){
            if(vitamins_arch_freq_available(coreType,coreFreq)){
            	uint32_t tgtIPC,tgtPower,tgtIPSactive,tgtTLC;
            	bin_pred_result_t *result = vitamins_bin_predict(task,currCoreType,currCoreFreq,coreType,coreFreq,bin_predictors_ipcpower);

            	//predict the active IPC and power
            	tgtIPC = result->ipcActive;

				if(coreType == COREARCH_Exynos5422_BIG){
					tgtPower = arch_idle_power_scaled(coreType, coreFreq) * 8;
				}
				else if(coreType == COREARCH_Exynos5422_LITTLE){
					tgtPower = arch_idle_power_scaled(coreType, coreFreq) * 5;
				}
				else BUG_ON(true);
				BUG_ON(tgtPower <= arch_idle_power_scaled(coreType, coreFreq));

                //IPS active
                tgtIPSactive = tgtIPC * freqToValMHz_i(coreFreq);
                tgtIPSactive = CONV_scaledINTany_INTany(tgtIPSactive);//ips is not scaled

                //Predict TLC
                tgtTLC = vitamins_predict_tlc(task,avgIPS,avgTLC,tgtIPSactive);
                //correct
                if(tgtTLC > MAX_ALLOWED_TLC) tgtTLC = MAX_ALLOWED_TLC;
                if(tgtTLC < MIN_ALLOWED_TLC) tgtTLC = MIN_ALLOWED_TLC;

                //task->ipc_active[coreType] = tgtIPC;
                task->ips_active[coreType][coreFreq] = tgtIPSactive;
                task->tlc[coreType][coreFreq] = tgtTLC;
                task->power_active[coreType][coreFreq] = tgtPower;
            }
        }
    }
}
*/


void vitamins_bin_predictor(model_sys_t *sys)
{
	int task;
	vitamins_bin_predictor_calc_error(sys);
	for(task = 0; task < sys->task_list_size; ++task)
		vitamins_bin_predict_for_task(sys->task_list[task]);
}

/*
void vitamins_dumb_exynos_predictor(vitamins_sys_t *sys)
{
	int task;
	vitamins_bin_predictor_calc_error(sys);
	for(task = 0; task < sys->task_list_size; ++task)
		vitamins_dumb_exynos_predict_for_task(sys->task_list[task]);
}

void vitamins_halfdumb_exynos_predictor(vitamins_sys_t *sys)
{
	int task;
	vitamins_bin_predictor_calc_error(sys);
	for(task = 0; task < sys->task_list_size; ++task)
		vitamins_halfdumb_exynos_predict_for_task(sys->task_list[task]);
}
*/

