/*
 * actuator.h
 *
 *  Created on: May 10, 2017
 *      Author: tiago
 */

#ifndef __arm_rt_actuator_h
#define __arm_rt_actuator_h

#include <queue>
#include <map>

#include "types.h"
#include <core/core.h>

#include <runtime/interfaces/sensed_data.h>

class ActuationInterface;

class Actuator {

  private:

	actuation_type _type;
	actuation_mode _mode;

  public:

	const sys_info_t &info;

  protected:

	void setActForResource(void *rsc);

	Actuator(actuation_type type,const sys_info_t &_info);

	virtual ~Actuator();

	virtual void implSystemMode(){
		arm_throw(ActuatorException,"%s not implemented!",__PRETTY_FUNCTION__);
	}
	virtual void implSystemMode(const std::string &arg){
		arm_throw(ActuatorException,"%s not implemented!",__PRETTY_FUNCTION__);
	}
	virtual void implFrameworkMode(){
		arm_throw(ActuatorException,"%s not implemented!",__PRETTY_FUNCTION__);
	}

  public:

	actuation_type type() { return _type;}

	void setSystemMode()
	{
		_mode = ACTMODE_SYSTEM;
		implSystemMode();
	}
	void setSystemMode(const std::string &arg)
	{
		_mode = ACTMODE_SYSTEM;
		implSystemMode(arg);
	}
	void setFrameworkMode(){
		_mode = ACTMODE_FRAMEWORK;
		implFrameworkMode();
	}

	actuation_mode mode() { return _mode;}

	bool systemMode() { return _mode==ACTMODE_SYSTEM;}
	bool frameworkMode() { return _mode==ACTMODE_FRAMEWORK;}

	#define ActuatorDefDefaultActuationFuncs(ResourceT,ActuationValT,...)\
	virtual void doSysActuation(ResourceT *rsc,ActuationValT val,##__VA_ARGS__){\
		arm_throw(ActuatorException,"%s not implemented!",__PRETTY_FUNCTION__);\
	}\
	virtual void getSysActuation(ResourceT *rsc,ActuationValT *val,##__VA_ARGS__){\
		arm_throw(ActuatorException,"%s not implemented!",__PRETTY_FUNCTION__);\
	}\
	virtual void doModelActuation(ResourceT *rsc,ActuationValT val,##__VA_ARGS__){\
		arm_throw(ActuatorException,"%s not implemented!",__PRETTY_FUNCTION__);\
	}\
	virtual void getModelActuation(ResourceT *rsc,ActuationValT *val,##__VA_ARGS__){\
		arm_throw(ActuatorException,"%s not implemented!",__PRETTY_FUNCTION__);\
	}


	/*
	 * Define here the actuation functions that actuators could implement
	 *
	 * ActuatorDefDefaultActuationFuncs(resource_type_to_actuate,
	 * 								    type_of_actuation_value,
	 * 								    optional_params)
	 */

	//used when setting frequency
	ActuatorDefDefaultActuationFuncs(freq_domain_info_t,
									 int);
	//used when setting idle cores
	ActuatorDefDefaultActuationFuncs(freq_domain_info_t,
									 int,
									 const SensedData& data, int wid, int cores_min=1);


};


class ActuationInterface {
	friend class Actuator;

  private:

	static std::map<actuation_type,std::map<void*,Actuator*>> _actuatorMap;

	template<actuation_type ACT_T,typename ResourceT>
	static Actuator* actForResource(ResourceT *rsc)
	{
		auto aux = _actuatorMap.find(ACT_T);
		if(aux == _actuatorMap.end())
			arm_throw(ActuatorException,"Actuator for resource %p,actuation type %d not set",rsc,ACT_T);
		else{
			auto aux2 = aux->second.find(rsc);
			if(aux2 == aux->second.end())
				arm_throw(ActuatorException,"Actuator for resource %p,actuation type %d not set",rsc,ACT_T);
			else
				return aux2->second;
		}
	}

  public:

	/*
	 * Sets a new actuation valute
	 */
	template<actuation_type ACT_T,typename ResourceT>
	static void actuate(ResourceT *rsc, typename actuation_type_val<ACT_T>::type val)
	{
		Actuator *act = actForResource<ACT_T>(rsc);
		act->doSysActuation(rsc,val);
	}

	/*
	 * Gets the current actuation value.
	 * Value is stored at *val
	 */
	template<actuation_type ACT_T,typename ResourceT>
	static void actuationVal(ResourceT *rsc, typename actuation_type_val<ACT_T>::type *val)
	{
		Actuator *act = actForResource<ACT_T>(rsc);
		act->getSysActuation(rsc,val);
	}

	/*
	 * Gets the current actuation value.
	 * A new val object is returned
	 */
	template<actuation_type ACT_T,typename ResourceT>
	static typename actuation_type_val<ACT_T>::type actuationVal(ResourceT *rsc)
	{
		Actuator *act = actForResource<ACT_T>(rsc);
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
		Actuator *act = actForResource<ACT_T>(rsc);
		act->doModelActuation(rsc,val);
	}

	/*
	 * Gets the current actuatuation values from the predictive models
	 * Value is stored at *val
	 */
	template<actuation_type ACT_T,typename ResourceT>
	static void tryActuationVal(ResourceT *rsc, typename actuation_type_val<ACT_T>::type *val)
	{
		Actuator *act = actForResource<ACT_T>(rsc);
		act->getModelActuation(rsc,val);
	}

	/*
	 * Gets the current actuatuation values from the predictive models
	 * A new val object is returned
	 */
	template<actuation_type ACT_T,typename ResourceT>
	static typename actuation_type_val<ACT_T>::type tryActuationVal(ResourceT *rsc)
	{
		Actuator *act = actForResource<ACT_T>(rsc);
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
