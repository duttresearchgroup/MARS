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

#include <runtime/interfaces/performance_data.h>

class ActuationInterface;

class Actuator {
	friend class ActuationInterface;

  private:

	ActuationType _type;
	ActuationMode _mode;

  public:

	const sys_info_t &info;

  protected:

	static std::map<ActuationType,std::map<void*,Actuator*>> _actuatorMap;

	void setActForResource(void *rsc);

	template<ActuationType ACT_T,typename ResourceT>
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

	Actuator(ActuationType type,const sys_info_t &_info);

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

	ActuationType type() { return _type;}

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

	ActuationMode mode() { return _mode;}

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
									 const PerformanceData& data, int wid, int cores_min=1);


};


#endif /* ACTUATOR_H_ */
