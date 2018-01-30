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

#ifndef __task_map_top_level_h
#define __task_map_top_level_h

#include "solver_defines.h"
#include "solver.h"
#ifdef __KERNEL__
typedef unsigned long long int uint64_t;
typedef long long int int64_t;
#endif

#include "../core/base/base.h"

namespace SASolverImpl {

#define OPT_MAX_IPS_WATT
//#define OPT_MAX_IPS
//#define OPT_MIN_POWER
//#define OPT_MAX_IPS_FOR_POWER

#define OBJ_POWER_CONSTRAINT 8

#if defined(OPT_MAX_IPS_WATT)
	#define OBJ_F	Objectives::Max_IPC_per_Watt
	#define OBJ_TGR	MAXIMIZE
#elif defined(OPT_MAX_IPS)
	#define OBJ_F	Objectives::Max_IPC
	#define OBJ_TGR	MAXIMIZE
#elif defined(OPT_MIN_POWER)
	#define OBJ_F	Objectives::Min_Power
	#define OBJ_TGR	MINIMIZE
#elif defined(OPT_MAX_IPS_FOR_POWER)
	#define OBJ_F	Objectives::Max_Perf_given_Power
	#define OBJ_TGR	MAXIMIZE
#endif



class SolverTOP {
private:

	typedef OBJ_F ObjectiveF_t;
	typedef Schedule<ObjectiveF_t> Solution_t;
	typedef Solver<ObjectiveF_t, OBJ_TGR, Stop_Criteria::Never,SASOLVER_MAX_ITER_HARD> Solver_t;

	class Solver_Wrapper : public Solver_t {
	public:
		Solver_Wrapper()
			:Solver_t(Stop_Criteria::Never())
		{}
	};



	int max_iter;
	double gen_nb_temp;
	double gen_nb_temp_alpha;
	double accept_temp;
	double accept_temp_alpha;
	double diff_scaling;

	Solver_Wrapper sas;

public:

	bool is_ok(){
		return sas.is_ok();
	}

	void start_solver(){
		vassert(Global::Input_Params::N_TASKS <= Global::Input_Params::MAX_N_TASKS);
		vassert(Global::Input_Params::N_TASKS > 0);
		vassert(Global::Input_Params::TASK_EXPLORATION_FACTOR >= 0);
		vassert(Global::Input_Params::TASK_EXPLORATION_FACTOR <= 1);
		vassert(Global::Input_Params::N_CPUS <= Global::Input_Params::MAX_N_CPUS);
		vassert(Global::Input_Params::N_CPUS > 0);

		vassert(max_iter > 0);

		vassert((sas.get_stats().curr_stop_reason == Solver_t::IDLE) &&
				"SOlver must be reset before running");

		sas.solve();
	}

	void reset_solver(){
		vassert(Global::Input_Params::N_TASKS <= Global::Input_Params::MAX_N_TASKS);
		vassert(Global::Input_Params::N_TASKS > 0);
		vassert(Global::Input_Params::TASK_EXPLORATION_FACTOR >= 0);
		vassert(Global::Input_Params::TASK_EXPLORATION_FACTOR <= 1);
		vassert(Global::Input_Params::N_CPUS <= Global::Input_Params::MAX_N_CPUS);
		vassert(Global::Input_Params::N_CPUS > 0);

		vassert(max_iter > 0);

		vassert((sas.get_stats().curr_stop_reason != Solver_t::RUNNING) &&
				"Never issue a RESET cmd while the solver one is still running");

		sas.reset_params(gen_nb_temp, gen_nb_temp_alpha, accept_temp, accept_temp_alpha, diff_scaling, max_iter);
		Global::constraints().total_power = OBJ_POWER_CONSTRAINT;
	}

	void set_max_iter(int val){
		max_iter = val;
		vassert(max_iter > 0);
		vassert(max_iter <= SASOLVER_MAX_ITER_HARD);
	}

	void set_gen_nb_temp(unsigned int val){
		gen_nb_temp = val;
		vassert(gen_nb_temp > 0);
	}

