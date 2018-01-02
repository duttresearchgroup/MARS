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
	int *_freq_max_mhz;
	int	*_freq_min_mhz;

	void _identify_sys(const sys_info_t &sys_info);

	FrequencyActuatorCommon(const sys_info_t &sys_info)
		:_freq_max_mhz(new int[sys_info.freq_domain_list_size]),
		 _freq_min_mhz(new int[sys_info.freq_domain_list_size]){
		_identify_sys(sys_info);
	}

	~FrequencyActuatorCommon()
	{
		//pinfo("%s called\n",__PRETTY_FUNCTION__);
		delete[] _freq_max_mhz;
		delete[] _freq_min_mhz;
	}

public:

	int freqMax(const freq_domain_info_t* domain) { return _freq_max_mhz[domain->domain_id];}
	int freqMin(const freq_domain_info_t* domain) { return _freq_min_mhz[domain->domain_id];}
	int freqMid(const freq_domain_info_t* domain){ return (freqMax(domain)+freqMin(domain))/2;}

	int freqMax(const freq_domain_info_t& domain) { return freqMax(&domain);}
	int freqMin(const freq_domain_info_t& domain) { return freqMin(&domain);}
	int freqMid(const freq_domain_info_t& domain){ return freqMid(&domain);}


	/*
	 * TODO should actually call the set max from CpuFreq
	 */
	void freqMax(const freq_domain_info_t* domain,int setMaxMHz) { _freq_max_mhz[domain->domain_id] = setMaxMHz;}
	void freqMax(const freq_domain_info_t& domain,int setMaxMHz) { freqMax(&domain,setMaxMHz);}


public:
	static core_freq_t closestValidFreq(int freqMHz);
};


#endif /* ACTUATOR_H_ */
