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

#ifndef __arm_rt_helpers_h
#define __arm_rt_helpers_h

#include <linux/kernel.h>
#include <linux/sched.h>

#include <base/base.h>
#include <runtime/interfaces/common/performance_data.h>

bool actuate_change_cpu(tracked_task_data_t *kern_tsk, int next_cpu);
bool actuate_change_cpu(uint32_t pid, int next_cpu);

#endif

