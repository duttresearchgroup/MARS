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

#ifndef __task_map_top_level_cmds_h
#define __task_map_top_level_cmds_h

#define SASOLVER_MAX_ITER_HARD	 10000

enum {
	SD_SOLVER_STATUS_UNITIALIZED,
	SD_SOLVER_STATUS_IDLE,
	SD_SOLVER_STATUS_RUNNING,
	SD_SOLVER_STATUS_STOP_MAX_ITERATIONS,
	SD_SOLVER_STATUS_STOP_CRIT_MET
};

typedef void* (*vit_allocator_f)(long unsigned int);
typedef void (*vit_deallocator_f)(void*);
typedef void (*vit_assertion_f)(int,const char *,int,const char *);


#ifndef __KERNEL__
	#define LOG_RESULTS
#endif

#endif


