#ifndef __arm_rt_actuation_interface_h
#define __arm_rt_actuation_interface_h

//has IS_LINUX_PLAT / IS_OFFLINE_PLAT
#include <runtime/interfaces/common/pal/pal_setup.h>

#include "linux/freq_actuator.h"
#include "linux/idledomain_actuator.h"
#include "offline/freq_actuator.h"
#include "offline/idledomain_actuator.h"

#if defined(IS_LINUX_PLAT)
	typedef LinuxFrequencyActuator FrequencyActuator;
	typedef LinuxIdleDomainActuator IdleDomainActuator;
#elif defined(IS_OFFLINE_PLAT)
	typedef OfflineFrequencyActuator FrequencyActuator;
	typedef OfflineIdleDomainActuator IdleDomainActuator;
#else
#error "Platform not properly defined"
#endif

#endif

