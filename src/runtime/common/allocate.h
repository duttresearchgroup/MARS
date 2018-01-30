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

#ifndef __arm_rt_allocate_h
#define __arm_rt_allocate_h

#include <core/core.h>

//Prediciton, allocation, and remaping function defs

void vitamins_predict_allocate(model_sys_t *sys);
void vitamins_maping_test(model_sys_t *sys);
void vitamins_remap(model_sys_t *sys);
void vitamins_remap_overheadtest(model_sys_t *sys,int core0, int core1);


#endif

