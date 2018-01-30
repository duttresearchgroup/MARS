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

#ifndef __c_task_map_top_level_h
#define __c_task_map_top_level_h

#ifdef __cplusplus
extern "C" {
#endif

int  vit_solver_create(void* (*allocator)(long unsigned int),
		               void (*deallocator)(void*),
		               void (*assertor)(int,const char*,int,const char *),
		               long unsigned int max_num_cpus, long unsigned int max_num_tasks);
void vit_solver_destroy(void);

void vit_solver_start_solver(void);
void vit_solver_reset_solver(void);
void vit_solver_set_rnd_seed(unsigned int seed);
void vit_solver_set_max_iter(unsigned int val);
void vit_solver_set_gen_nb_temp(unsigned int val);
void vit_solver_set_gen_nb_temp_alpha_scaled(unsigned int val_scaled);
void vit_solver_set_accept_temp(unsigned int val);
void vit_solver_set_accept_temp_alpha_scaled(unsigned int val_scaled);
unsigned int vit_solver_get_accept_temp_alpha_scaled(void);
void vit_solver_set_diff_scaling_factor_scaled(unsigned int val_scaled);
unsigned int vit_solver_get_solver_stat_iterations(void);
unsigned int vit_solver_get_solver_stat_better_sol_accepted(void);
unsigned int vit_solver_get_solver_stat_equal_sol_accepted(void);
unsigned int vit_solver_get_solver_stat_evaluated_solutions(void);
unsigned int vit_solver_get_solver_stat_rejected_solutions(void);
unsigned int vit_solver_get_solver_stat_worse_sol_accepted(void);
long long int vit_solver_get_solver_obj_func_history_scaled(int iter);
bool vit_solver_get_solver_obj_func_accepted(int iter);
void vit_solver_set_num_tasks(unsigned int val);
void vit_solver_set_task_expl_factor_scaled(unsigned int val_scaled);
void vit_solver_set_num_cores(unsigned int val);
int vit_solver_get_solver_state(void);
long long int vit_solver_get_current_objective_scaled(void);
long long int vit_solver_get_current_secondary_objective_scaled(int obj);

void vit_solver_io_set_solution_reset(void);
void vit_solver_io_set_solution(unsigned int cpu, unsigned int pos, unsigned int task);
void vit_solver_io_set_task_active_ips(unsigned int task, unsigned int cpu, unsigned int val);
void vit_solver_io_set_task_active_power_scaled(unsigned int task, unsigned int cpu, unsigned int val);
void vit_solver_io_set_task_demand_scaled(unsigned int task, unsigned int cpu, unsigned int val_scaled);
void vit_solver_io_set_cpu_idle_power_scaled(unsigned int cpu, unsigned int idle_power_scaled,unsigned int idle_power_when_empty_scaled);
void vit_solver_io_set_cpu_kernel_idle_load_scaled(int cpu, unsigned int val_scaled);
void vit_solver_io_set_cpu_kernel_active_power_scaled(int cpu, unsigned int val_scaled);
unsigned int vit_solver_io_get_solution(unsigned int cpu, unsigned int pos);
long long int vit_solver_io_get_pred_task_load_scaled(unsigned int task);

#ifdef __cplusplus
}
#endif

#endif


