#ifndef __arm_rt_framework_sensing_interface_h
#define __arm_rt_framework_sensing_interface_h

#include "types.h"
#include <core/core.h>


class SensingInterface {

  public:

	/*
	 * Returns the sensed value for given window
	 */
	template<sensing_type SEN_T,typename ResourceT>
	static typename sensing_type_val<SEN_T>::type sense(const ResourceT *rsc, int wid);


	/*
	 * Returns aggregated sensed value for the
	 * current and previous instances of the given window
	 */
	template<sensing_type SEN_T,typename ResourceT>
	static typename sensing_type_val<SEN_T>::type senseAgg(const ResourceT *rsc, int wid);

	/*
	 * Wrappers for the functions above supporting the resource being passed as ref
	 * or non const
	 */
	template<sensing_type SEN_T,typename ResourceT>
	static typename sensing_type_val<SEN_T>::type sense(ResourceT *rsc, int wid)
	{ return sense<SEN_T,ResourceT>((const ResourceT*)rsc,wid); }

	template<sensing_type SEN_T,typename ResourceT>
	static typename sensing_type_val<SEN_T>::type sense(ResourceT &rsc, int wid)
	{ return sense<SEN_T,ResourceT>(&rsc,wid); }

	template<sensing_type SEN_T,typename ResourceT>
	static typename sensing_type_val<SEN_T>::type sense(const ResourceT &rsc, int wid)
	{ return sense<SEN_T,ResourceT>(&rsc,wid); }

	template<sensing_type SEN_T,typename ResourceT>
	static typename sensing_type_val<SEN_T>::type senseAgg(ResourceT *rsc, int wid)
	{ return senseAgg<SEN_T,ResourceT>((const ResourceT*)rsc,wid); }

	template<sensing_type SEN_T,typename ResourceT>
	static typename sensing_type_val<SEN_T>::type senseAgg(ResourceT &rsc, int wid)
	{ return senseAgg<SEN_T,ResourceT>(&rsc,wid); }

	template<sensing_type SEN_T,typename ResourceT>
	static typename sensing_type_val<SEN_T>::type senseAgg(const ResourceT &rsc, int wid)
	{ return senseAgg<SEN_T,ResourceT>(&rsc,wid); }

};

#endif /* ACTUATOR_H_ */
