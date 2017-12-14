#ifndef __arm_rt_sensing_module_h
#define __arm_rt_sensing_module_h

//has IS_LINUX_PLAT / IS_OFFLINE_PLAT
#include <runtime/interfaces/common/pal/pal_setup.h>

#include "linux/sensing_module.h"
#include "offline/sensing_module.h"

#if defined(IS_LINUX_PLAT)
	typedef LinuxSensingModule SensingModule;
#elif defined(IS_OFFLINE_PLAT)
	typedef OfflineSensingModule SensingModule;
#else
#error "Platform not properly defined"
#endif

#endif
