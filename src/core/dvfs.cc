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

#include "dvfs.h"
#include "idlepower.h"
#include "helpers.h"
#include "power.h"

static dvfs_algorithm_t VITAMINS_GLOBAL_DVFS = DVFS_DEFAULT;

const uint32_t DVFS_ONDEMAND_UP_THR = CONV_DOUBLE_scaledUINT32(0.90);
const uint32_t DVFS_ONDEMAND_DOWN_THR = CONV_DOUBLE_scaledUINT32(0.40);

static uint32_t dvfs_epoch_us = 50;
static uint32_t map_epoch_us = 200;

const int CHIP_SZ_CORES = 4;
static bool _wave_up[CHIP_SZ_CORES] = {true};
static uint32_t _repeat[CHIP_SZ_CORES] = {0};
static core_freq_t _last_freq[CHIP_SZ_CORES] = {SIZE_COREFREQ};

core_freq_t vitamins_dvfs_get_freq(model_core_t *core)
{
	BUG_ON(core->info->freq->this_domain == nullptr);
    return core->info->freq->this_domain->freq;
}

static inline core_freq_t maximum_possible_freq(core_arch_t arch)
{
    int curr;
    for(curr = 0; curr < SIZE_COREFREQ; ++curr)
        if(vitamins_arch_freq_available(arch,(core_freq_t)curr)) return (core_freq_t)curr;
    BUG_ON(true);
    return SIZE_COREFREQ;
}
static inline core_freq_t maximum_possible_freq_agingaware(model_core_t *core)
{
    int curr;
    for(curr = 0; curr < SIZE_COREFREQ; ++curr)
        if(vitamins_arch_freq_available(core->info->arch,(core_freq_t)curr)){
            if(freqToPeriodPS_i((core_freq_t)curr) > core->aging_info.current_delay_ps)
                return (core_freq_t)curr;
        }
    BUG_ON(true);
    return SIZE_COREFREQ;
}
static inline core_freq_t minimum_possible_freq(core_arch_t arch)
{
    int curr;
    for(curr = SIZE_COREFREQ-1; curr >= 0; --curr)
        if(vitamins_arch_freq_available(arch,(core_freq_t)curr)) return (core_freq_t)curr;
    BUG_ON(true);
    return SIZE_COREFREQ;
}


#define DVFS_FUNC_NAME(policy,mode) vitamins_dvfs_##mode##_next_freq_##policy
#define DVFS_DECLARE_PREDICT_FUNC(policy) static core_freq_t DVFS_FUNC_NAME(policy,predict)(model_core_t *c)
#define DVFS_DECLARE_COMPUTE_FUNC(policy) static core_freq_t DVFS_FUNC_NAME(policy,compute)(model_core_t *c)

#define DVFS_CHOICE_BEGIN(var)  switch (VITAMINS_GLOBAL_DVFS) {
#define DVFS_CHOICE_END \
        default: break;\
    }\
    BUG_ON(true);\
    return (core_freq_t)0;

#define DVFS_CHOSE_PREDICT(policy,core) \
case policy: core->info->freq->this_domain->last_pred_freq = DVFS_FUNC_NAME(policy,predict)(core); return core->info->freq->this_domain->last_pred_freq;

#define DVFS_CHOSE_COMPUTE(policy,core) \
case policy: return DVFS_FUNC_NAME(policy,compute)(core)

core_freq_t vitamins_dvfs_get_maximum_freq(model_core_t *core)
{
    DVFS_CHOICE_BEGIN(VITAMINS_GLOBAL_DVFS)
        case DVFS_DEFAULT:
        case DVFS_MANUAL:
        case DVFS_ONDEMAND:
        case DVFS_ONDEMAND_PG:
        case DVFS_CTRL:
            return maximum_possible_freq(core->info->arch);
        case DVFS_WAVE:
            return maximum_possible_freq(core->info->arch);
        case DVFS_ONDEMAND_PG_AGINGAWARE:
            return maximum_possible_freq_agingaware(core);
    DVFS_CHOICE_END
}

