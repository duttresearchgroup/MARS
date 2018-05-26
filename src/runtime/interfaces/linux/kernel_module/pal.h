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

#ifndef __arm_rt_pal_h
#define __arm_rt_pal_h

#include "../common/pal/pal_setup.h"
#include "../../common/c/perfcnts.h"

/////////////////////////////////////////
//platform specific functions

//start/stop perf sense on the current cpu. Despite taking the cpu as argument, the value should always be == the current cpu
void start_perf_sense(int cpu);
void stop_perf_sense(int cpu);

//Call once during plat setup to set which counters to be enabled when start_perf_sense is called.
//This should be called before enabling the hooks.
//Only the vit_available_perfcnt func should call this
void plat_enable_perfcnt(perfcnt_t perfcnt);
void plat_reset_perfcnts(void);//undo any call to enable

//returns the current num of enabled perf. counts and the maximum
int plat_enabled_perfcnts(void);
int plat_max_enabled_perfcnts(void);

//read counters from the current cpu. Despite taking the cpu as argument, the value should always be == the current cpu
uint32_t read_perfcnt(int cpu,perfcnt_t perfcnt);



#endif

