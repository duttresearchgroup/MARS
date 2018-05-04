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

#ifndef __common_pal_gem5_defs_h
#define __common_pal_gem5_defs_h

#define IS_LINUX_PLAT

// Currently we are using a Linux-gem5 that does not have the DVFS gov
//#define LINUX_HAS_CPUFREQ

#define MAX_NR_CPUS 64
#define MAX_CREATED_TASKS 64

#define MAX_PERFCNTS 64//maximum number of counters

// Daemons won't dump logs to kmsg
#define DAEMON_NO_KMSG


#endif
