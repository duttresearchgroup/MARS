//#define HASDEBUG
#include "core.h"

//load above this is considered full load and is not scaled by the load predictor
static const uint32_t FULL_LOAD = CONV_DOUBLE_scaledUINT32(0.98);

static load_tracker_type_t cur_lt = LT_DEFAULT;

void vitamins_load_tracker_set(load_tracker_type_t t)
{
	cur_lt = t;
}

load_tracker_type_t vitamins_load_tracker_get(void){
	return cur_lt;
}

uint32_t
vitamins_predict_tlc(model_task_t* task,
                  uint32_t srcCoreIPS, uint32_t srcCoreTLC,
                  uint32_t tgtCoreIPS)
{
    //use the demand to create a virtual active and sleep part
    //scale the active part according to performance ration between cores
    //recalculate the utilization
    uint32_t predTLC, activePart, sleepPart;

    //if the demand is 1.0 sharp, we can skip some computation
    if(srcCoreTLC >= FULL_LOAD) return srcCoreTLC;

    predTLC = 0;
    activePart = srcCoreTLC;
    sleepPart = CONV_INTany_scaledINTany(1)-srcCoreTLC;

    activePart *= srcCoreIPS;
    activePart /= tgtCoreIPS;

    predTLC = CONV_INTany_scaledINTany(activePart) / (activePart+sleepPart);

    return predTLC;
}


uint32_t
vitamins_estimate_tlc(model_task_t* task){

    uint32_t tlc = 0;
    BUG_ON(task_curr_core(task) == nullptr);
    BUG_ON(task_curr_core(task)->sensed_data.num_tasks < 1);

    if(task_curr_core(task)->sensed_data.num_tasks==1)
        tlc = task->sensed_data.proc_time_share;
    else {
        //estimate based on context switch counters
        tlc = CONV_INTany_scaledINTany(task->sensed_data.nivcsw) / (task->sensed_data.nivcsw + task->sensed_data.nvcsw);

        //it cannot be smaller than the amount of cpu it has used
        if(tlc < task->sensed_data.proc_time_share)
            tlc = task->sensed_data.proc_time_share;
    }

    //ajust in case of extreme error; minimum allowable is 0.1%
    if(tlc > CONV_INTany_scaledINTany(1))
        tlc = CONV_INTany_scaledINTany(1);
    if(tlc <  CONV_DOUBLE_scaledINT64(0.001))
        tlc = CONV_DOUBLE_scaledINT64(0.001);

    return tlc;
}

//TODO returns magic number
//must be tweaked per platform
static inline
uint32_t
_load_sched_ovh(uint32_t num_tasks){
#ifndef __KERNEL__
	if(num_tasks > 1)
		return CONV_DOUBLE_scaledUINT32(0.03) * num_tasks;
	else
		return 0;
#else
	return 0;
#endif
}


//RUN-DMC model of CFS scheduler

static
void
_vitamins_cfs_estimator_do_update(cfs_load_estimator_t *cfs, core_freq_t freq)
{
    model_task_t *task;

    cfs->equal_share = 0;
    cfs->residual_share = 0;
    cfs->common.task_cnt = 0;
    cfs->app_tlc_sum = 0;
    cfs->total_tlc_sum = 0;
    cfs->tlc_sum_residual = 0;

    BUG_ON(freq == SIZE_COREFREQ);

    for_each_in_internal_list(cfs->common.core,mapped_tasks,task,mapping){
    	BUG_ON(freq == COREFREQ_0000MHz);//invalid if some task is mapped
        cfs->app_tlc_sum += task->tlc[cfs->common.core->info->arch][freq];
        cfs->common.task_cnt += 1;
    }
    cfs->total_tlc_sum = cfs->app_tlc_sum;

    BUG_ON(cfs->common.task_cnt < 0);
    BUG_ON((cfs->common.task_cnt == 0) && (cfs->app_tlc_sum > 0));
    BUG_ON((cfs->common.task_cnt != 0) && (cfs->app_tlc_sum == 0));

    //system load
    if(freq == COREFREQ_0000MHz){
    	//if the core is power gated, so the system load is always 0.
    	//we check the load at the maximum and minimum freq. to make sure this is true
    	BUG_ON(cfs->common.core->systask->tlc[cfs->common.core->info->arch][vitamins_dvfs_get_minimum_freq(cfs->common.core)] != 0);
    	BUG_ON(cfs->common.core->systask->tlc[cfs->common.core->info->arch][vitamins_dvfs_get_maximum_freq(cfs->common.core)] != 0);
    }
    else{
    	//else we add up the system load
    	BUG_ON(cfs->common.core->systask->tlc[cfs->common.core->info->arch][freq] == INVALID_METRIC_VAL);
    	cfs->total_tlc_sum += cfs->common.core->systask->tlc[cfs->common.core->info->arch][freq];
    }

    if(cfs->common.task_cnt > 0)
        cfs->equal_share =
        		(CONV_INTany_scaledINTany(1) - cfs->common.core->systask->tlc[cfs->common.core->info->arch][freq])
        	    / cfs->common.task_cnt;

    if((cfs->total_tlc_sum >= CONV_INTany_scaledINTany(1)) && (cfs->common.task_cnt > 0)){
    	for_each_in_internal_list(cfs->common.core,mapped_tasks,task,mapping){
    		if(task->tlc[cfs->common.core->info->arch][freq] <= cfs->equal_share)
    			cfs->residual_share += cfs->equal_share - task->tlc[cfs->common.core->info->arch][freq];
    		else
    			cfs->tlc_sum_residual += task->tlc[cfs->common.core->info->arch][freq];
        }
    }

    if(cfs->total_tlc_sum < CONV_INTany_scaledINTany(1)){
    	cfs->common.load = cfs->total_tlc_sum;
    	cfs->common.task_load_overhead = CONV_INTany_scaledINTany(1) - _load_sched_ovh(cfs->common.task_cnt);
    }
    else{
    	cfs->common.load = CONV_INTany_scaledINTany(1);
    	cfs->common.task_load_overhead = CONV_INTany_scaledINTany(1);
    }

    //used for aging-aware mapping
    cfs->aged_load = cfs->common.load + cfs->common.core->aging_info.rel_delay_degrad_penalty;
}


