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


/*
 * 1) One function implements the actuation on the system
 * 2) Another one in the model (this one could be generic probably)
 *
 * The policy should use 2) first to see the effects on the system,
 * then use 1) make them effective if this is the case
 *
 * Implement 1) first for RSP
 *
 *
 *
 */
class Actuator {
private:
	static std::map<actuation_type,std::map<void*,Actuator*>> _actuatorMap;
	actuation_type _type;
	actuation_mode _mode;
public:
	const sys_info_t &info;

protected:
	Actuator(actuation_type type,const sys_info_t &_info);

	virtual ~Actuator();

	void setActForResource(void *rsc);

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


	template<actuation_type ACT_T,typename ResourceT,typename ActuationValT>
	static void actuation(ResourceT *rsc,ActuationValT val)
	{
		Actuator *act = actForResource<ACT_T>(rsc);
		act->doSysActuation(rsc,val);
	}

	template<actuation_type ACT_T,typename ResourceT,typename ActuationValT>
	static void actuation(ResourceT *rsc,ActuationValT *val)
	{
		Actuator *act = actForResource<ACT_T>(rsc);
		act->getSysActuation(rsc,val);
	}

	template<actuation_type ACT_T,typename ResourceT,typename ActuationValT>
	static void modelActuation(ResourceT *rsc,ActuationValT val)
	{
		Actuator *act = actForResource<ACT_T>(rsc);
		act->doModelActuation(rsc,val);
	}

	template<actuation_type ACT_T,typename ResourceT,typename ActuationValT>
	static void modelActuation(ResourceT *rsc,ActuationValT *val)
	{
		Actuator *act = actForResource<ACT_T>(rsc);
		act->getModelActuation(rsc,val);
	}


	template<actuation_type ACT_T,typename ResourceT,typename ActuationValT>
	static void actuation(ResourceT &rsc,ActuationValT val)
	{
		Actuator *act = actForResource<ACT_T>(&rsc);
		act->doSysActuation(&rsc,val);
	}

	template<actuation_type ACT_T,typename ResourceT,typename ActuationValT>
	static void actuation(ResourceT &rsc,ActuationValT *val)
	{
		Actuator *act = actForResource<ACT_T>(&rsc);
		act->getSysActuation(&rsc,val);
	}

	template<actuation_type ACT_T,typename ResourceT,typename ActuationValT>
	static void modelActuation(ResourceT &rsc,ActuationValT val)
	{
		Actuator *act = actForResource<ACT_T>(&rsc);
		act->doModelActuation(&rsc,val);
	}

	template<actuation_type ACT_T,typename ResourceT,typename ActuationValT>
	static void modelActuation(ResourceT &rsc,ActuationValT *val)
	{
		Actuator *act = actForResource<ACT_T>(&rsc);
		act->getModelActuation(&rsc,val);
	}


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
									 core_freq_t);
	ActuatorDefDefaultActuationFuncs(freq_domain_info_t,
									 int);
	//used when setting idle cores
	ActuatorDefDefaultActuationFuncs(freq_domain_info_t,
									 int,
									 const SensedData& data, int wid, int cores_min=1);


};


template<typename ActuatorT>
class ActuationPolicy {
	ActuatorT actuator;
	//TODO
};



#endif /* ACTUATOR_H_ */
