#ifndef __arm_rt_framework_sensing_interface_h
#define __arm_rt_framework_sensing_interface_h

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

	/*
	 * Wrappers for the functions above supporting the resource being passed as ref
	 * or non const
	 */
	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(ResourceT *rsc, int wid)
	{ return sense<SEN_T,ResourceT>((const ResourceT*)rsc,wid); }

	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(ResourceT &rsc, int wid)
	{ return sense<SEN_T,ResourceT>(&rsc,wid); }

	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(const ResourceT &rsc, int wid)
	{ return sense<SEN_T,ResourceT>(&rsc,wid); }

	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType senseAgg(ResourceT *rsc, int wid)
	{ return senseAgg<SEN_T,ResourceT>((const ResourceT*)rsc,wid); }

	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType senseAgg(ResourceT &rsc, int wid)
	{ return senseAgg<SEN_T,ResourceT>(&rsc,wid); }

	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType senseAgg(const ResourceT &rsc, int wid)
	{ return senseAgg<SEN_T,ResourceT>(&rsc,wid); }


    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType sense(typename SensingTypeInfo<SEN_T>::ParamType p, ResourceT *rsc, int wid)
    { return sense<SEN_T,ResourceT>(p,(const ResourceT*)rsc,wid); }

    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType sense(typename SensingTypeInfo<SEN_T>::ParamType p, ResourceT &rsc, int wid)
    { return sense<SEN_T,ResourceT>(p,&rsc,wid); }

    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType sense(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT &rsc, int wid)
    { return sense<SEN_T,ResourceT>(p,&rsc,wid); }

    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseAgg(typename SensingTypeInfo<SEN_T>::ParamType p, ResourceT *rsc, int wid)
    { return senseAgg<SEN_T,ResourceT>(p,(const ResourceT*)rsc,wid); }

    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseAgg(typename SensingTypeInfo<SEN_T>::ParamType p, ResourceT &rsc, int wid)
    { return senseAgg<SEN_T,ResourceT>(p,&rsc,wid); }

    template<SensingType SEN_T,typename ResourceT>
    static typename SensingTypeInfo<SEN_T>::ValType senseAgg(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT &rsc, int wid)
    { return senseAgg<SEN_T,ResourceT>(p,&rsc,wid); }

};

#endif /* ACTUATOR_H_ */