core_freq_t vitamins_dvfs_get_minimum_freq(model_core_t *core)
{
    DVFS_CHOICE_BEGIN(VITAMINS_GLOBAL_DVFS)
        case DVFS_DEFAULT:
        case DVFS_MANUAL:
        case DVFS_ONDEMAND:
        case DVFS_ONDEMAND_PG:
        case DVFS_ONDEMAND_PG_AGINGAWARE:
        case DVFS_CTRL:
            return minimum_possible_freq(core->info->arch);
        case DVFS_WAVE:
            return minimum_possible_freq(core->info->arch);
    DVFS_CHOICE_END
}

/*
static inline core_freq_t ondemand_search_up_freq(vitamins_core_arch_t arch, core_freq_t start)
{
    int curr = (int)start - 1;
    while(curr >= 0) {
        if(vitamins_arch_freq_available_with_pred(arch,(core_freq_t)curr)) return (core_freq_t)curr;
        curr -= 1;
    }
    return start;
}
*/
static inline core_freq_t ondemand_search_down_freq(core_arch_t arch, core_freq_t start)
{
    core_freq_t curr = (core_freq_t)(start+1);
    BUG_ON(start >= SIZE_COREFREQ);
    while(curr < SIZE_COREFREQ) {
        if(vitamins_arch_freq_available(arch,curr)) return curr;
        curr = (core_freq_t)(curr + 1);
    }
    return start;
}

static inline uint32_t ondemand_predicted_load(model_core_t *core)
{
    return core->load_tracking.common.load;
}

static inline uint32_t ondemand_dynamic_load(model_core_t *core)
{
    uint32_t activeCySum = core->sensed_data.last_dvfs_epoch_sumCyclesActive;
    uint32_t load = 0;
    core_freq_t last_dvfs_epoch_freq = core->sensed_data.last_dvfs_epoch_freq;

    BUG_ON(core->info->freq->this_domain == 0);

    if(core->info->freq->this_domain->freq == COREFREQ_0000MHz) return 0;//its power-gated
    if(activeCySum == 0) return 0;

    load = CONV_INTany_scaledINTany(activeCySum / freqToValMHz_i(last_dvfs_epoch_freq)) / dvfs_epoch_us;

    //offline simulator won't be 100% accurate so the load  calculated here might be a bit above 1.0
    BUG_ON(load > CONV_DOUBLE_scaledUINT32(1.05));
    if(load > CONV_DOUBLE_scaledUINT32(1.0)) load = CONV_DOUBLE_scaledUINT32(1.0);

    return load;
}

static core_freq_t _vitamins_dvfs_predict_next_freq_DVFS_CTRL(model_core_t *c) {
	return c->info->freq->this_domain->freq;
}

static core_freq_t vitamins_dvfs_next_freq_DVFS_CTRL(model_core_t *c) {
	if (_repeat[c->info->position % CHIP_SZ_CORES] > 0) _repeat[c->info->position % CHIP_SZ_CORES]--;
	else {
		c->info->freq->this_domain->ctrl_freq.referenceOutput(0.5);
	}
	//uint64_t last_ips = c->sensed_data.last_dvfs_epoch_ips;
	//double l_ips = (double)last_ips/1000;
	double last_p = c->sensed_data.last_dvfs_epoch_avg_power;

	double n_freq = c->info->freq->this_domain->ctrl_freq.nextInputVal(last_p);
	//printf("LAST ERR %f REF OUT %f\n", c->info->freq->this_domain->ctrl_freq.lastError(), c->info->freq->this_domain->ctrl_freq.referenceOutput());
	n_freq *= 1000;
//	uint32_t next_freq = n_freq*1000;
	//printf("LAST IPS %lu (%f) LAST P %f NEXT FREQ (%f)\n", last_ips, l_ips, last_p, n_freq);
	return valToFreqMHz_d(n_freq);
}

static core_freq_t _vitamins_dvfs_predict_next_freq_DVFS_WAVE(model_core_t *c) {
	if(c->info->freq->this_domain->freq == COREFREQ_0000MHz){
		return vitamins_dvfs_get_minimum_freq(c);
	}
	return c->info->freq->this_domain->freq;
}

