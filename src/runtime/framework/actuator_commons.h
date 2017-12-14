/*
 * actuator.h
 *
 *  Created on: May 10, 2017
 *      Author: tiago
 */

#ifndef __arm_rt_actuator_commons_h
#define __arm_rt_actuator_commons_h


#include <core/core.h>


class FrequencyActuatorCommon {
protected:
	core_freq_t *_freq_max;
	core_freq_t	*_freq_min;

	void _identify_sys(const sys_info_t &sys_info);

	FrequencyActuatorCommon(const sys_info_t &sys_info)
		:_freq_max(new core_freq_t[sys_info.freq_domain_list_size]),
		 _freq_min(new core_freq_t[sys_info.freq_domain_list_size]){
		_identify_sys(sys_info);
	}

	~FrequencyActuatorCommon()
	{
		//pinfo("%s called\n",__PRETTY_FUNCTION__);
		delete[] _freq_max;
		delete[] _freq_min;
	}

public:

	core_freq_t freqMax(const freq_domain_info_t* domain) { return _freq_max[domain->domain_id];}
	core_freq_t freqMin(const freq_domain_info_t* domain) { return _freq_min[domain->domain_id];}
	core_freq_t freqMid(const freq_domain_info_t* domain){ return closestFreq((freqToValMHz_i(freqMax(domain))+freqToValMHz_i(freqMin(domain)))/2);}

	core_freq_t freqMax(const freq_domain_info_t& domain) { return freqMax(&domain);}
	core_freq_t freqMin(const freq_domain_info_t& domain) { return freqMin(&domain);}
	core_freq_t freqMid(const freq_domain_info_t& domain){ return freqMid(&domain);}

	/*
	 * TODO should actually call the set max from CpuFreq
	 */
	void freqMax(const freq_domain_info_t* domain,core_freq_t setMax) { _freq_max[domain->domain_id] = setMax;}
	void freqMax(const freq_domain_info_t& domain,core_freq_t setMax) { freqMax(&domain,setMax);}


public:
	static core_freq_t closestFreq(int freqMHz);
};


#endif /* ACTUATOR_H_ */
