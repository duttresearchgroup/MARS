
#include "actuator.h"

Actuator::Actuator(actuation_type type,const sys_info_t &_info)
	:_type(type),_mode(ACTMODE_SYSTEM),info(_info)
{

}

Actuator::~Actuator()
{
	//TODO remove all entries that have this obj from _actuatorMap
	//pinfo("%s called\n",__PRETTY_FUNCTION__);
}

std::map<actuation_type,std::map<void*,Actuator*>> Actuator::_actuatorMap;

void Actuator::setActForResource(void *rsc)
{
	if(Actuator::_actuatorMap.find(_type)==Actuator::_actuatorMap.end()){
		Actuator::_actuatorMap[_type] = std::map<void*,Actuator*>();
	}
	std::map<void*,Actuator*> &aux = Actuator::_actuatorMap[_type];
	if(aux.find(rsc)!=aux.end())
		arm_throw(ActuatorExcepetion,"Resource %p for actuation type %d already set",rsc,_type);
	aux[rsc] = this;
}