static core_freq_t vitamins_dvfs_next_freq_DVFS_WAVE(model_core_t *c) {
//	core_freq_t last_freq = c->info->freq->this_domain->freq;
	core_freq_t last_freq = _last_freq[c->info->position % CHIP_SZ_CORES];

	if (c->__vitaminslist_head_mapped_tasks == nullptr){
			return COREFREQ_0000MHz;
	}
	if (last_freq < vitamins_dvfs_get_maximum_freq(c) || last_freq > vitamins_dvfs_get_minimum_freq(c)) {
		return vitamins_dvfs_get_maximum_freq(c);
	}
//	printf("LAST FREQ %s\n", freqToString(last_freq));
//	if (_repeat == 0) {
	if (_repeat[c->info->position % CHIP_SZ_CORES] % (((c->info->position % CHIP_SZ_CORES) + 1)*6) == 0) {
		if(last_freq == vitamins_dvfs_get_minimum_freq(c)) {
			_wave_up[c->info->position % CHIP_SZ_CORES] = true;
			return (core_freq_t)(last_freq-1);
		} else if(last_freq == vitamins_dvfs_get_maximum_freq(c)) {
			_wave_up[c->info->position % CHIP_SZ_CORES] = false;
			return (core_freq_t)(last_freq+1);
		} else {
			return _wave_up[c->info->position % CHIP_SZ_CORES] ? (core_freq_t)(last_freq-1) : (core_freq_t)(last_freq+1);
		}
	} else {
		return last_freq;
	}
}

static core_freq_t vitamins_dvfs_next_freq_DVFS_ONDEMAND(model_core_t *c, bool pred, bool power_gated)
{
    /*
     * If any core in the domain have load >= up_threashold then bring freq to max
     */
    core_info_t *core;
    uint32_t core_load = 0;
    uint32_t domain_load_acc = 0;

    core_freq_t last_freq = pred ? c->info->freq->this_domain->last_pred_freq : c->info->freq->this_domain->freq;

    for_each_in_internal_list(c->info->freq,cores,core,freq_domain){
        core_load = pred ? ondemand_predicted_load(core->this_core) : ondemand_dynamic_load(core->this_core);
        domain_load_acc += core_load;
        if(core_load >= DVFS_ONDEMAND_UP_THR)
            return vitamins_dvfs_get_maximum_freq(core->this_core);
    }

    /*
     * Returns min frequency if the domain is empty
     */
    if(domain_load_acc == 0) {
        return power_gated ? COREFREQ_0000MHz : vitamins_dvfs_get_minimum_freq(c);
    }

    /*
     * If it was clock gated before then start with the maximum frequency
     */
    if(last_freq == COREFREQ_0000MHz){
        BUG_ON(!power_gated);
        return vitamins_dvfs_get_minimum_freq(c);
    }

    /*
     * If no core above the up_threashold and at least one core between up_threashhold and down_threashold then keep the same
     */
    for_each_in_internal_list(c->info->freq,cores,core,freq_domain){
        core_load = pred ? ondemand_predicted_load(core->this_core) : ondemand_dynamic_load(core->this_core);
        if(core_load > DVFS_ONDEMAND_DOWN_THR)
            return last_freq;
    }
    /*
     * All cores in the domain have load <= down_threashold, so scale down
     */
    return ondemand_search_down_freq(c->info->arch,last_freq);
}


DVFS_DECLARE_PREDICT_FUNC(DVFS_DEFAULT)
{
    core_info_t *core;
    for_each_in_internal_list(c->info->freq,cores,core,freq_domain){
        if(core->this_core->load_tracking.common.load > 0) return maximum_possible_freq(core->arch);
    }
    return minimum_possible_freq(c->info->arch);
}
DVFS_DECLARE_COMPUTE_FUNC(DVFS_DEFAULT)
{
    core_info_t *core;
    for_each_in_internal_list(c->info->freq,cores,core,freq_domain){
    	if(core->this_core->sensed_data.last_dvfs_epoch_sumCyclesActive > 0) return maximum_possible_freq(core->arch);
    }
    return minimum_possible_freq(c->info->arch);
}

