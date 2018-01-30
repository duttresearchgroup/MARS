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

#include "globals.h"
#include "globals_alloc.h"

#ifdef __KERNEL__
void* operator new (long unsigned int count, void* ptr ){
	return ptr;
}
#endif

namespace SASolverImpl {

namespace Global{

namespace Global_Internal{
vit_allocator_f allocator = 0;
vit_deallocator_f deallocator = 0;
vit_assertion_f assertor = 0;
}

namespace Input_Params {
	int MAX_N_CPUS = 0;
	int MAX_N_TASKS = 0;

	int N_TASKS = 0;
	int N_CPUS = 0;

	int MAPPING_SIZE = 0;

	//Controls the number of tasks that may be affected when generating a new solution,
	//which is N_TASKS*TASK_EXPLORATION_FACTOR
	// 0 means only one tasks may be affected
	double TASK_EXPLORATION_FACTOR = 0;
};
Input_Data* _input_data = 0;
Output_Data* _output_data = 0;
Constraints*  _constraints = 0;


void deallocate(void* ptr){
	(*Global_Internal::deallocator)(ptr);
}

bool create_globals(vit_allocator_f allocator, vit_deallocator_f deallocator, vit_assertion_f assertor,
		            long unsigned int max_num_cpus, long unsigned int max_num_tasks){
	Global_Internal::allocator = allocator;
	Global_Internal::deallocator = deallocator;
	Global_Internal::assertor = assertor;

	if(Global_Internal::allocator == 0) return false;
	if(Global_Internal::deallocator == 0) return false;
	if(Global_Internal::assertor == 0) return false;

	Input_Params::MAX_N_TASKS = max_num_tasks;
	Input_Params::MAX_N_CPUS = max_num_cpus;
	vassertSTR(Input_Params::MAX_N_TASKS>0);
	vassertSTR(Input_Params::MAX_N_CPUS>0);

	_input_data = allocate<Input_Data>();
	if(_input_data == 0) return false;

	_output_data = allocate<Output_Data>();
	if(_output_data == 0) return false;

	_constraints = allocate<Constraints>();
	if(_constraints == 0) return false;

	_input_data->task_per_cpu_data = allocate_matrix_obj<pred_task_per_cpu_input_matrix_t>(Input_Params::MAX_N_TASKS,Input_Params::MAX_N_CPUS);
	if(_input_data->task_per_cpu_data == 0) return false;

	_input_data->cpu_static_data = allocate_array_obj<cpu_static_input_matrix_t>(Input_Params::MAX_N_CPUS);
	if(_input_data->cpu_static_data == 0) return false;

	_output_data->task_mapping = allocate_matrix<task_t>(Input_Params::MAX_N_CPUS,Input_Params::MAX_N_TASKS);
	if(_output_data->task_mapping == 0) return false;

	_output_data->task_pred_load = allocate_array<double>(Input_Params::MAX_N_TASKS);
	if(_output_data->task_mapping == 0) return false;

	return true;
}

void destroy_globals(){
	if(_input_data != 0){
		if(_input_data->task_per_cpu_data != 0) {
			deallocate_matrix_obj<pred_task_per_cpu_input_matrix_t>(_input_data->task_per_cpu_data,Input_Params::MAX_N_TASKS,Input_Params::MAX_N_CPUS);
		}
		if(_input_data->cpu_static_data != 0){
			deallocate_array_obj(_input_data->cpu_static_data,Input_Params::MAX_N_CPUS);
		}

		deallocate(_input_data);
		_input_data = 0;
	}

	if(_output_data != 0){

		if(_output_data->task_mapping != 0) {
			deallocate_matrix<task_t>(_output_data->task_mapping,Input_Params::MAX_N_CPUS,Input_Params::MAX_N_TASKS);
		}

		if(_output_data->task_pred_load != 0) {
			deallocate(_output_data->task_pred_load);
		}

		deallocate(_output_data);
		_output_data = 0;
	}

	if(_constraints != 0) {
		deallocate(_constraints);
		_constraints = 0;
	}


	Input_Params::MAX_N_CPUS = 0;
	Input_Params::MAX_N_TASKS = 0;


	Global_Internal::allocator = 0;
	Global_Internal::deallocator = 0;
	Global_Internal::assertor = 0;
}


Input_Data& input_data(){
	return *_input_data;
}
Output_Data& output_data(){
	return *_output_data;
}
Constraints& constraints(){
	return *_constraints;
}


};

};
