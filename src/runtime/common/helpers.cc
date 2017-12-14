

#include <sched.h>

#include "helpers.h"

static inline bool _change_cpu(uint32_t pid, int next_cpu){
	cpu_set_t set;

		CPU_ZERO( &set );
		CPU_SET( next_cpu, &set );
		return sched_setaffinity( pid, sizeof( cpu_set_t ), &set ) != -1;
}

bool actuate_change_cpu(uint32_t pid, int next_cpu) { return _change_cpu(pid,next_cpu); }

bool actuate_change_cpu(tracked_task_data_t *kern_tsk, int next_cpu){
	return _change_cpu(kern_tsk->this_task_pid,next_cpu);
}


