#ifndef __arm_rt_framework_sensing_interface_h
#define __arm_rt_framework_sensing_interface_h

#include "types.h"
#include <core/core.h>


class SensingInterface {

  public:

	/*
	 * Returns the sensed value
	 */
	template<sensing_type SEN_T,typename ResourceT>
	static typename sensing_type_val<SEN_T>::type sense(ResourceT *rsc, int wid);

	template<sensing_type SEN_T,typename ResourceT>
	static typename sensing_type_val<SEN_T>::type sense(ResourceT &rsc, int wid) { return sense<SEN_T>(&rsc,wid); }

};




#endif /* ACTUATOR_H_ */