static
uint32_t
_vitamins_cfs_estimator_task_load(model_task_t *task, core_freq_t freq)
{
    cfs_load_estimator_t *cfs;
    uint32_t task_load;
    BUG_ON(task_next_core(task) == nullptr);
    cfs = &(task_next_core(task)->load_tracking.cfs);
    BUG_ON(cfs == nullptr);

    BUG_ON(task_next_core(task) != cfs->common.core);

    BUG_ON(freq == COREFREQ_0000MHz);
    BUG_ON(freq == SIZE_COREFREQ);

    if((cfs->total_tlc_sum < CONV_INTany_scaledINTany(1)) || (task->tlc[cfs->common.core->info->arch][freq] <= cfs->equal_share)){
        task_load = task->tlc[cfs->common.core->info->arch][freq];
    }
    else{
    	uint32_t residual;
    	BUG_ON(cfs->tlc_sum_residual == 0);
    	BUG_ON(cfs->equal_share == 0);
    	BUG_ON(cfs->tlc_sum_residual > cfs->total_tlc_sum);
    	BUG_ON(cfs->residual_share >= CONV_INTany_scaledINTany(1));

    	residual = ((task->tlc[cfs->common.core->info->arch][freq]*cfs->residual_share)/cfs->tlc_sum_residual);

    	task_load = cfs->equal_share + residual;

    	cfs->tlc_sum_residual -= task->tlc[cfs->common.core->info->arch][freq];
    	cfs->residual_share -= residual;

    	BUG_ON(task_load > CONV_INTany_scaledINTany(1));
    	BUG_ON(task_load < cfs->equal_share);
    	BUG_ON(cfs->tlc_sum_residual > cfs->total_tlc_sum);
    	BUG_ON(cfs->residual_share >= CONV_INTany_scaledINTany(1));

    	if(task_load > task->tlc[cfs->common.core->info->arch][freq]){
    		//return to the pool the load I'm not using
    		cfs->residual_share += task_load - task->tlc[cfs->common.core->info->arch][freq];
    		task_load = task->tlc[cfs->common.core->info->arch][freq];
    	}
    }
    BUG_ON(task_load > CONV_INTany_scaledINTany(1));
    BUG_ON(task_load == 0);
    BUG_ON(task_load > task->tlc[cfs->common.core->info->arch][freq]);

    return task_load;
}

//Model for the round robin schedule:
//TODO why this work better for Linsched ?
/*static
uint32_t
_vitamins_cfs_estimator_task_load(vitamins_task_t *task, vitamins_core_freq_t freq)
{
    cfs_load_estimator_t *cfs;
    uint32_t task_load;
    BUG_ON(task_next_core(task) == nullptr);
    cfs = &(task_next_core(task)->load_tracking.cfs);
    BUG_ON(cfs == nullptr);

    BUG_ON(task_next_core(task) != cfs->common.core);

    BUG_ON(freq == COREFREQ_0000MHz);
    BUG_ON(freq == SIZE_COREFREQ);

    if((cfs->total_tlc_sum < CONV_INTany_scaledINTany(1))){
        task_load = task->tlc[cfs->common.core->info->arch][freq];
    }
    else{
        task_load = CONV_INTany_scaledINTany(task->tlc[cfs->common.core->info->arch][freq]) / cfs->app_tlc_sum;
        task_load *= CONV_INTany_scaledINTany(1) - (cfs->total_tlc_sum - cfs->app_tlc_sum);
        task_load = CONV_scaledINTany_INTany(task_load);
    }
    BUG_ON(task_load > CONV_INTany_scaledINTany(1));

    return task_load;
}*/

