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

#ifndef __arm_rt_sensing_window_defs_h
#define __arm_rt_sensing_window_defs_h


//Minimum lenght of a sensing window
//This is also the period used internally in the sensing module to sample
//sensors/counters that are not sampled at task context switch granularity
#define MINIMUM_WINDOW_LENGTH_MS 5

#define MAX_WINDOW_CNT 4

//SPECIAL codes that may be returned instead of a window id
#define WINDOW_ID_MASK 0xFAB00000

//Returned when the sensing module is exiting
#define WINDOW_EXIT ((int)(WINDOW_ID_MASK | 1))
//Invalid period when creating a window
#define WINDOW_INVALID_PERIOD ((int)(WINDOW_ID_MASK | 2))
//Max number of windows were created
#define WINDOW_MAX_NWINDOW ((int)(WINDOW_ID_MASK | 3))
//WIndow with the same period exists
#define WINDOW_EXISTS ((int)(WINDOW_ID_MASK | 4))

#endif