	void set_gen_nb_temp_alpha_scaled(unsigned int val_scaled){
		gen_nb_temp_alpha = CONV_scaledINTany_DOUBLE(val_scaled);
		vassert(gen_nb_temp_alpha > 0);
		vassert(gen_nb_temp_alpha < 1);
	}

	void set_accept_temp(unsigned int val){
		accept_temp = val;
		vassert(gen_nb_temp > 0);
	}

	void set_rnd_seed(unsigned int seed){
		Solver_t::Rnd::seed(seed);
	}

	void set_accept_temp_alpha_scaled(unsigned int val_scaled){
		accept_temp_alpha = CONV_scaledINTany_DOUBLE(val_scaled);
		vassert(accept_temp_alpha > 0);
		vassert(accept_temp_alpha < 1);
	}

	unsigned int get_accept_temp_alpha_scaled(){
		return CONV_DOUBLE_scaledUINT(accept_temp_alpha);
	}

	void set_diff_scaling_factor_scaled(unsigned int val_scaled){
		diff_scaling = CONV_scaledINTany_DOUBLE(val_scaled);
		vassert(diff_scaling > 0);
	}

	int get_solver_stat_iterations(){
		return sas.get_stats().curr_iter;
	}

	int get_solver_stat_better_sol_accepted(){
		return sas.get_stats().better_solutions_accepted;
	}

	int get_solver_stat_equal_sol_accepted(){
		return sas.get_stats().eq_solutions_accepted;
	}

	int get_solver_stat_evaluated_solutions(){
		return sas.get_stats().solutions_evaluated;
	}

	int get_solver_stat_rejected_solutions(){
		return sas.get_stats().solutions_rejected;
	}


	int get_solver_stat_worse_sol_accepted(){
		return sas.get_stats().worse_solutions_accepted;
	}

#ifdef LOG_RESULTS
	long long int get_solver_obj_func_history_scaled(int iter){
		return CONV_DOUBLE_scaledLLINT(sas.obj_func_history[iter]);
	}

	bool get_solver_obj_func_accepted(int iter){
		return sas.obj_func_accepted[iter];
	}
#endif

	void set_num_tasks(int val){
		Global::Input_Params::N_TASKS = val;
		Global::Input_Params::MAPPING_SIZE = Global::Input_Params::N_TASKS*Global::Input_Params::N_CPUS;
	}

	void set_task_expl_factor_scaled(unsigned int val_scaled){
		Global::Input_Params::TASK_EXPLORATION_FACTOR = CONV_scaledINTany_DOUBLE(val_scaled);
	}

	void set_num_cores(int val){
		Global::Input_Params::N_CPUS = val;
		Global::Input_Params::MAPPING_SIZE = Global::Input_Params::N_TASKS*Global::Input_Params::N_CPUS;
		inputs_precond_limits_cpu();

		//seting these to 0 elliminate their effect
		for(int i = 0; i < Global::Input_Params::N_CPUS; ++i){
			Global::input_data().cpu_static_data[i].kernel_idle_load = 0;
			Global::input_data().cpu_static_data[i].kernel_active_power = 0;
		}
	}

	int get_solver_state(){
		return sas.get_stats().curr_stop_reason;
	}

	long long int get_current_objective_scaled(){
		return CONV_DOUBLE_scaledLLINT(sas.get_solution().objective_val());
	}

