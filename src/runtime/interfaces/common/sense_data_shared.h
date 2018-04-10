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

#ifndef __arm_rt_sense_data_shared_h
#define __arm_rt_sense_data_shared_h

//////////////////////////////////////////
//Sensing data definition         ////////
//////////////////////////////////////////

//this header is self-contained and does not have kernel includes. It can be used in both kernel and user-level stuff

#include "sense_defs.h"
#include "sensing_window_defs.h"
#include "pal/pal_setup.h"

//TODO unsafe XD
#define SECRET_WORD 0xCAFEBABE

struct perf_window_data_struct {
	//updated before when the sensing window period is done
	//no synch and the user-level task should be done reading these guys before the next perio expires
	perf_data_cpu_t			cpus[MAX_NR_CPUS];
	perf_data_freq_domain_t	freq_domains[MAX_NR_CPUS];

	perf_data_task_t			tasks[MAX_CREATED_TASKS];
};
typedef struct perf_window_data_struct perf_window_data_t;

struct perf_window_struct {
	//sensing window data
	//updated before when the sensing window period is done
	//user interface accesses these for all counter data
	//no synch and the user-level task should be done reading these guys before the next period expires
	perf_window_data_t curr;
    perf_window_data_t aggr;//aggregate values for all previous samples for this window.

	uint64_t		   			curr_sample_time_ms;
	uint64_t		   			prev_sample_time_ms;

	uint32_t num_of_samples;

	int created_tasks_cnt;

	//this window id
	int wid;
};
typedef struct perf_window_struct perf_window_t;

//single struct containing all the sensed data
//there is only one instance of this
struct perf_data_struct {
	uint32_t __checksum0;

	//sensing window global sensed data
	//the tasks' sensed data for each window is inside each task hook data
	perf_window_t sensing_windows[MAX_WINDOW_CNT];

	//Accumulated counters used to compute the windows above
	//Counters in __acc_cpus and __acc_tasks are continuously incremented every
	//context switch, while __acc_freq_domains is incremented every minimum_sensing_window().
    //Unsafe to read from user-level and private to the sensing module only.
	//Unsafe to read/write without the proper locks by anyone.
	//Notice __acc_cpus and __acc_tasks are double-buffered to avoid blocking
	//during context switches
    perf_data_cpu_t         __acc_cpus[MAX_NR_CPUS][2];
    perf_data_cpu_t         __acc_tasks[MAX_CREATED_TASKS][2];
    int                     __acc_tasks_last_cpu[MAX_CREATED_TASKS];
    perf_data_freq_domain_t __acc_freq_domains[MAX_NR_CPUS];

	// list of created tasks. Stores sensing info for each task.
	//As of now there is no dealloc because we keep al the sensed info even
	//after the task is done
	tracked_task_data_t created_tasks[MAX_CREATED_TASKS];
	//number of created tasks. Notice this may increment in the middle of a
	//sensing window, so sensing_windows[wid].created_tasks_cnt should be used
	//to get the number of tasks in the latest window
	int created_tasks_cnt;
	int __created_tasks_cnt_tmp;//used to handle race conditions during task creation. Do not read/write this

	uint32_t num_of_minimum_periods;//number of times the innermost sensing loop was executed
	uint32_t num_of_csw_periods[MAX_NR_CPUS];//number of time sensing was done during context switch on each cpu

	int number_of_cpus;
	uint32_t starttime_ms;
	uint32_t stoptime_ms;

	// perf counter mapping info
	int idx_to_perfcnt_map[MAX_PERFCNTS];
	int perfcnt_to_idx_map[SIZE_PERFCNT];
	int perfcnt_mapped_cnt;

	uint32_t __sysChecksum;
	uint32_t __checksum1;
};
typedef struct perf_data_struct perf_data_t;

static inline void set_perf_data_cksum(perf_data_t *data){
	data->__checksum0 = 0xDEADBEEF;
	data->__checksum1 = 0xBEEFDEAD;
}

static inline bool check_perf_data_cksum(perf_data_t *data){
	return (data->__checksum0 == 0xDEADBEEF) && (data->__checksum1 == 0xBEEFDEAD);
}

#endif

