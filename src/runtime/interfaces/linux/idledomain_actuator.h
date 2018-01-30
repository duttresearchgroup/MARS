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

#ifndef __arm_rt_linux_actuatoridledomain_h
#define __arm_rt_linux_actuatoridledomain_h

#include <runtime/framework/types.h>
#include <runtime/framework/actuator.h>

#include "idledomain.h"

class LinuxIdleDomainActuator : public Actuator {

private:
	IdleDomain **_idleDomain;

protected:
	void implSystemMode(){

	}
	void implSystemMode(const std::string &arg){

	}
	void implFrameworkMode(){

	}

public:
	LinuxIdleDomainActuator(const sys_info_t &_info)
		:Actuator(ACT_ACTIVE_CORES,_info),_idleDomain(nullptr)
	{
		_idleDomain = new IdleDomain*[info.freq_domain_list_size];
		for(int i = 0; i < _info.freq_domain_list_size; ++i){
			assert_true(info.freq_domain_list[i].domain_id == i);
			_idleDomain[i] = new IdleDomain(&info,info.freq_domain_list[i]);
		}

		for(int i = 0; i < _info.freq_domain_list_size; ++i)
			setActForResource(&(_info.freq_domain_list[i]));
	}
	~LinuxIdleDomainActuator()
	{
		for(int i = 0; i < info.freq_domain_list_size; ++i){
			delete _idleDomain[i];
		}
		delete _idleDomain;
	}

	void doSysActuation(freq_domain_info_t *rsc, int cores, const PerformanceData& data, int wid, int cores_min=1){
		_idleDomain[rsc->domain_id]->idleCores(cores,data,wid,cores_min);
	}
	void getSysActuation(freq_domain_info_t *rsc, int *cores, const PerformanceData& data, int wid, int cores_min=1){
		*cores = _idleDomain[rsc->domain_id]->idleCores();
	}


};

#endif