static inline
void
vitamins_cfs_estimator_update(model_core_t *core, core_freq_t freq)
{
	model_task_t *task;

	_vitamins_cfs_estimator_do_update(&(core->load_tracking.cfs),freq);

	for_each_in_internal_list(core,mapped_tasks,task,mapping){
    	task->_next_mapping_load = _vitamins_cfs_estimator_task_load(task,freq);
    	BUG_ON(task->_next_mapping_load > task->tlc[core->info->arch][freq]);
    	BUG_ON(task->_next_mapping_ != core);
    	BUG_ON(task_next_core_pred_freq(task) != freq);
    }
}

static
void
_vitamins_lt_default_estimator_do_update(model_core_t *core, core_freq_t freq, bool agingaware)
{
    model_task_t *task;

    default_load_estimator_t *ltd = &(core->load_tracking.defaut);


    ltd->common.task_cnt = 0;
    ltd->common.load = 0;

    for_each_in_internal_list(core,mapped_tasks,task,mapping){
    	ltd->common.load += task->sensed_data.proc_time_share_avg;
        ltd->common.task_cnt += 1;
        //note: load acc for system task is used only in the cfs tracker
    }

    ltd->app_load_acc = ltd->common.load;
    ltd->total_load_acc = ltd->common.load;

    if(agingaware) ltd->common.load += CONV_INTany_scaledINTany(1) - core->aging_info.rel_delay_degrad_penalty;

    if(ltd->common.load > CONV_INTany_scaledINTany(1)) ltd->common.load = CONV_INTany_scaledINTany(1);

    ltd->common.task_load_overhead = CONV_INTany_scaledINTany(1);

}

static inline
uint32_t
_vitamins_lt_default_estimator_task_load(model_task_t *task, core_freq_t freq, bool agingaware)
{
	BUG_ON(task_next_core(task)->load_tracking.defaut.total_load_acc == 0);
	return CONV_INTany_scaledINTany(task->sensed_data.proc_time_share_avg) / task_next_core(task)->load_tracking.defaut.total_load_acc;
}


static inline
void
vitamins_lt_default_estimator_update(model_core_t *core, core_freq_t freq, bool agingaware)
{
	model_task_t *task;

	_vitamins_lt_default_estimator_do_update(core,freq,agingaware);

	for_each_in_internal_list(core,mapped_tasks,task,mapping){
    	task->_next_mapping_load = _vitamins_lt_default_estimator_task_load(task,freq,agingaware);
    }
}

static inline
void
_vitamins_load_tracker_map_changed_update_core_load(load_tracker_type_t t, model_core_t *core,core_freq_t freq)
{
    switch (t) {
        case LT_CFS:
            vitamins_cfs_estimator_update(core,freq);
            break;
        case LT_DEFAULT:
            vitamins_lt_default_estimator_update(core,freq,false);
            break;
        case LT_DEFAULT_AGINGAWARE:
            vitamins_lt_default_estimator_update(core,freq,true);
            break;
        default:
            BUG_ON("Invalid load tracker");
            break;
    }
}

static inline
void
_vitamins_load_tracker_map_changed_update_load(load_tracker_type_t t, model_core_t *ref_core,core_freq_t freq)
{
	//recomputes the whole domain load
	core_info_t *core;
	for_each_in_internal_list(ref_core->info->freq,cores,core,freq_domain){
		_vitamins_load_tracker_map_changed_update_core_load(t,core->this_core,freq);
	}
}


void
vitamins_load_tracker_map_changed(model_core_t *core)
{
	uint32_t iter;
	uint32_t iter_max = vitamins_dvfs_get_dvfs_to_map_ratio();
	//start with the maximim freq and predict load
	core_freq_t freq =  vitamins_dvfs_predict_next_freq_initial(core);
	core_freq_t freq_prev = SIZE_COREFREQ;
	_vitamins_load_tracker_map_changed_update_load(cur_lt,core,freq);
	//then predict frequency based on the predicted load, and predict load again based on the new frequency
	//repeat until stable
	BUG_ON(iter_max<1);
	for(iter = 0; (freq != freq_prev) && (iter <= iter_max); ++iter) {
		freq_prev = freq;
		freq =  vitamins_dvfs_predict_next_freq(core);
		_vitamins_load_tracker_map_changed_update_load(cur_lt,core,freq);
	}
}

uint32_t vitamins_load_tracker_task_load(model_task_t *task)
{
    BUG_ON(task->_next_mapping_load == INVALID_METRIC_VAL);
	return task->_next_mapping_load;
}

uint32_t vitamins_load_tracker_task_load_with_overhead(model_task_t *task)
{
    uint32_t load = task->_next_mapping_load;
    BUG_ON(task->_next_mapping_load == INVALID_METRIC_VAL);
    load *= task_next_core(task)->load_tracking.common.task_load_overhead;
    return CONV_scaledINTany_INTany(load);
}
