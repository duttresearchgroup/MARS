
#include <map>

#include <runtime/interfaces/common/pal/pal_setup.h>

#include "actuator_commons.h"


core_freq_t FrequencyActuatorCommon::closestValidFreq(int freqMHz)
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

void FrequencyActuatorCommon::_identify_sys(const sys_info_t & sys_info)
{
	for(int domain_id = 0; domain_id < sys_info.freq_domain_list_size; ++domain_id){
		auto arch = sys_info.freq_domain_list[domain_id].__vitaminslist_head_cores->arch;
		_freq_max_mhz[domain_id] = -1;
		for(int f = 0; f < SIZE_COREFREQ; ++f){
			if(pal_arch_has_freq(arch,(core_freq_t)f)){
				_freq_max_mhz[domain_id] = freqToValMHz_i((core_freq_t)f);
				break;
			}
		}
		if(_freq_max_mhz[domain_id] == -1) arm_throw(DaemonSystemException,"Cannot find maximum frequency");

		_freq_min_mhz[domain_id] = -1;
		for(int f = SIZE_COREFREQ-1; f >=0; --f){
			if(pal_arch_has_freq(arch,(core_freq_t)f)){
				_freq_min_mhz[domain_id] = freqToValMHz_i((core_freq_t)f);
				break;
			}
		}
		if(_freq_min_mhz[domain_id] == -1) arm_throw(DaemonSystemException,"Cannot find minimum frequency");
	}
}
