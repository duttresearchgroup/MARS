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

#ifndef __sim_annealing_h
#define __sim_annealing_h

#ifndef __KERNEL__
	#include <iostream>	// needed for basic IO
	#include <sstream>
#endif

#include "custom_random.h"
#include "custom_math.h"
#include "solver_defines.h"
#include "globals.h"
#include "criteria.h"
#include "schedule.h"
#include "objectives.h"

namespace SASolverImpl {

enum OBJ_TARGET {
	MINIMIZE,
	MAXIMIZE,
};

template<typename ObjectiveF, OBJ_TARGET Target, typename Stop_Criterium, int MAX_ITER_HARD>
class Solver {
public:
	typedef enum{
		UNITIALIZED = SD_SOLVER_STATUS_UNITIALIZED,
		IDLE = SD_SOLVER_STATUS_IDLE,//used by top_level interface only. This object is either running or stoped
		RUNNING = SD_SOLVER_STATUS_RUNNING,
		STOP_MAX_ITERATIONS = SD_SOLVER_STATUS_STOP_MAX_ITERATIONS, //Maximum number of iterations reached. Current solution does not match the precision criterion
		STOP_CRIT_MET = SD_SOLVER_STATUS_STOP_CRIT_MET //Distance to target is smaller than the required precision. Solution found
	}stop_reason_t;

	typedef struct {
		int curr_iter;
		stop_reason_t curr_stop_reason;
		int solutions_evaluated;
		int better_solutions_accepted;
		int eq_solutions_accepted;
		int worse_solutions_accepted;
		int solutions_rejected;
	} stats_t;

	typedef Pseudo_Random<RND_DEFAULT> Rnd;

private:
	void init_solution(Schedule<ObjectiveF> &sched){
		sched.read_from_ports();
	}

	void commit_solution(const Schedule<ObjectiveF> &sched){
		sched.write_to_ports();
	}

	void give_random_neighbour_for_task(const int task_idx){


		int task = task_idx + 1;

		int curr_position = curr_solution.task_position(task_idx);

		int perturb = Rnd::random((-1)*Global::Input_Params::MAPPING_SIZE, Global::Input_Params::MAPPING_SIZE*1);

		double temp_sqrt = Math::sqrt(gen_nb_temp);

		double perturb_temp_sqrt = perturb * temp_sqrt;

		int new_position = (int)Math::round_nearest<int,double>(double(curr_position) + perturb_temp_sqrt);
		//wrap around
		while(new_position < 0) 					new_position = Global::Input_Params::MAPPING_SIZE + new_position;
		while(new_position >= Global::Input_Params::MAPPING_SIZE)	new_position = new_position - Global::Input_Params::MAPPING_SIZE;

		vassert(new_position >= 0);
		vassert(new_position < Global::Input_Params::MAPPING_SIZE);

		//insert
		curr_solution.switch_task_position(task, curr_position, new_position);
	}

	void give_random_neighbour() {

		if(Global::Input_Params::TASK_EXPLORATION_FACTOR == 1){
			//perturb all tasks
			for (int task_idx = 0; task_idx < Global::Input_Params::N_TASKS; ++task_idx) {
				give_random_neighbour_for_task(task_idx);
			}
		}
		else {
			int n_tasks_to_perturb = (Global::Input_Params::TASK_EXPLORATION_FACTOR == 0)
					? 1
					: Math::round_nearest<int,double>(Global::Input_Params::N_TASKS * Global::Input_Params::TASK_EXPLORATION_FACTOR);

			if(n_tasks_to_perturb < 1) n_tasks_to_perturb = 1;

			vassert(n_tasks_to_perturb <= Global::Input_Params::N_TASKS);

			for (int i = 0; i < n_tasks_to_perturb; ++i) {
				//select a random task
				int task_idx = Rnd::random() % Global::Input_Params::N_TASKS;
				vassert(task_idx < Global::Input_Params::N_TASKS);
				vassert(task_idx >= 0);

				give_random_neighbour_for_task(task_idx);
			}
		}
	}

	double calc_distance_to_target() const{

		double distance = 0;
		switch (Target) {
		case MINIMIZE:
			distance = curr_solution.objective_val(); //dis to 0 equals obj func val
			break;
		case MAXIMIZE:
			distance = 1/curr_solution.objective_val(); //distance -> 0 if val -> infinity (the maximum)
			break;
		default:
			vassert(false && "Fuck");
			break;
		}

		vassert(distance >= 0); //no negative distance allowed
		return distance;
	}

public:

	Solver(const Stop_Criterium& _stop_criterium)
		:gen_nb_temp(0), gen_nb_temp_alpha(0),
		 accept_temp(0), accept_temp_alpha(0),
		 diff_scaling(0),

		 stop_criterium(_stop_criterium),

		 max_iter(0),

		 current_distance_to_target(0),
		 new_distance_to_target(0),
		 curr_solution(),
		 obj_func_history(0),obj_func_accepted(0)
	{
		stats.curr_stop_reason = UNITIALIZED;
		#ifdef LOG_RESULTS
			obj_func_history= Global::allocate_array<double>(MAX_ITER_HARD+1);
			obj_func_accepted= Global::allocate_array<bool>(MAX_ITER_HARD+1);
		#endif
	}

	~Solver(){
		#ifdef LOG_RESULTS
			if(obj_func_history!=0) Global::deallocate(obj_func_history);
			if(obj_func_accepted!=0) Global::deallocate(obj_func_accepted);
		#endif
	}

