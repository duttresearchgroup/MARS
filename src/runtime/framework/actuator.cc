
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
	if(_actuatorMap.find(_type)==_actuatorMap.end()){
		_actuatorMap[_type] = std::map<void*,Actuator*>();
	}
	std::map<void*,Actuator*> &aux = _actuatorMap[_type];
	if(aux.find(rsc)!=aux.end())
		arm_throw(ActuatorExcepetion,"Resource %p for actuation type %d already set",rsc,_type);
	aux[rsc] = this;
}

