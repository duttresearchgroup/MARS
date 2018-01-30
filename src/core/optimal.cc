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
#include "cfs.h"

static model_core_t* bestmapping[MAX_NUM_TASKS];
static core_freq_t bestfreq[MAX_NUM_CORES];

static uint32_t bestmapping_ipswatt;
static uint32_t bestmapping_ips;
static uint32_t bestmapping_ips_upthr;
static uint32_t bestmapping_ips_downthr;
static uint32_t bestmapping_ips_tgt;

static uint32_t maxPerm = 0;
static uint32_t currPerm = 0;

static
void
calc_ips_watt(uint32_t *ret_ips, uint32_t *ret_ips_watt,
		model_sys_t *sys)
{
    *ret_ips = system_total_ips(sys);
    *ret_ips_watt = CONV_INTany_scaledINTany(*ret_ips) / system_total_power(sys);
}

static uint32_t _fact(uint32_t n,uint32_t nstop){
    if(n == nstop) return 1;
    else       return n*_fact(n-1,nstop);
}
static uint32_t _pow(uint32_t x, uint32_t n){
    uint32_t i;
    uint32_t result = 1;
    for(i = 0; i < n; ++i) result *= x;
    return result;
}
static inline uint32_t count_permutations(uint32_t ntasks, uint32_t ncores, bool share_cores)
{
    if(share_cores)
        return _pow(ncores,ntasks);
    else
        return _fact(ncores,ncores-ntasks); //does fact(ncores) / fact(ncores-ntasks)
}

typedef bool (objective_eval_func)(uint32_t ips, uint32_t ipswatt);
typedef void (objective_update_func)(uint32_t ips, uint32_t ipswatt);


inline static
void
eval_one_mapping(
        model_sys_t *sys,
        bool explore_freq, objective_eval_func obj_eval, objective_update_func obj_update)
{
    int core,task;
    uint32_t ips,ipswatt;
    calc_ips_watt(&ips,&ipswatt,sys);
    if((obj_eval)(ips,ipswatt)){
        (obj_update)(ips,ipswatt);
        //save mapping
        for(task = 0; task < sys->task_list_size; ++task) {
            BUG_ON(task_next_core(sys->task_list[task]) == nullptr);
            bestmapping[task] = task_next_core(sys->task_list[task]);
        }
        //save freq
        if(explore_freq){
            for(core = 0; core < sys->info->core_list_size; ++core){
                bestfreq[core] = sys->info->core_list[core].freq->this_domain->manual_freq;
            }
        }
    }
}

static
void
explore_all_mappings(
        int currTask,
        model_sys_t *sys,
        const bool share_cores, const bool explore_freq,
        objective_eval_func obj_eval, objective_update_func obj_update)
{
    int core;

    if(currTask < sys->task_list_size){
        //try to map the curr task to all cores
        for(core = 0; core < sys->info->core_list_size; ++core){
            if(share_cores || (sys->info->core_list[core].this_core->load_tracking.common.task_cnt == 0)){
                //map and go to the next task
                BUG_ON(task_next_core(sys->task_list[currTask]) != nullptr);
                task_next_core_map(sys->task_list[currTask],sys->info->core_list[core].this_core);

                explore_all_mappings(
                        currTask+1,
                        sys,
                        share_cores,explore_freq,obj_eval,obj_update);

                //unmap and try a different free core in the next iteration
                BUG_ON(task_next_core_idx(sys->task_list[currTask]) != core);
                task_next_core_unmap(sys->task_list[currTask]);
            }
        }
    }
    else{
        //we mapped everything, evaluate the mapping and go
        ++currPerm;
        if(explore_freq){
            for(core = 0; core < sys->info->core_list_size; ++core){
            	for_enum(core_freq_t,freq,0,SIZE_COREFREQ,+1){
                    if(!vitamins_arch_freq_available(sys->info->core_list[core].arch,freq)) continue;

                    vitamins_dvfs_manual_freq(sys->info->core_list[core].this_core,freq);
                    vitamins_load_tracker_map_changed(sys->info->core_list[core].this_core);

                    eval_one_mapping(sys,explore_freq,obj_eval,obj_update);
                }
            }
        }
        else{
            eval_one_mapping(sys,explore_freq,obj_eval,obj_update);
        }
    }

}

static
void
init_mappings(model_sys_t *sys)
{
    int task,core;

    vitamins_load_tracker_set(LT_CFS);

    //all cores available, all threads unmapped
    for (task = 0; task < sys->task_list_size; ++task) bestmapping[task] = nullptr;
    for (core = 0; core < sys->info->core_list_size; ++core) bestfreq[core] = SIZE_COREFREQ;
    bestmapping_ipswatt = 0;
    bestmapping_ips = 0;
    bestmapping_ips_upthr = 0;
    bestmapping_ips_downthr = 0;
    clear_next_map(sys);
}