DVFS_DECLARE_PREDICT_FUNC(DVFS_MANUAL)
{
    return c->info->freq->this_domain->manual_freq;
}
DVFS_DECLARE_COMPUTE_FUNC(DVFS_MANUAL)
{
    return c->info->freq->this_domain->manual_freq;
}

DVFS_DECLARE_PREDICT_FUNC(DVFS_ONDEMAND)
{
    return vitamins_dvfs_next_freq_DVFS_ONDEMAND(c,true,false);
}
DVFS_DECLARE_COMPUTE_FUNC(DVFS_ONDEMAND)
{
    return vitamins_dvfs_next_freq_DVFS_ONDEMAND(c,false,false);
}

DVFS_DECLARE_PREDICT_FUNC(DVFS_ONDEMAND_PG)
{
    return vitamins_dvfs_next_freq_DVFS_ONDEMAND(c,true,true);
}
DVFS_DECLARE_COMPUTE_FUNC(DVFS_ONDEMAND_PG)
{
    return vitamins_dvfs_next_freq_DVFS_ONDEMAND(c,false,true);
}

DVFS_DECLARE_PREDICT_FUNC(DVFS_ONDEMAND_PG_AGINGAWARE)
{
    return vitamins_dvfs_next_freq_DVFS_ONDEMAND(c,true,true);
}
DVFS_DECLARE_COMPUTE_FUNC(DVFS_ONDEMAND_PG_AGINGAWARE)
{
    return vitamins_dvfs_next_freq_DVFS_ONDEMAND(c,false,true);
}


DVFS_DECLARE_PREDICT_FUNC(DVFS_WAVE)
{
    return _vitamins_dvfs_predict_next_freq_DVFS_WAVE(c);
}
DVFS_DECLARE_COMPUTE_FUNC(DVFS_WAVE)
{
    return vitamins_dvfs_next_freq_DVFS_WAVE(c);
}

DVFS_DECLARE_PREDICT_FUNC(DVFS_CTRL)
{
    return _vitamins_dvfs_predict_next_freq_DVFS_CTRL(c);
}
DVFS_DECLARE_COMPUTE_FUNC(DVFS_CTRL)
{
    return vitamins_dvfs_next_freq_DVFS_CTRL(c);
}

core_freq_t vitamins_dvfs_predict_next_freq_initial(model_core_t *core)
{
	core->info->freq->this_domain->last_pred_freq = vitamins_dvfs_get_maximum_freq(core);
	return core->info->freq->this_domain->last_pred_freq;
}
core_freq_t vitamins_dvfs_predict_next_freq(model_core_t *core)
{
    DVFS_CHOICE_BEGIN(VITAMINS_GLOBAL_DVFS)
        DVFS_CHOSE_PREDICT(DVFS_DEFAULT,core);
        DVFS_CHOSE_PREDICT(DVFS_MANUAL,core);
        DVFS_CHOSE_PREDICT(DVFS_ONDEMAND,core);
        DVFS_CHOSE_PREDICT(DVFS_ONDEMAND_PG,core);
        DVFS_CHOSE_PREDICT(DVFS_ONDEMAND_PG_AGINGAWARE,core);
        DVFS_CHOSE_PREDICT(DVFS_WAVE,core);
        DVFS_CHOSE_PREDICT(DVFS_CTRL,core);
    DVFS_CHOICE_END
}

core_freq_t vitamins_dvfs_compute_next_freq(model_core_t *core)
{
    DVFS_CHOICE_BEGIN(VITAMINS_GLOBAL_DVFS)
        DVFS_CHOSE_COMPUTE(DVFS_DEFAULT,core);
        DVFS_CHOSE_COMPUTE(DVFS_MANUAL,core);
        DVFS_CHOSE_COMPUTE(DVFS_ONDEMAND,core);
        DVFS_CHOSE_COMPUTE(DVFS_ONDEMAND_PG,core);
        DVFS_CHOSE_COMPUTE(DVFS_ONDEMAND_PG_AGINGAWARE,core);
        DVFS_CHOSE_COMPUTE(DVFS_WAVE,core);
        DVFS_CHOSE_COMPUTE(DVFS_CTRL,core);
    DVFS_CHOICE_END
}


