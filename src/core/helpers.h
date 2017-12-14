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
