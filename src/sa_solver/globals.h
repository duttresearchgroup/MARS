#ifndef __sasolver_io_h
#define __sasolver_io_h

#ifndef __KERNEL__
	#include <cassert> // will use assert to check certain values
	#include <iostream>
#endif

#include "solver_defines.h"

namespace SASolverImpl {

namespace Global{

typedef int task_t;

namespace Input_Params {
extern int MAX_N_CPUS;
extern int MAX_N_TASKS;

extern int N_TASKS;
extern int N_CPUS;
	
extern int MAPPING_SIZE;

	//Controls the number of tasks that may be affected when generating a new solution,
	//which is N_TASKS*TASK_EXPLORATION_FACTOR
	// 0 means only one tasks may be affected
extern double TASK_EXPLORATION_FACTOR;
};

typedef struct {
	double active_ips;
	double active_power;
	double demand;
} pred_task_per_cpu_input_matrix_t;

typedef struct {
	double idle_power;
	double idle_power_when_empty;
	double kernel_idle_load;
	double kernel_active_power;
} cpu_static_input_matrix_t;

struct Input_Data {
	pred_task_per_cpu_input_matrix_t **task_per_cpu_data;//[Global::Input_Params::MAX_N_TASKS][Global::Input_Params::MAX_N_CPUS];
	cpu_static_input_matrix_t 		 *cpu_static_data;//[Global::Input_Params::MAX_N_CPUS];
};

struct Output_Data {
	task_t **task_mapping;//[Global::Input_Params::MAX_N_CPUS][Global::Input_Params::MAX_N_TASKS];
	double *task_pred_load;
};

struct Constraints {
	double total_power;
};

namespace Global_Internal{
	extern vit_assertion_f assertor;
}

bool create_globals(vit_allocator_f allocator, vit_deallocator_f deallocator, vit_assertion_f assertor,
		            long unsigned int max_num_cpus, long unsigned int max_num_tasks);

void destroy_globals();

Input_Data& input_data();
Output_Data& output_data();
Constraints& constraints();


};

#ifndef NDEBUG

template<typename DataType>
void inline vassert_dump(bool exp,bool hasData,DataType data){
#ifndef __KERNEL__
	if(!exp){
		std::cerr << "SASolver: a assert() is going to fail. Dumping some more data:\n";
		std::cerr << "MAX_N_CPUS="<<(int)Global::Input_Params::MAX_N_CPUS<<"\n";
		std::cerr << "MAX_N_TASKS="<<(int)Global::Input_Params::MAX_N_TASKS<<"\n";
		std::cerr << "N_CPUS="<<(int)Global::Input_Params::N_CPUS<<"\n";
		std::cerr << "N_TASKS="<<(int)Global::Input_Params::N_TASKS<<"\n";
		std::cerr << "TASK_EXPLORATION_FACTOR="<<Global::Input_Params::TASK_EXPLORATION_FACTOR<<"\n";

		for (int task = 0; task < Global::Input_Params::N_TASKS; ++task){
			for (int cpu = 0; cpu < Global::Input_Params::N_CPUS; ++cpu){
				std::cerr << "Global::input_data().task_per_cpu_data["<<task<<","<<cpu<<"]=(active_ips="<<Global::input_data().task_per_cpu_data[task][cpu].active_ips<<", dyn_power="<<Global::input_data().task_per_cpu_data[task][cpu].dyn_power<<", demand="<<Global::input_data().task_per_cpu_data[task][cpu].demand<<")\n";
			}
		}
		for (int cpu = 0; cpu < Global::Input_Params::N_CPUS; ++cpu)
			std::cerr << "Global::input_data().cpu_static_data["<<cpu<<"]=(static_power_idle="<<Global::input_data().cpu_static_data[cpu].static_power_idle<<",static_power_active="<<Global::input_data().cpu_static_data[cpu].static_power_active<<")\n";
		for (int cpu = 0; cpu < Global::Input_Params::N_CPUS; ++cpu)
		for (int task = 0; task < Global::Input_Params::N_TASKS; ++task)
			std::cerr << "Output_Data::task_mapping["<<cpu<<","<<task<<"]="<<Global::output_data().task_mapping[cpu][task]<<"\n";
		if(hasData){
			std::cerr << "Failed data value: "<<data<<"\n";
		}
	}
#endif
}

void inline vassert_dump(bool exp){
	vassert_dump(exp,false,0);
}

#define vassertX(val,valdata) vassert_dump(val,true,valdata);(*Global::Global_Internal::assertor)(val,0,0,0)

#define vassert(val) vassert_dump(val); (*Global::Global_Internal::assertor)(val,0,0,0)

#define vassertSTR(val) vassert_dump(val); (*Global::Global_Internal::assertor)(val, __FILE__,__LINE__,#val)

#else
#define vassert(val)
#define vassertX(val,valdata)
#define vassertSTR(val)
#endif

};

#endif
