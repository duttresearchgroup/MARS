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

#ifndef __schedule_h
#define __schedule_h

#include "globals.h"
#include "globals_alloc.h"
#ifndef __KERNEL__
	#include <sstream>
#endif

namespace SASolverImpl {

template<typename ObjectiveF_t>
class Schedule {
private:
	typedef Global::task_t task_t;
	typedef int position_t;

	ObjectiveF_t objective;
	int change_buffer_size;

	task_t **task_mapping;//[Global::Input_Params::MAX_N_CPUS][Global::Input_Params::MAX_N_TASKS];
	position_t *task_positions;//[Global::Input_Params::MAX_N_TASKS];


	struct tasks_change {
		tasks_change() :task(0), curr_position(0), new_position(0){}
		tasks_change(int _task, int _curr_position, int _new_position) :task(_task), curr_position(_curr_position), new_position(_new_position){}
		int task;
		int curr_position;
		int new_position;
	};
	tasks_change *change_buffer;//[Global::Input_Params::MAX_N_TASKS];

	Global::Output_Data &output_data;

public:

	Schedule()
		:objective(),
		 change_buffer_size(0),
		 task_mapping(0),
		 task_positions(0),
		 change_buffer(0),
		 output_data(Global::output_data())
	{

		task_mapping = Global::allocate_matrix<task_t>(Global::Input_Params::MAX_N_CPUS,Global::Input_Params::MAX_N_TASKS);

		task_positions = Global::allocate_array<position_t>(Global::Input_Params::MAX_N_TASKS);

		change_buffer = Global::allocate_array_obj<tasks_change>(Global::Input_Params::MAX_N_TASKS);
	}

	~Schedule(){
		if(task_mapping != 0) Global::deallocate_matrix<task_t>(task_mapping,Global::Input_Params::MAX_N_CPUS,Global::Input_Params::MAX_N_TASKS);
		if(task_positions != 0) Global::deallocate(task_positions);
		if(change_buffer != 0) Global::deallocate_array_obj(change_buffer,Global::Input_Params::MAX_N_TASKS);
	}

	Schedule(const Schedule<ObjectiveF_t> &other)
		:objective(other.objective),
		 change_buffer_size(other.change_buffer_size),
		 task_mapping(0),
		 task_positions(0),
		 change_buffer(0),
		 output_data(Global::output_data())
	{

		task_mapping = Global::allocate_matrix<task_t>(Global::Input_Params::MAX_N_CPUS,Global::Input_Params::MAX_N_TASKS);

		task_positions = Global::allocate_array<position_t>(Global::Input_Params::MAX_N_TASKS);

		change_buffer = Global::allocate_array_obj<tasks_change>(Global::Input_Params::MAX_N_TASKS);

		vassert(is_ok());

		for (int cpu = 0; cpu < Global::Input_Params::MAX_N_CPUS; ++cpu) {
			for (int task = 0; task < Global::Input_Params::MAX_N_TASKS; ++task) {
				task_mapping[cpu][task] = other.task_mapping[cpu][task];
			}
		}

		for (int task = 0; task < Global::Input_Params::MAX_N_TASKS; ++task) {
			task_positions[task] = other.task_positions[task];
			change_buffer[task] = other.change_buffer[task];
		}
	}

	Schedule<ObjectiveF_t>&  operator=( const Schedule<ObjectiveF_t>& other ) {
		objective = other.objective;
		change_buffer_size = other.change_buffer_size;

		//the matrices should have been allocates and different from other
		vassert(is_ok());

		vassert(task_mapping!=other.task_mapping);
		for (int cpu = 0; cpu < Global::Input_Params::MAX_N_CPUS; ++cpu) {
			vassert(task_mapping[cpu]!=other.task_mapping[cpu]);

			for (int task = 0; task < Global::Input_Params::MAX_N_TASKS; ++task) {
				task_mapping[cpu][task] = other.task_mapping[cpu][task];
			}
		}

		vassert(task_positions!=other.task_positions);
		vassert(change_buffer!=other.change_buffer);

		for (int task = 0; task < Global::Input_Params::MAX_N_TASKS; ++task) {
			task_positions[task] = other.task_positions[task];
			change_buffer[task] = other.change_buffer[task];
		}

		return *this;
	}

	bool is_ok(){
		if (task_mapping==0) return false;
		for(int i = 0; i < Global::Input_Params::MAX_N_CPUS; ++i){
			if(task_mapping[i]==0) return false;
		}
		if (task_positions==0) return false;
		if (change_buffer==0) return false;
		return true && objective.is_ok();
	}

