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

#ifndef __arm_rt_model_h
#define __arm_rt_model_h

#include <base/base.h>
#include <runtime/framework/window_manager.h>

#include "actuation_interface.h"
#include "sensing_interface.h"

class PolicyManager;

class Model : public ActuationInterface, public SensingInterface {
    friend class PolicyManager;
  public:
    typedef SensingWindowManager::Priority Priority;

    static constexpr Priority PRIORITY_DEFAULT = SensingWindowManager::PRIORITY_DEFAULT;
    static constexpr Priority PRIORITY_MIN = SensingWindowManager::PRIORITY_MIN;
    static constexpr Priority PRIORITY_MAX = SensingWindowManager::PRIORITY_MAX;

  private:
    const int _periodMS;
    const Priority _priority;

    //set later when the model is registered
    const sys_info_t *sys_info;

  protected:
    Model(int periodMS, Priority priority = PRIORITY_DEFAULT)
      :_periodMS(periodMS),_priority(priority), sys_info(nullptr)
    {
    }

  public:
    virtual ~Model(){}

    virtual void execute(int wid) = 0;

    Priority priority() const { return _priority; }
    int periodMS() const { return _periodMS; }

    const sys_info_t* info() {
        assert_true(sys_info != nullptr);//is this policy registered ???
        return sys_info;
    }
};



#endif

