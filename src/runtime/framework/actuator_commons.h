/*
 * actuator.h
 *
 *  Created on: May 10, 2017
 *      Author: tiago
 */

#ifndef __arm_rt_actuator_commons_h
#define __arm_rt_actuator_commons_h


#include <core/core.h>

// Returns the closest valid core_freq_t for the
// given MHz freq value
//
// The new framework should be as independent as
// possible from <core/core.h> so this should
// be used to interface with older code only
core_freq_t closestStaticFreq(int freqMHz);

// Returns the maximum core_freq_t statically defined
// for the core_arch_t of the cores in the given
// frequency domain
//
// The new framework should be as independent as
// possible from <core/core.h> so this should
// be used to interface with older code only
core_freq_t maxStaticFreq(const freq_domain_info_t* domain);

// Returns the minimum core_freq_t statically defined
// for the core_arch_t of the cores in the given
// frequency domain
//
// The new framework should be as independent as
// possible from <core/core.h> so this should
// be used to interface with older code only
core_freq_t minStaticFreq(const freq_domain_info_t* domain);


#endif /* ACTUATOR_H_ */