	template<typename CPU_IDX_T, typename TASK_IDX_T>
	task_t& operator()(CPU_IDX_T cpu_idx, TASK_IDX_T task_idx) {
		vassert(cpu_idx < Global::Input_Params::N_CPUS);
		vassert(task_idx < Global::Input_Params::N_TASKS);
		return task_mapping[cpu_idx][task_idx];
	}
	template<typename CPU_IDX_T, typename TASK_IDX_T>
	const task_t& operator()(CPU_IDX_T cpu_idx, TASK_IDX_T task_idx) const {
		vassert(cpu_idx < Global::Input_Params::N_CPUS);
		vassert(task_idx < Global::Input_Params::N_TASKS);
		return task_mapping[cpu_idx][task_idx];
	}

	const double& objective_val() const{
		return objective.value();
	}

	const double& objective_val(int idx) const{
		return objective.value(idx);
	}

	const position_t& task_position(int task_idx) const {
		vassert(task_idx < Global::Input_Params::N_TASKS);
		return task_positions[task_idx];
	}

#ifndef __KERNEL__
	friend std::ostream& operator<<( std::ostream &output,
			const Schedule &s)
	{
		output << "\n";
		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			for (int j = 0; j < Global::Input_Params::N_TASKS; ++j) {
				output << s(i,j);
				if(j < Global::Input_Params::N_TASKS-1) output << ", ";
			}
			output << "\n";
		}

		return output;
	}
#endif

	void read_from_ports(){
		int acc_cnt = 0;
		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			for (int j = 0; j < Global::Input_Params::N_TASKS; ++j) {
				task_t task = output_data.task_mapping[i][j];
				(*this)(i,j) = task;
				if(task != 0) {
					task_positions[task-1] = j+(i*Global::Input_Params::N_TASKS);
					++acc_cnt;
				}
			}
		}
		vassert((acc_cnt ==  Global::Input_Params::N_TASKS) && "Read sched is invalid");
		objective.update_new(*this);
		change_buffer_size = 0;
	}

	void write_to_ports() const{
		int acc_cnt = 0;
		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			int k = 0;
			for (int j = 0; j < Global::Input_Params::N_TASKS; ++j) {
				output_data.task_mapping[i][j] = 0;
				if((*this)(i,j) != 0){
					output_data.task_mapping[i][k] = (*this)(i,j);
					++k;
					++acc_cnt;
				}
			}
		}
		vassert((acc_cnt ==  Global::Input_Params::N_TASKS) && "Written sched is invalid");
	}

	void switch_task_position(int task, int curr_position, int new_position){
		_switch_task_position(task,curr_position, new_position, true);
	}

	void revert_changes(){
		while(change_buffer_size > 0) _revert_last_change();
	}

	void accept_changes(){
		vassert(change_buffer_size > 0);
		change_buffer_size = 0;
	}


private:

	void _revert_last_change(){
		vassert(change_buffer_size > 0);
		vassert(change_buffer_size <= Global::Input_Params::N_TASKS);

		--change_buffer_size;
		_switch_task_position(change_buffer[change_buffer_size].task,
				change_buffer[change_buffer_size].new_position,
				change_buffer[change_buffer_size].curr_position,
				false);
	}

	void _switch_task_position(int task, int curr_position, int new_position, bool log_change){

		if(log_change){
			vassert(change_buffer_size < Global::Input_Params::N_TASKS);

			change_buffer[change_buffer_size] = tasks_change(task, curr_position, new_position);
			++change_buffer_size;
		}

		if(curr_position == new_position) {
			//nothing to do
			return;
		}

		int curr_position_cpu = curr_position / Global::Input_Params::N_TASKS;
		int curr_position_task = curr_position % Global::Input_Params::N_TASKS;

		int new_position_cpu = new_position / Global::Input_Params::N_TASKS;
		int new_position_task = new_position % Global::Input_Params::N_TASKS;

		int switched_task = task_mapping[new_position_cpu][new_position_task];
		task_mapping[curr_position_cpu][curr_position_task] = switched_task;
		task_mapping[new_position_cpu][new_position_task] = task;

		task_positions[task-1] = new_position;
		if(switched_task != 0) task_positions[switched_task-1] = curr_position;

		objective.update_on_switch(task, curr_position_cpu, switched_task, new_position_cpu, *this);
	}
};

};

#endif



