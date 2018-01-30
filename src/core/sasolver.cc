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

#include "core.h"
#include "../sa_solver/solver_cinterface.h"
#include "../sa_solver/solver_defines.h"


sasolver_solver_conf_t curr_conf = {
/*.max_iter = 300,
.gen_nb_temp = 2048,
.gen_nb_temp_alpha_scaled = CONV_DOUBLE_scaledUINT(0.85),
.accept_temp = 256,
.accept_temp_alpha_scaled = CONV_DOUBLE_scaledUINT(0.97),
.diff_scaling_factor_scaled = CONV_DOUBLE_scaledUINT(0.1),
.task_expl_factor_scaled = CONV_DOUBLE_scaledUINT(0),
.rnd_seed = -123456789*/
        .max_iter = 500,
        .gen_nb_temp = 3670,
        .gen_nb_temp_alpha_scaled = CONV_DOUBLE_scaledUINT(0.98),
        .accept_temp = 28,
        .accept_temp_alpha_scaled = CONV_DOUBLE_scaledUINT(0.91),
        .diff_scaling_factor_scaled = CONV_DOUBLE_scaledUINT(1000),
        .task_expl_factor_scaled = CONV_DOUBLE_scaledUINT(0),
        .rnd_seed = 123456789
};

static void sasolver_init_solver(void){

    vit_solver_set_max_iter(curr_conf.max_iter);

    vit_solver_set_gen_nb_temp(curr_conf.gen_nb_temp);
    vit_solver_set_gen_nb_temp_alpha_scaled(curr_conf.gen_nb_temp_alpha_scaled);
    vit_solver_set_accept_temp(curr_conf.accept_temp);
    vit_solver_set_accept_temp_alpha_scaled(curr_conf.accept_temp_alpha_scaled);

    vit_solver_set_diff_scaling_factor_scaled(curr_conf.diff_scaling_factor_scaled);
    vit_solver_set_task_expl_factor_scaled(curr_conf.task_expl_factor_scaled);

    vit_solver_set_rnd_seed(curr_conf.rnd_seed);

}

sasolver_solver_conf_t* vitamins_sasolver_get_conf(void){
    return &curr_conf;
}
void vitamins_sasolver_set_conf(sasolver_solver_conf_t *conf){
    if(conf != &curr_conf) {
        curr_conf = *conf;
        sasolver_init_solver();
    }
}


#ifdef SASOLVER_SOLVER_NUM_ITER_ADAPTATIVE
static inline void sasolver_start_solver_set_iter(int _cpu, unsigned int num_cpus, unsigned int num_tasks){
    unsigned int n_iter;
    unsigned int prob_size = num_cpus*num_tasks;
         if(prob_size <= (4*4)) n_iter = 100;
    else if(prob_size <= (4*8)) n_iter = 150;
    else if(prob_size <= (4*16)) n_iter = 300;
    else n_iter = SASOLVER_SOLVER_NUM_ITER;
    vit_solver_set_max_iter(n_iter);
}
#endif

static void sasolver_start_solver(model_sys_t *sys){
    int i,j;

    //printk(KERN_INFO "sasolver_solver(%d)\n",_cpu);

    vit_solver_set_num_cores(sys->info->core_list_size);

    //cpu static power
    for (i= 0; i < sys->info->core_list_size; ++i){
        //its one task per core, with the task power including all components, so this is 0
        vit_solver_io_set_cpu_idle_power_scaled(
                i,
                arch_idle_power_scaled(sys->info->core_list[i].arch,vitamins_dvfs_get_maximum_freq(sys->info->core_list[i].this_core)),
                arch_idle_power_scaled(sys->info->core_list[i].arch,vitamins_dvfs_get_minimum_freq(sys->info->core_list[i].this_core)));
    }

    vit_solver_set_num_tasks(sys->task_list_size);

    //task ips
    for (i = 0; i < sys->task_list_size; ++i){
        for (j = 0; j < sys->info->core_list_size; ++j) {
            vit_solver_io_set_task_active_ips(i, j,
            		sys->task_list[i]->ips_active[sys->info->core_list[j].arch][vitamins_dvfs_get_maximum_freq(sys->info->core_list[i].this_core)]);
        }
    }

    //printk(KERN_INFO "sasolver_solver_2\n");

    //task dyn power
    //its one task per core, so this includes all power including all components
    for (i = 0; i < sys->task_list_size; ++i){
        for (j = 0; j < sys->info->core_list_size; ++j){
            vit_solver_io_set_task_active_power_scaled(i, j,
            		sys->task_list[i]->power_active[sys->info->core_list[j].arch][vitamins_dvfs_get_maximum_freq(sys->info->core_list[i].this_core)]);
        }
    }


    //task demand
    //its one task per core, and ips and power include both idle and active components,
    //so we always set this to 1
    for (i = 0; i < sys->task_list_size; ++i)
        for (j = 0; j < sys->info->core_list_size; ++j)
            vit_solver_io_set_task_demand_scaled(i, j, CONV_DOUBLE_scaledUINT64(1.0));

    //update overheads
    //lets have this at 0 for now
    for (j = 0; j < sys->info->core_list_size; ++j){
        vit_solver_io_set_cpu_kernel_active_power_scaled(j, 0);
        vit_solver_io_set_cpu_kernel_idle_load_scaled(j,0);
    }

    //initial sched
    vit_solver_io_set_solution_reset();

    for(j = 0; j < sys->info->core_list_size; ++j){
        int pos = 0;
        for (i = 0; i < sys->task_list_size; ++i){
            if(task_curr_core_idx(sys->task_list[i]) == j)
                vit_solver_io_set_solution(j, pos++, i+1);
        }
    }


    vit_solver_reset_solver();

    BUG_ON(vit_solver_get_solver_state() == SD_SOLVER_STATUS_UNITIALIZED);


    vit_solver_start_solver();

}

static void sasolver_optimize(model_sys_t *sys){

    int i,task,pos;

    vitamins_load_tracker_set(LT_CFS);

    clear_next_map(sys);

    sasolver_start_solver(sys);

    BUG_ON(vit_solver_get_solver_state() == SD_SOLVER_STATUS_IDLE);
    BUG_ON(vit_solver_get_solver_state() == SD_SOLVER_STATUS_RUNNING);

    //We may have only found a equivalent (but potentially very different) solution.
    //In this, case, balancing may yield a lot of overhead without any actual gain
    if(vit_solver_get_solver_stat_better_sol_accepted() > 0) {
        for (i = 0; i < sys->info->core_list_size; ++i) {
            for(pos = 0; pos < sys->task_list_size; ++pos){
                task = vit_solver_io_get_solution(i,pos);
                if(task != 0){
                    BUG_ON(task > sys->task_list_size);
                    task_next_core_map(sys->task_list[task-1],sys->info->core_list[i].this_core);
                }
            }
        }
    }
    else{
        for(task = 0; task < sys->task_list_size; ++task){
            task_next_core_map(sys->task_list[task],task_curr_core(sys->task_list[task]));
        }
    }
    //kernel_fpu_end();
}


static bool init_once = true;

static void
_init_sasolver_once(void)
{
    sasolver_init_solver();
}

static inline void
init_sasolver_once(void)
{
    if (init_once) {
        _init_sasolver_once();
        init_once = false;
    }
}


void
vitamins_sasolver_map(model_sys_t *sys)
{
    init_sasolver_once();
    sasolver_optimize(sys);
}

void
vitamins_sasolver_cleanup()
{
    init_once = true;
}
