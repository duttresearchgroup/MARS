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

#include "solver_cinterface.h"
#include "solver_interface.h"

extern "C" {

SASolverImpl::SolverTOP *vit_solver_obj = 0;

int vit_solver_create(void* (*allocator)(long unsigned int),
					  void (*deallocator)(void*),
					  void (*assertor)(int,const char*,int,const char *),
				      long unsigned int max_num_cpus, long unsigned int max_num_tasks){
	if(SASolverImpl::Global::create_globals(allocator,deallocator,assertor,max_num_cpus,max_num_tasks)){

		vit_solver_obj = SASolverImpl::Global::allocate<SASolverImpl::SolverTOP>();

		if(vit_solver_obj!=0){
			if(vit_solver_obj->is_ok()) return 1;
			else{
				SASolverImpl::Global::deallocate_obj<SASolverImpl::SolverTOP>(vit_solver_obj);
				SASolverImpl::Global::destroy_globals();
				return 0;
			}
		}
		else{
			SASolverImpl::Global::destroy_globals();
			return 0;
		}

	}
	else{
		SASolverImpl::Global::destroy_globals();
		return 0;
	}
}

void vit_solver_destroy(){
	SASolverImpl::Global::deallocate_obj<SASolverImpl::SolverTOP>(vit_solver_obj);
	SASolverImpl::Global::destroy_globals();
}

void vit_solver_start_solver(){
	vit_solver_obj->start_solver();
}
void vit_solver_reset_solver(){
	vit_solver_obj->reset_solver();
}
void vit_solver_set_rnd_seed(unsigned int seed){
	vit_solver_obj->set_rnd_seed(seed);
}
void vit_solver_set_max_iter(unsigned int val){
	vit_solver_obj->set_max_iter(val);
}
void vit_solver_set_gen_nb_temp(unsigned int val){
	vit_solver_obj->set_gen_nb_temp(val);
}
void vit_solver_set_accept_temp(unsigned int val){
	vit_solver_obj->set_accept_temp(val);
}
void vit_solver_set_gen_nb_temp_alpha_scaled(unsigned int val_scaled){
	vit_solver_obj->set_gen_nb_temp_alpha_scaled(val_scaled);
}
void vit_solver_set_accept_temp_alpha_scaled(unsigned int val_scaled){
	vit_solver_obj->set_accept_temp_alpha_scaled(val_scaled);
}
unsigned int vit_solver_get_accept_temp_alpha_scaled(){
	return vit_solver_obj->get_accept_temp_alpha_scaled();
}
void vit_solver_set_diff_scaling_factor_scaled(unsigned int val_scaled){
	vit_solver_obj->set_diff_scaling_factor_scaled(val_scaled);
}
unsigned int vit_solver_get_solver_stat_iterations(){
	return vit_solver_obj->get_solver_stat_iterations();
}
unsigned int vit_solver_get_solver_stat_better_sol_accepted(){
	return vit_solver_obj->get_solver_stat_better_sol_accepted();
}
unsigned int vit_solver_get_solver_stat_equal_sol_accepted(){
	return vit_solver_obj->get_solver_stat_equal_sol_accepted();
}
unsigned int vit_solver_get_solver_stat_evaluated_solutions(){
	return vit_solver_obj->get_solver_stat_evaluated_solutions();
}
unsigned int vit_solver_get_solver_stat_rejected_solutions(){
	return vit_solver_obj->get_solver_stat_rejected_solutions();
}
unsigned int vit_solver_get_solver_stat_worse_sol_accepted(){
	return vit_solver_obj->get_solver_stat_worse_sol_accepted();
}
#ifdef LOG_RESULTS
long long int vit_solver_get_solver_obj_func_history_scaled(int iter){
	return vit_solver_obj->get_solver_obj_func_history_scaled(iter);
}
bool vit_solver_get_solver_obj_func_accepted(int iter){
	return vit_solver_obj->get_solver_obj_func_accepted(iter);
}
#endif
void vit_solver_set_num_tasks(unsigned int val){
	vit_solver_obj->set_num_tasks(val);
}
void vit_solver_set_task_expl_factor_scaled(unsigned int val_scaled){
	vit_solver_obj->set_task_expl_factor_scaled(val_scaled);
}
void vit_solver_set_num_cores(unsigned int val){
	vit_solver_obj->set_num_cores(val);
}
int vit_solver_get_solver_state(){
	return vit_solver_obj->get_solver_state();
}
long long int vit_solver_get_current_objective_scaled(void){
	return vit_solver_obj->get_current_objective_scaled();
}
long long int vit_solver_get_current_secondary_objective_scaled(int obj){
	return vit_solver_obj->get_current_secondary_objective_scaled(obj);
}

void vit_solver_io_set_solution_reset(){
	vit_solver_obj->io_set_solution_reset();
}
void vit_solver_io_set_solution(unsigned int cpu, unsigned int pos, unsigned int task){
	vit_solver_obj->io_set_solution(cpu,pos,task);
}
void vit_solver_io_set_task_active_ips(unsigned int task, unsigned int cpu, unsigned int val){
	vit_solver_obj->io_set_task_active_ips(task,cpu,val);
}
void vit_solver_io_set_task_active_power_scaled(unsigned int task, unsigned int cpu, unsigned int val){
	vit_solver_obj->io_set_task_active_power_scaled(task,cpu,val);
}
void vit_solver_io_set_task_demand_scaled(unsigned int task, unsigned int cpu,unsigned int val_scaled){
	vit_solver_obj->io_set_task_demand_scaled(task,cpu,val_scaled);
}
void vit_solver_io_set_cpu_idle_power_scaled(unsigned int cpu, unsigned int idle_power_scaled,unsigned int idle_power_when_empty_scaled){
	vit_solver_obj->io_set_cpu_idle_power_scaled(cpu,idle_power_scaled,idle_power_when_empty_scaled);
}
void vit_solver_io_set_cpu_kernel_idle_load_scaled(int cpu, unsigned int val_scaled){
	vit_solver_obj->io_set_cpu_kernel_idle_load_scaled(cpu,val_scaled);
}
void vit_solver_io_set_cpu_kernel_active_power_scaled(int cpu, unsigned int val_scaled){
	vit_solver_obj->io_set_cpu_kernel_active_power_scaled(cpu,val_scaled);
}
unsigned int vit_solver_io_get_solution(unsigned int cpu, unsigned int pos){
	return vit_solver_obj->io_get_solution(cpu,pos);
}
long long int vit_solver_io_get_pred_task_load_scaled(unsigned int task){
	return vit_solver_obj->io_get_pred_task_load_scaled(task);
}

}



