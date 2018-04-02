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
#include "reflective.h"

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
    const std::string _name;
    const Priority _priority;

    //set later when the model is registered
    const sys_info_t *sys_info;

    const PolicyManager *pm;

    bool _pureModel;

    int _currentTimeMS;
    int _tmpCurrentTimeMS;

    // Increased as this model get's executed in reflective scope
    int _nesting;

    ReflectiveEngine::ReflectiveScheduleEntry* _schedule;

    void setCurrentTimeMS(int timeMS)
    {
        // We will temporally increment _currentTimeMS
        // When running the models as part of the reflective
        // flow, so keep a copy at _tmpCurrentTimeMS so we can
        // restore it later
        _currentTimeMS = timeMS;
        _tmpCurrentTimeMS = timeMS;
    }

  protected:
    Model(int periodMS, const std::string &name, Priority priority)
      :_periodMS(periodMS),_name(name),_priority(priority), sys_info(nullptr), pm(nullptr),
       _pureModel(true), _currentTimeMS(0), _tmpCurrentTimeMS(0), _nesting(0),
       _schedule(nullptr)
    {
    }
    Model(int periodMS) :Model(periodMS,"DefaultModelName",PRIORITY_DEFAULT) {}
    Model(int periodMS, const std::string &name) :Model(periodMS,name,PRIORITY_DEFAULT) {}
    Model(int periodMS, Priority priority) :Model(periodMS,"DefaultModelName",priority) {}

    virtual void onRegister() {}

    void pureModel(bool val) { _pureModel = val; }

  public:
    virtual ~Model()
    {
        while(_schedule != nullptr){
            auto tmp = _schedule->next;
            delete _schedule;
            _schedule = tmp;
        }
    }

    virtual void execute(int wid) = 0;

    Priority priority() const { return _priority; }
    const std::string& name() const { return _name; }
    int periodMS() const { return _periodMS; }

    const sys_info_t* info() {
        assert_true(sys_info != nullptr);//is this policy registered ???
        return sys_info;
    }

    const PolicyManager* manager() {
        assert_true(pm != nullptr);//is this policy registered ???
        return pm;
    }

    bool pureModel() const { return _pureModel; }

    int currExecTimeMS() { return _currentTimeMS; }

    int nextExecTimeMS() const
    {
        return _currentTimeMS + periodMS();
    }

    void tmpSetCurrExecTimeMS(int timeMS) { _currentTimeMS = timeMS; }

    void restoreCurrExecTimeMS() { _currentTimeMS = _tmpCurrentTimeMS; }

    int nesting() { return _nesting; }

    void incNesting() { _nesting += 1; }
    void decNesting() { _nesting -= 1; }

    void schedule(ReflectiveEngine::ReflectiveScheduleEntry *s)
    {
        assert_true(_schedule == nullptr);
        _schedule = s;
    }

    ReflectiveEngine::ReflectiveScheduleEntry* schedule()
    {
        return _schedule;
    }


};


#endif
