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

#ifndef __arm_rt_actuator_commons_h
#define __arm_rt_actuator_commons_h


#include <base/base.h>

// Returns the closest valid core_freq_t for the
// given MHz freq value
//
// The new framework should be as independent as
// possible from <base/base.h> so this should
// be used to interface with older code only
core_freq_t closestStaticFreq(int freqMHz);

// Returns the maximum core_freq_t statically defined
// for the core_arch_t of the cores in the given
// frequency domain
//
// The new framework should be as independent as
// possible from <base/base.h> so this should
// be used to interface with older code only
core_freq_t maxStaticFreq(const freq_domain_info_t* domain);

// Returns the minimum core_freq_t statically defined
// for the core_arch_t of the cores in the given
// frequency domain
//
// The new framework should be as independent as
// possible from <base/base.h> so this should
// be used to interface with older code only
core_freq_t minStaticFreq(const freq_domain_info_t* domain);


#endif /* ACTUATOR_H_ */
