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

#include <map>

#include <runtime/interfaces/common/pal/pal_setup.h>

#include "actuator_commons.h"


core_freq_t closestStaticFreq(int freqMHz)
{
	static std::map<int,core_freq_t> freqMap;

	auto iter = freqMap.find(freqMHz);
	if(iter == freqMap.end()){
		int closestFreq = 0;
		int err = std::abs(freqMHz-(int)freqToValMHz_i((core_freq_t)closestFreq));
		for(int f = 1; f < SIZE_COREFREQ; ++f){
			int newErr = std::abs(freqMHz-(int)freqToValMHz_i((core_freq_t)f));
			if(newErr < err){
				err = newErr;
				closestFreq = f;
			}
		}
		freqMap[freqMHz] = (core_freq_t)closestFreq;
		return (core_freq_t)closestFreq;
	}
	else
		return iter->second;
}

core_freq_t maxStaticFreq(const freq_domain_info_t* domain)
{
	auto arch = domain->__vitaminslist_head_cores->arch;
	int freq = -1;
	for(int f = 0; f < SIZE_COREFREQ; ++f){
		if(pal_arch_has_freq(arch,(core_freq_t)f)){
			freq = f;
			break;
		}
	}
	if(freq == -1) arm_throw(DaemonSystemException,"Cannot find maximum frequency");
	return (core_freq_t)freq;
}

core_freq_t minStaticFreq(const freq_domain_info_t* domain)
{
	auto arch = domain->__vitaminslist_head_cores->arch;
	int freq = -1;
	for(int f = SIZE_COREFREQ-1; f >=0; --f){
		if(pal_arch_has_freq(arch,(core_freq_t)f)){
			freq = f;
			break;
		}
	}
	if(freq == -1) arm_throw(DaemonSystemException,"Cannot find minimum frequency");
	return (core_freq_t)freq;
}
