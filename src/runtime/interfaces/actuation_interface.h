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

#ifndef __arm_rt_actuation_interface_h
#define __arm_rt_actuation_interface_h

//has IS_LINUX_PLAT / IS_OFFLINE_PLAT
#include <runtime/interfaces/common/pal/pal_setup.h>

#include "linux/freq_actuator.h"
#include "linux/idledomain_actuator.h"
#include "linux/taskmap_actuator.h"
#include "offline/freq_actuator.h"
#include "offline/idledomain_actuator.h"
#include "offline/taskmap_actuator.h"

#if defined(IS_LINUX_PLAT)
	typedef LinuxFrequencyActuator FrequencyActuator;
	typedef LinuxTaskMapActuator TaskMapActuator;
	typedef LinuxIdleDomainActuator IdleDomainActuator;
#elif defined(IS_OFFLINE_PLAT)
	typedef OfflineFrequencyActuator FrequencyActuator;
	typedef OfflineTaskMapActuator TaskMapActuator;
	typedef OfflineIdleDomainActuator IdleDomainActuator;
#else
#error "Platform not properly defined"
#endif

#endif

