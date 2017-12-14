#ifndef __arm_rt_linux_actuatorfreq_h
#define __arm_rt_linux_actuatorfreq_h


#include <runtime/framework/types.h>
#include <runtime/framework/actuator.h>
#include <runtime/framework/actuator_commons.h>

#include "cpufreq.h"

class LinuxFrequencyActuator : public Actuator, public FrequencyActuatorCommon {

private:
	CpuFreq _cpufreq;

protected:
	void implSystemMode(){
		_cpufreq.scaling_governor("ondemand");
	}
	void implSystemMode(const std::string &arg){
		_cpufreq.scaling_governor(arg);
	}
	void implFrameworkMode(){
		_cpufreq.scaling_governor("userspace");
	}

public:
	LinuxFrequencyActuator(const sys_info_t &_info)
		:Actuator(ACT_FREQ_MHZ,_info),FrequencyActuatorCommon(_info),_cpufreq(_info)
	{
		for(int i = 0; i < _info.freq_domain_list_size; ++i)
			setActForResource(&(_info.freq_domain_list[i]));
	}

	~LinuxFrequencyActuator()
	{
		//pinfo("%s called\n",__PRETTY_FUNCTION__);
	}

	void doSysActuation(freq_domain_info_t *rsc, core_freq_t val){\
		_cpufreq.scaling_setspeed(rsc,val);
	}
	void getSysActuation(freq_domain_info_t *rsc, core_freq_t *val){\
		int freq_mhz = _cpufreq.scaling_cur_freq(rsc);
		*val = closestFreq(freq_mhz);
	}
	void doSysActuation(freq_domain_info_t *rsc, int val_mhz){\
		_cpufreq.scaling_setspeed(rsc,val_mhz);
	}
	void getSysActuation(freq_domain_info_t *rsc, int *val_mhz){\
		*val_mhz = _cpufreq.scaling_cur_freq(rsc);
	}


};

#endif

