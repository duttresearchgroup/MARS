/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "actuator.h"

Actuator::Actuator(ActuationType type,const sys_info_t &_info)
	:_type(type),_mode(ACTMODE_SYSTEM),info(_info)
{

}

Actuator::~Actuator()
{
	//TODO remove all entries that have this obj from _actuatorMap
	//pinfo("%s called\n",__PRETTY_FUNCTION__);
}

std::map<ActuationType,std::map<void*,Actuator*>> Actuator::_actuatorMap;

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

