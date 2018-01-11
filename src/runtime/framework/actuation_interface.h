/*
 * actuator.h
 *
 *  Created on: May 10, 2017
 *      Author: tiago
 */

#ifndef __arm_rt_actuator_interface_h
#define __arm_rt_actuator_interface_h

#include <queue>
#include <map>

#include "types.h"
#include <core/core.h>
#include "actuator.h"


class ActuationInterface {

  public:

	/*
	 * Sets a new actuation valute
	 */
	template<ActuationType ACT_T,typename ResourceT>
	static void actuate(ResourceT *rsc, typename ActuationTypeInfo<ACT_T>::ValType val)
	{
		Actuator *act = Actuator::actForResource<ACT_T>(rsc);
		act->doSysActuation(rsc,val);
	}

	/*
	 * Gets the current actuation value.
	 * Value is stored at *val
	 */
	template<ActuationType ACT_T,typename ResourceT>
	static void actuationVal(ResourceT *rsc, typename ActuationTypeInfo<ACT_T>::ValType *val)
	{
		Actuator *act = Actuator::actForResource<ACT_T>(rsc);
		act->getSysActuation(rsc,val);
	}

	/*
	 * Gets the current actuation value.
	 * A new val object is returned
	 */
	template<ActuationType ACT_T,typename ResourceT>
	static typename ActuationTypeInfo<ACT_T>::ValType actuationVal(ResourceT *rsc)
	{
		Actuator *act = Actuator::actForResource<ACT_T>(rsc);
		typename ActuationTypeInfo<ACT_T>::ValType val;
		act->getSysActuation(rsc,&val);
		return val;
	}


	/*
	 * Updates the predictive models with a new actuation value
	 */
	template<ActuationType ACT_T,typename ResourceT>
	static void tryActuate(ResourceT *rsc, typename ActuationTypeInfo<ACT_T>::ValType val)
	{
		Actuator *act = Actuator::actForResource<ACT_T>(rsc);
		act->doModelActuation(rsc,val);
	}

	/*
	 * Gets the current actuatuation values from the predictive models
	 * Value is stored at *val
	 */
	template<ActuationType ACT_T,typename ResourceT>
	static void tryActuationVal(ResourceT *rsc, typename ActuationTypeInfo<ACT_T>::ValType *val)
	{
		Actuator *act = Actuator::actForResource<ACT_T>(rsc);
		act->getModelActuation(rsc,val);
	}

	/*
	 * Gets the current actuatuation values from the predictive models
	 * A new val object is returned
	 */
	template<ActuationType ACT_T,typename ResourceT>
	static typename ActuationTypeInfo<ACT_T>::ValType tryActuationVal(ResourceT *rsc)
	{
		Actuator *act = Actuator::actForResource<ACT_T>(rsc);
		typename ActuationTypeInfo<ACT_T>::ValType val;
		act->getModelActuation(rsc,&val);
		return val;
	}


	//Convenience functions that for passing resource as reference

	template<ActuationType ACT_T,typename ResourceT>
	static void actuate(ResourceT &rsc, typename ActuationTypeInfo<ACT_T>::ValType val)
	{
		actuate<ACT_T,ResourceT>(&rsc,val);
	}

	template<ActuationType ACT_T,typename ResourceT>
	static void actuationVal(ResourceT &rsc, typename ActuationTypeInfo<ACT_T>::ValType *val)
	{
		actuationVal<ACT_T,ResourceT>(&rsc,val);
	}

	template<ActuationType ACT_T,typename ResourceT>
	static typename ActuationTypeInfo<ACT_T>::ValType actuationVal(ResourceT &rsc)
	{
		return actuationVal<ACT_T,ResourceT>(&rsc);
	}

	template<ActuationType ACT_T,typename ResourceT>
	static void tryActuate(ResourceT &rsc, typename ActuationTypeInfo<ACT_T>::ValType val)
	{
		tryActuate<ACT_T,ResourceT>(&rsc,val);
	}

	template<ActuationType ACT_T,typename ResourceT>
	static void tryActuationVal(ResourceT &rsc, typename ActuationTypeInfo<ACT_T>::ValType *val)
	{
		tryActuationVal<ACT_T,ResourceT>(&rsc,val);
	}

	template<ActuationType ACT_T,typename ResourceT>
	static typename ActuationTypeInfo<ACT_T>::ValType tryActuationVal(ResourceT &rsc)
	{
		return tryActuationVal<ACT_T,ResourceT>(&rsc);
	}
};




#endif /* ACTUATOR_H_ */