static inline void vitamins_dvfs_set_freq_phy(model_core_t *core,core_freq_t next)
{
    if((core->info->freq->this_domain->freq != next) && (core->info->freq->set_freq_callback != nullptr)){
        BUG_ON(!(*(core->info->freq->set_freq_callback))(core->info->freq->this_domain,next));
    }
}

static inline void _set_freq(model_core_t *core, core_freq_t freq)
{
    vitamins_dvfs_set_freq_phy(core,freq);
    core->info->freq->this_domain->freq = freq;
}

void vitamins_dvfs_set_predicted_freq(model_core_t *core)
{
    _set_freq(core, core->info->freq->this_domain->last_pred_freq);
}

void vitamins_dvfs_set_freq(model_core_t *core)
{
    core_freq_t next = vitamins_dvfs_compute_next_freq(core);
//	core_freq_t last_freq = core->info->freq->this_domain->freq;

	_set_freq(core, next);

	if (next != COREFREQ_0000MHz) {
//		if (_repeat == 0) {
//			_repeat = 16;
//			if(next - last_freq > 0) {
//				_wave_up = false;
//			} else if(next - last_freq < 0) {
//				_wave_up = true;
//			}
//		} else {
//			_repeat--;
//		}
		_repeat[core->info->position % CHIP_SZ_CORES]++;

		_last_freq[core->info->position % CHIP_SZ_CORES] = next;
	}
}

bool vitamins_dvfs_is_power_gated(model_core_t *core)
{
    return core->info->freq->this_domain->freq == COREFREQ_0000MHz;
}

void vitamins_dvfs_wakeup_core(model_core_t *core)
{
    BUG_ON(core->info->freq->this_domain->freq != COREFREQ_0000MHz);
    _set_freq(core,vitamins_dvfs_get_minimum_freq(core));
}

static uint32_t vitamins_freq_to_mVolt_map_GEM5(core_freq_t freq)
{
    switch (freq) {
    case COREFREQ_3000MHZ: return 1300;
    case COREFREQ_2000MHZ: return 1000;
    case COREFREQ_1500MHZ: return  800;
    case COREFREQ_1000MHZ: return  700;
    case COREFREQ_0500MHZ: return  600;
    default:
        BUG_ON("Invalid frequency");
        return 0;
    }
    return 0;
}


uint32_t vitamins_freq_to_mVolt_map(core_arch_t arch, core_freq_t freq)
{
    switch (arch) {
    case COREARCH_GEM5_BIG_BIG:
    case COREARCH_GEM5_BIG_LITTLE:
    case COREARCH_GEM5_LITTLE_BIG:
    case COREARCH_GEM5_LITTLE_LITTLE:
        return vitamins_freq_to_mVolt_map_GEM5(freq);
    default:
        BUG_ON("Architecture not supported");
        return 0;
    }
    return 0;
}


void vitamins_dvfs_manual_freq(model_core_t *core,core_freq_t freq)
{
    BUG_ON(!vitamins_arch_freq_available(core->info->arch,freq));
    BUG_ON(VITAMINS_GLOBAL_DVFS != DVFS_MANUAL);
    core->info->freq->this_domain->manual_freq = freq;

}

void vitamins_dvfs_set_dvfs_epoch(uint32_t _dvfs_epoch_us)
{
    dvfs_epoch_us = _dvfs_epoch_us;
}
void vitamins_dvfs_set_map_epoch(uint32_t _map_epoch_us)
{
    map_epoch_us = _map_epoch_us;
}
uint32_t vitamins_dvfs_get_dvfs_to_map_ratio(void)
{
    BUG_ON(dvfs_epoch_us >= map_epoch_us);
    return map_epoch_us / dvfs_epoch_us;
}

void vitamins_dvfs_set_global_policy(dvfs_algorithm_t policy)
{
    VITAMINS_GLOBAL_DVFS = policy;
}

dvfs_algorithm_t vitamins_dvfs_get_global_policy(void)
{
    return VITAMINS_GLOBAL_DVFS;
}
