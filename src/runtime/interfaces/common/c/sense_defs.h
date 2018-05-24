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

#ifndef __arm_rt_sense_defs_h
#define __arm_rt_sense_defs_h

//////////////////////////////////////////
//Base sensing structs definitions////////
//////////////////////////////////////////

#include "../pal/defs.h"

#include "perfcnts.h"
#include "sensing_window_defs.h"
#include "task_beat_data.h"

////////////////////////////
//info sensed for each task

typedef struct {
	//from PMU perf counters
    uint64_t perfcnts[MAX_PERFCNTS];

    //from the sensing hook parameter
    uint64_t nvcsw;
    uint64_t nivcsw;

    //from jiffies
    uint64_t time_busy_ms;
    uint64_t time_total_ms;//time_total is only updated at epochs
} perf_data_perf_counters_t;

#define get_perfcnt(counters,perfcnt) ((counters).perfcnts[vitsdata->perfcnt_to_idx_map[(perfcnt)]])

//sensed data for each task and each sensing window
struct perf_data_task_struct {
    perf_data_perf_counters_t perfcnt;//only valid for the latest epoch this task executed
    uint64_t					beats[MAX_BEAT_DOMAINS];//if the task is using the beats interface, this records the # of beats in the window in each domain
    int 						last_cpu_used;//tells in which cpu the task executed recently
};
typedef struct perf_data_task_struct perf_data_task_t;

//this is the data structure allocated for each task created after the module is loaded
struct tracked_task_data_struct {
	//kernel task info
	#define TASK_NAME_SIZE 16
	uint32_t this_task_pid;//we keep this here so we have the info after *this_task ends
	char this_task_name[TASK_NAME_SIZE];
	bool task_finished;

	int task_idx;//idx in the list of created tasks. Also used to get this task sensed data from the sensing window

	//may not be valid if the task does not use the beat library
	task_beat_data_t beats[MAX_BEAT_DOMAINS];//this info is updated using IOCTL
	int num_beat_domains;
	bool parent_has_beats;

    model_task_t *tsk_model;//this pointer should only be valid in the user-level daemon address space
};
typedef struct tracked_task_data_struct tracked_task_data_t;

/////////////////////////////
//info sensed for each cpu
//sums up info sensed for all tasks on this cpu
//but the datastruct is the same used per task
struct perf_data_cpu_struct {
    perf_data_perf_counters_t perfcnt;//cpu perfcnts
    uint64_t beats[MAX_BEAT_DOMAINS];//sums up all beats for all tasks in this cpu. NOT meanifull if tasks with different beat semantics run in the same cpu
};
typedef struct perf_data_cpu_struct perf_data_cpu_t;

//////////////////////////////////////////////////////////////////////////////////////////
//info sensed for each freq domain (updated at epochs)
//note: these are different from the other freq. counters
//these are updated at DVFS epochs and account for the overall freq., including idle time
typedef struct {
	uint64_t avg_freq_mhz_acc;
    uint64_t time_ms_acc;
    uint64_t last_update_time_ms;
} perf_data_freq_domain_t;


// helper funcitons

//begin must be a value read before end
//check for overflow
//assumes counter wraps around when overflow
//assumes it has overflown only once
//checks this and fix
static inline uint32_t counter_diff_32(uint32_t end, uint32_t begin)
{
	//if(begin > end) return (0xFFFFFFFF - begin) + end;
	//else return end-begin;
	return end-begin;
}


#endif