static inline uint32_t ips_upthr_scale(uint32_t ips) { return ips * CONV_DOUBLE_scaledUINT32(1.05); }
static inline uint32_t ips_downthr_scale(uint32_t ips) { return ips * CONV_DOUBLE_scaledUINT32(0.97); }
static inline uint32_t ips_scale(uint32_t ips) { return ips * CONV_DOUBLE_scaledUINT32(1.0); }
static inline
bool
objective_eval_sparta(uint32_t ips, uint32_t ipswatt)
{
    uint32_t ips_scaled = ips_scale(ips);
    if((ips_scaled >= bestmapping_ips) && (ipswatt >= bestmapping_ipswatt))
        return true;
    else if(ips_scaled >= bestmapping_ips_upthr)
        return true;
    else if((ips_scaled >= bestmapping_ips_downthr) && (ipswatt >= bestmapping_ipswatt))
        return true;
    else
        return false;

}
static inline
void
objective_update_sparta(uint32_t ips, uint32_t ipswatt)
{
    bestmapping_ips_upthr    = ips_upthr_scale(ips);
    bestmapping_ips          = ips_scale(ips);
    bestmapping_ips_downthr  = ips_downthr_scale(ips);
    bestmapping_ipswatt = ipswatt;
}


static inline
bool
objective_eval_max_ips(uint32_t ips, uint32_t ipswatt)
{
    return ips >= bestmapping_ips;

}
static inline
bool
objective_eval_max_ips_per_watt(uint32_t ips, uint32_t ipswatt)
{
    return ipswatt >= bestmapping_ipswatt;

}
static inline
bool
objective_eval_ips_tgt(uint32_t ips, uint32_t ipswatt)
{
    if(ips >= bestmapping_ips_tgt){
        return ipswatt >= bestmapping_ipswatt;
    }
    else if(bestmapping_ips < bestmapping_ips_tgt){
        //we haven't found anything that meets the target so accept if it improves IPS
        return ips >= bestmapping_ips;
    }
    else return false;
}
static inline
void
objective_update_max_ips_and_ipswatt(uint32_t ips, uint32_t ipswatt)
{
    bestmapping_ipswatt = ipswatt;
    bestmapping_ips = ips;
}



static
void
optimal_map(
        model_sys_t *sys,
        bool share_cores, bool explore_freq, objective_eval_func obj_eval, objective_update_func obj_update)
{
    int core,task;

    init_mappings(sys);

    maxPerm = count_permutations(sys->task_list_size,sys->info->core_list_size,share_cores);
    currPerm = 0;

    //call the recursive function, starting with the first task
    explore_all_mappings(0,sys,share_cores,explore_freq,obj_eval,obj_update);

    BUG_ON(maxPerm != currPerm);

    //sanity state check
    for (core = 0; core < sys->info->core_list_size; ++core) BUG_ON(sys->info->core_list[core].this_core->load_tracking.common.task_cnt != 0);
    for (task = 0; task < sys->task_list_size; ++task) BUG_ON(task_next_core(sys->task_list[task]) != nullptr);

    //assign mapping
    if(explore_freq){
        for(core = 0; core < sys->info->core_list_size; ++core){
            vitamins_dvfs_manual_freq(sys->info->core_list[core].this_core,bestfreq[core]);
        }
    }
    for (task = 0; task < sys->task_list_size; ++task){
        task_next_core_map(sys->task_list[task],bestmapping[task]);
    }
}


void
vitamins_optimal_map(model_sys_t *sys)
{
    const bool share_cores = false;
    const bool explore_freqs = false;
    optimal_map(sys,
            share_cores,explore_freqs,
            objective_eval_max_ips_per_watt,
            objective_update_max_ips_and_ipswatt);
}
void
vitamins_optimal_shared_map(model_sys_t *sys)
{
    const bool share_cores = true;
    const bool explore_freqs = false;
    optimal_map(sys,
            share_cores,explore_freqs,
            objective_eval_max_ips_per_watt,
            objective_update_max_ips_and_ipswatt);
}
void
vitamins_optimal_shared_freq_map(model_sys_t *sys)
{
    const bool share_cores = true;
    const bool explore_freqs = true;
    optimal_map(sys,
            share_cores,explore_freqs,
            objective_eval_max_ips_per_watt,
            objective_update_max_ips_and_ipswatt);
}
void
vitamins_optimalIPS_map(model_sys_t *sys)
{
    const bool share_cores = false;
    const bool explore_freqs = false;
    optimal_map(sys,
            share_cores,explore_freqs,
            objective_eval_max_ips,
            objective_update_max_ips_and_ipswatt);
}
void
vitamins_optimalIPS_shared_map(model_sys_t *sys)
{
    const bool share_cores = true;
    const bool explore_freqs = false;
    optimal_map(sys,
            share_cores,explore_freqs,
            objective_eval_max_ips,
            objective_update_max_ips_and_ipswatt);
}
void
vitamins_optimal_sparta_map(model_sys_t *sys)
{
    const bool share_cores = false;
    const bool explore_freqs = false;
    optimal_map(sys,
            share_cores,explore_freqs,
            objective_eval_sparta,
            objective_update_sparta);
}
void
vitamins_optimal_sparta_shared_map(model_sys_t *sys)
{
    const bool share_cores = true;
    const bool explore_freqs = false;
    optimal_map(sys,
            share_cores,explore_freqs,
            objective_eval_sparta,
            objective_update_sparta);
}
void
vitamins_optimal_energy_given_ips(model_sys_t *sys, uint32_t ips_tgt)
{
    const bool share_cores = false;
    const bool explore_freqs = false;
    bestmapping_ips_tgt = ips_tgt;
    optimal_map(sys,
            share_cores,explore_freqs,
            objective_eval_ips_tgt,
            objective_update_max_ips_and_ipswatt);
}
