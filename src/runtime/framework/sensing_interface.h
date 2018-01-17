#ifndef __arm_rt_framework_sensing_interface_h
#define __arm_rt_framework_sensing_interface_h

#include <type_traits>

#include "types.h"
#include <core/core.h>


class SensingInterface {

  public:

	/*
	 * Returns the sensed value for given window
	 */
	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(const ResourceT *rsc, int wid);

	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc, int wid);


	/*
	 * Returns aggregated sensed value for the
	 * current and previous instances of the given window
	 */
	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType senseAgg(const ResourceT *rsc, int wid);

	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType senseAgg(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc, int wid);
};

#endif /* ACTUATOR_H_ */
