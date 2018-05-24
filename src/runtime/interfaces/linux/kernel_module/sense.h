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

#ifndef __arm_rt_sense_h
#define __arm_rt_sense_h

#include "core.h"
#include "sense_data.h"

//Sensing functions definitions

bool sense_create_mechanisms(sys_info_t *sys);
void sense_init(sys_info_t *sys);
void sense_begin(sys_info_t *sys);
void sense_stop(sys_info_t *sys);
void sense_cleanup(sys_info_t *sys);
void sense_destroy_mechanisms(sys_info_t *sys);

//used to check the current frequency (could also be used to set a frequency)
void minimum_sensing_window(sys_info_t *sys);
//sums-up the sensed information of currently active tasks; sets task_list_size
void sense_window(sys_info_t *sys, int wid);

//sensing options
void set_per_task_sensing(bool val);

bool trace_perf_counter(perfcnt_t pc);
bool trace_perf_counter_reset(void);

private_hook_data_t* add_created_task(struct task_struct *tsk);

//copy task beat info from the internal struct accecible by the task to the sensed data struct
bool update_task_beat_info(pid_t pid);

#endif

