
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
