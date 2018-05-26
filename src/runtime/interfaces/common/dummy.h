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

// Implementation of the sense/actuate functions for the ACT_DUMMY1,ACT_DUMMY2,
// and SEN_DUMMY types

#ifndef __arm_rt_interfaces_dummy_h
#define __arm_rt_interfaces_dummy_h


#include <base/base.h>

#include <runtime/interfaces/sensing_module.h>
#include <runtime/interfaces/common/sensor.h>
#include <runtime/framework/types.h>


class DummySensor : public SensorBase<SEN_DUMMY,DummySensor,SensingModule> {

  public:

    typename SensingTypeInfo<SEN_DUMMY>::ValType readSample();

    static void create(SensingModule *sm);
    static void destroy();
};


#endif
