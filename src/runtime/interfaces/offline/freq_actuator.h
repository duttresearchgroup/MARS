#ifndef __arm_rt_offline_actuatorfreq_h
#define __arm_rt_offline_actuatorfreq_h


#include <runtime/framework/actuator.h>
#include <runtime/framework/actuator_commons.h>
#include <runtime/framework/types.h>

class OfflineFrequencyActuator : public Actuator, public FrequencyActuatorCommon {
public:
	OfflineFrequencyActuator(const sys_info_t &_info)
		:Actuator(ACT_FREQ_MHZ,_info),FrequencyActuatorCommon(_info)
	{
		//TODO
	}

};

#endif