	bool is_ok(){
		#ifdef LOG_RESULTS
			bool ok = (obj_func_history!=0) && (obj_func_accepted!=0);
		#else
			bool ok = true;
		#endif
		return ok && curr_solution.is_ok();
	}

	void reset_params(const double& _gen_nb_temp, const double& _gen_nb_temp_alpha,
						const double& _accept_temp, const double& _accept_temp_alpha,
						const double& _diff_scaling,
						const int& _max_iter){
		gen_nb_temp = _gen_nb_temp;
		gen_nb_temp_alpha = _gen_nb_temp_alpha;
		accept_temp = _accept_temp;
		accept_temp_alpha = _accept_temp_alpha;

		diff_scaling = _diff_scaling;

		max_iter = _max_iter;

		current_distance_to_target = 0;
		new_distance_to_target = 0;

		vassert(gen_nb_temp >= 0); // only positive temperatures are allowed!
		vassert(accept_temp >= 0); // only positive temperatures are allowed!
		vassert(accept_temp_alpha > 0);
		vassert(gen_nb_temp_alpha > 0);
		vassert(accept_temp_alpha < 1);
		vassert(gen_nb_temp_alpha < 1);
		vassert(diff_scaling >= 0);

		init_solution(curr_solution);

		stats.curr_iter = 0;
		stats.curr_stop_reason = IDLE;

		stats.solutions_evaluated = 0;
		stats.better_solutions_accepted = 0;
		stats.eq_solutions_accepted = 0;
		stats.worse_solutions_accepted = 0;
		stats.solutions_rejected = 0;

		#ifdef LOG_RESULTS
			for (int i = 0; i < max_iter; ++i) {
				obj_func_history[i] = 0;
				obj_func_accepted[i] = false;
			}
		#endif
	}

	void solve(){

		vassert(stats.curr_stop_reason == IDLE);

		stats.curr_stop_reason = RUNNING;
		stop_reason_t curr_stop_reason = RUNNING;

		current_distance_to_target = calc_distance_to_target();
		
		#ifdef LOG_RESULTS
            obj_func_history[stats.curr_iter] = curr_solution.objective_val();
            obj_func_accepted[stats.curr_iter] = true;
		#endif

		while(curr_stop_reason == RUNNING){
			give_random_neighbour();

			new_distance_to_target = calc_distance_to_target();
			++(stats.solutions_evaluated);


			//these two could be at the end but are here to catch the correct value of the distances
			++(stats.curr_iter);
			curr_stop_reason = should_stop();

			#ifdef LOG_RESULTS
				obj_func_history[stats.curr_iter] = curr_solution.objective_val();
			#endif

			bool new_solution_accepted = accept();
			if(new_solution_accepted){
				current_distance_to_target = new_distance_to_target;
				#ifdef LOG_RESULTS
					obj_func_accepted[stats.curr_iter] = true;
				#endif
				curr_solution.accept_changes();
			}
			else {
			    ++(stats.solutions_rejected);
				#ifdef LOG_RESULTS
			    	obj_func_accepted[stats.curr_iter] = false;
				#endif
			    curr_solution.revert_changes();
			}

			gen_nb_temp *= gen_nb_temp_alpha;
			accept_temp *= accept_temp_alpha;
			vassert(gen_nb_temp >= 0); // only positive temperatures are allowed!
			vassert(accept_temp >= 0); // only positive temperatures are allowed!
		}

        commit_solution(get_solution());

        stats.curr_stop_reason = curr_stop_reason;

	}

	const Schedule<ObjectiveF>& get_solution() const{
		return curr_solution;
	}


	const volatile stats_t& get_stats() const{
		return stats;
	}


private:
	stop_reason_t should_stop() {
		if((max_iter > 0) && (stats.curr_iter >= max_iter)){
			return STOP_MAX_ITERATIONS;
		}
		else if(stop_criterium.met(new_distance_to_target, current_distance_to_target)){
			return STOP_CRIT_MET;
		}else{
			return RUNNING;
		}
	}


	bool accept(){
		vassert((current_distance_to_target >= 0) && (new_distance_to_target >= 0)); // distances are always positive

		double change = new_distance_to_target-current_distance_to_target;

		if(change < 0) {
			++(stats.better_solutions_accepted);
			return true;
		}

		if(change == 0) {
			++(stats.eq_solutions_accepted);
			return true;
		}

		const double FLOAT_LIMIT = 0.000000001;

		if(accept_temp < FLOAT_LIMIT){
			return false;
		}

		change *= diff_scaling;

		double probability = Math::fast_exp(-1.0*change/accept_temp);

		if((probability <= 0) || (probability > 1)){
			return false;
		}

		bool outmcome = (Rnd::random()%((int)(1.0/probability))==0);

		if(outmcome) ++(stats.worse_solutions_accepted);

		return outmcome;
	}

private:
	double gen_nb_temp;
	double gen_nb_temp_alpha;
	double accept_temp;
	double accept_temp_alpha;

	double diff_scaling;

	Stop_Criterium stop_criterium;

	int max_iter;

	double current_distance_to_target;
	double new_distance_to_target;

	Schedule<ObjectiveF> curr_solution;

public:
	double *obj_func_history;//[MAX_ITER_HARD+1];
	bool  *obj_func_accepted;//[MAX_ITER_HARD+1];

private:
	volatile stats_t stats;

};

};

#endif
