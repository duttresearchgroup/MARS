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
	template<actuation_type ACT_T,typename ResourceT>
	static void actuate(ResourceT *rsc, typename actuation_type_val<ACT_T>::type val)
	{
		Actuator *act = Actuator::actForResource<ACT_T>(rsc);
		act->doSysActuation(rsc,val);
	}

	/*
	 * Gets the current actuation value.
	 * Value is stored at *val
	 */
	template<actuation_type ACT_T,typename ResourceT>
	static void actuationVal(ResourceT *rsc, typename actuation_type_val<ACT_T>::type *val)
	{
		Actuator *act = Actuator::actForResource<ACT_T>(rsc);
		act->getSysActuation(rsc,val);
	}

	/*
	 * Gets the current actuation value.
	 * A new val object is returned
	 */
	template<actuation_type ACT_T,typename ResourceT>
	static typename actuation_type_val<ACT_T>::type actuationVal(ResourceT *rsc)
	{
		Actuator *act = Actuator::actForResource<ACT_T>(rsc);
		typename actuation_type_val<ACT_T>::type val;
		act->getSysActuation(rsc,&val);
		return val;
	}


	/*
	 * Updates the predictive models with a new actuation value
	 */
	template<actuation_type ACT_T,typename ResourceT>
	static void tryActuate(ResourceT *rsc, typename actuation_type_val<ACT_T>::type val)
	{
		Actuator *act = Actuator::actForResource<ACT_T>(rsc);
		act->doModelActuation(rsc,val);
	}

	/*
	 * Gets the current actuatuation values from the predictive models
	 * Value is stored at *val
	 */
	template<actuation_type ACT_T,typename ResourceT>
	static void tryActuationVal(ResourceT *rsc, typename actuation_type_val<ACT_T>::type *val)
	{
		Actuator *act = Actuator::actForResource<ACT_T>(rsc);
		act->getModelActuation(rsc,val);
	}

	/*
	 * Gets the current actuatuation values from the predictive models
	 * A new val object is returned
	 */
	template<actuation_type ACT_T,typename ResourceT>
	static typename actuation_type_val<ACT_T>::type tryActuationVal(ResourceT *rsc)
	{
		Actuator *act = Actuator::actForResource<ACT_T>(rsc);
		typename actuation_type_val<ACT_T>::type val;
		act->getModelActuation(rsc,&val);
		return val;
	}


	//Convenience functions that for passing resource as reference

	template<actuation_type ACT_T,typename ResourceT>
	static void actuate(ResourceT &rsc, typename actuation_type_val<ACT_T>::type val)
	{
		actuate<ACT_T,ResourceT>(&rsc,val);
	}

	template<actuation_type ACT_T,typename ResourceT>
	static void actuationVal(ResourceT &rsc, typename actuation_type_val<ACT_T>::type *val)
	{
		actuationVal<ACT_T,ResourceT>(&rsc,val);
	}

	template<actuation_type ACT_T,typename ResourceT>
	static typename actuation_type_val<ACT_T>::type actuationVal(ResourceT &rsc)
	{
		return actuationVal<ACT_T,ResourceT>(&rsc);
	}

	template<actuation_type ACT_T,typename ResourceT>
	static void tryActuate(ResourceT &rsc, typename actuation_type_val<ACT_T>::type val)
	{
		tryActuate<ACT_T,ResourceT>(&rsc,val);
	}

	template<actuation_type ACT_T,typename ResourceT>
	static void tryActuationVal(ResourceT &rsc, typename actuation_type_val<ACT_T>::type *val)
	{
		tryActuationVal<ACT_T,ResourceT>(&rsc,val);
	}

	template<actuation_type ACT_T,typename ResourceT>
	static typename actuation_type_val<ACT_T>::type tryActuationVal(ResourceT &rsc)
	{
		return tryActuationVal<ACT_T,ResourceT>(&rsc);
	}
};




#endif /* ACTUATOR_H_ */
