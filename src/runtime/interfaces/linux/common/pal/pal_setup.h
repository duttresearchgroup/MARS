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

#ifndef __arm_rt_linux_pal_setup_h
#define __arm_rt_linux_pal_setup_h

// Is included in the linux module which does not have the include path
#include "../../../common/pal/defs.h"

//implemented at common/pal/plat_name/lin_setup_info.c
//for all platforms that used the linux interfaces
//Header good for both kernel C and user C++

CBEGIN

// Returns a pointer to the sys_info object describing the current platform
sys_info_t* pal_sys_info(int num_online_cpus);

CEND


#endif
