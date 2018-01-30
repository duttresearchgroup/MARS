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

#ifndef __core_defs_helpers_h
#define __core_defs_helpers_h

#include "base/base.h"
#include "dvfs.h"
#include "power.h"

//helps iterate through enums in C++
#define for_enum(type,iterator,begin,end,inc)\
	for(type iterator = (type)begin; iterator < (type)end; iterator = (type)(iterator inc))


static inline core_freq_t valMHzToClosestFreq(core_arch_t currType, uint32_t val){
	int freq;
	uint32_t freq_val;
	core_freq_t min_freq=SIZE_COREFREQ,max_freq=SIZE_COREFREQ;
	uint32_t min_freq_val,max_freq_val;
	//first is highest
	for(freq = 0; freq < SIZE_COREFREQ; ++freq){
		if(!vitamins_arch_freq_available(currType,(core_freq_t)freq)) continue;
		freq_val = freqToValMHz_i((core_freq_t)freq);
		if(freq_val >= val) {
			max_freq = (core_freq_t)freq;
			max_freq_val = freq_val;
		}
		else{
			min_freq = (core_freq_t)freq;
			min_freq_val = freq_val;
			break;
		}
	}
	if(min_freq==SIZE_COREFREQ){
		BUG_ON(max_freq==SIZE_COREFREQ);
		return max_freq;
	}
	else if(max_freq==SIZE_COREFREQ){
		return min_freq;
	}
	else{
		if((max_freq_val - val) <= (val - min_freq_val)) return max_freq;
		else return min_freq;
	}
}

#endif