	long long int get_current_secondary_objective_scaled(int obj){
		return CONV_DOUBLE_scaledLLINT(sas.get_solution().objective_val(obj));
	}

private:
	inline void inputs_precond_limits_cpu(){
		vassertSTR(Global::Input_Params::N_CPUS <= Global::Input_Params::MAX_N_CPUS);
		vassertSTR(Global::Input_Params::N_CPUS > 0);
	}
	inline void inputs_precond_limits_task(){
		vassertSTR(Global::Input_Params::N_TASKS <= Global::Input_Params::MAX_N_TASKS);
		vassertSTR(Global::Input_Params::N_TASKS > 0);
	}
	inline void inputs_precond_cpu(int cpu){
		vassertSTR(cpu < Global::Input_Params::N_CPUS);
	}
	inline void inputs_precond_task(int task){
		vassertSTR(task < Global::Input_Params::N_TASKS);
	}
public:
	void io_set_solution_reset(){
		inputs_precond_limits_task();
		inputs_precond_limits_cpu();
		for (int cpu = 0; cpu < Global::Input_Params::N_CPUS; ++cpu) {
			for (int task = 0; task < Global::Input_Params::N_TASKS; ++task) {
				Global::output_data().task_mapping[cpu][task] = 0;
			}
		}
	}

	void io_set_solution(int cpu, int pos, int task){
		inputs_precond_limits_task();
		inputs_precond_limits_cpu();
		inputs_precond_cpu(cpu);
		inputs_precond_task(pos);
		vassertSTR(task > 0);
		vassertSTR(task <= Global::Input_Params::N_TASKS);
		Global::output_data().task_mapping[cpu][pos] = task;
	}

	void io_set_task_active_ips(int task, int cpu, unsigned int val){
		inputs_precond_limits_task();
		inputs_precond_limits_cpu();
		inputs_precond_cpu(cpu);
		inputs_precond_task(task);
		Global::input_data().task_per_cpu_data[task][cpu].active_ips = val;
	}

	void io_set_task_active_power_scaled(int task, int cpu, unsigned int val_scaled){
		inputs_precond_limits_task();
		inputs_precond_limits_cpu();
		inputs_precond_cpu(cpu);
		inputs_precond_task(task);
		Global::input_data().task_per_cpu_data[task][cpu].active_power = CONV_scaledINTany_DOUBLE(val_scaled);
	}

	void io_set_task_demand_scaled(int task, int cpu, unsigned int val_scaled){
		inputs_precond_limits_task();
		inputs_precond_limits_cpu();
		inputs_precond_task(task);
		Global::input_data().task_per_cpu_data[task][cpu].demand = CONV_scaledINTany_DOUBLE(val_scaled);
		vassert(Global::input_data().task_per_cpu_data[task][cpu].demand >= 0);
		vassert(Global::input_data().task_per_cpu_data[task][cpu].demand <= 1);
	}

	void io_set_cpu_idle_power_scaled(int cpu, unsigned int val_scaled, unsigned int val_when_empty_scaled){
		inputs_precond_limits_cpu();
		inputs_precond_cpu(cpu);
		Global::input_data().cpu_static_data[cpu].idle_power = CONV_scaledINTany_DOUBLE(val_scaled);
		Global::input_data().cpu_static_data[cpu].idle_power_when_empty = CONV_scaledINTany_DOUBLE(val_when_empty_scaled);
	}

	void io_set_cpu_kernel_idle_load_scaled(int cpu, unsigned int val_scaled){
		inputs_precond_limits_cpu();
		inputs_precond_cpu(cpu);
		Global::input_data().cpu_static_data[cpu].kernel_idle_load = CONV_scaledINTany_DOUBLE(val_scaled);
	}
	void io_set_cpu_kernel_active_power_scaled(int cpu, unsigned int val_scaled){
		inputs_precond_limits_cpu();
		inputs_precond_cpu(cpu);
		Global::input_data().cpu_static_data[cpu].kernel_active_power = CONV_scaledINTany_DOUBLE(val_scaled);
	}

	int io_get_solution(int cpu, int pos){
		inputs_precond_limits_cpu();
		inputs_precond_cpu(cpu);
		inputs_precond_task(pos);
		return Global::output_data().task_mapping[cpu][pos];
	}

	long long int io_get_pred_task_load_scaled(int task){
		inputs_precond_limits_cpu();
		inputs_precond_task(task);
		return CONV_DOUBLE_scaledLLINT(Global::output_data().task_pred_load[task]);
	}
};

};

#endif


