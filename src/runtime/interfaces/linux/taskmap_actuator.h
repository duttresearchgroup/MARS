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

#ifndef __arm_rt_linux_actuatortaskmap_h
#define __arm_rt_linux_actuatortaskmap_h


#include <runtime/framework/types.h>
#include <runtime/framework/actuator.h>

#include <sched.h>


class LinuxTaskMapActuator : public Actuator {

protected:
	void implSystemMode(){

	}
	void implSystemMode(const std::string &arg){

	}
	void implFrameworkMode(){

	}

public:
	LinuxTaskMapActuator(const sys_info_t &_info)
		:Actuator(ACT_TASK_MAP,_info)
	{
		for(int i = 0; i < _info.core_list_size; ++i)
			setActForResource(&(_info.core_list[i]));
	}

	~LinuxTaskMapActuator()
	{
	}

	// Standard actuation interface override

	void doSysActuation(core_info_t *rsc, const tracked_task_data_t *task) override {
	    cpu_set_t set;
	    CPU_ZERO( &set );
	    CPU_SET(rsc->position, &set );
	    sched_setaffinity(task->this_task_pid, sizeof( cpu_set_t ), &set);
	}
	void getSysActuation(core_info_t *rsc, const tracked_task_data_t **val_mhz) override {
		//DOES nothing
	    //We need to refactor the actuation iterface for two reasons
	    //1) This functions doesn't make sense in this actuator
	    //2) We could do the other way around, passing tasks as resource, but
	    //then we cannot call setActForResource for tasks since were not created
	    //yet.
	}

};

#endif

