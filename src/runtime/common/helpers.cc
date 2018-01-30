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


