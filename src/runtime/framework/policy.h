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

#ifndef __arm_rt_system_h
#define __arm_rt_system_h

#include <base/base.h>

#include <runtime/framework/window_manager.h>
#include <runtime/common/reports.h>
#include <runtime/interfaces/common/performance_data.h>

#include "actuation_interface.h"
#include "sensing_interface.h"
#include "model.h"

class Policy : public Model {
    friend class PolicyManager;

  protected:
    Policy(int periodMS, const std::string &name, Priority priority)
      :Model(periodMS,name,priority)
    {
        pureModel(false);
    }
    Policy(int periodMS) :Policy(periodMS,"DefaultPolicyName",PRIORITY_DEFAULT) {}
    Policy(int periodMS, const std::string &name) :Policy(periodMS,name,PRIORITY_DEFAULT) {}
    Policy(int periodMS, Priority priority) :Policy(periodMS,"DefaultPolicyName",PRIORITY_DEFAULT){}


  public:
    virtual ~Policy(){}
};

class PolicyManager : public ActuationInterface, public SensingInterface {
  private:

    SensingWindowManager _win_manager;
    sys_info_t *_sys_info;
    SensingModule *_sm;

	void _init_common();
	void _sensing_setup_common();

	int _pm_pid;
	std::string _pm_ready_file;

    // List of models
    // Ordered by period and then priority such that
    // "finer-grained" models appear first
    struct modelCMP {
        bool operator()(const Model *a, const Model *b) const
        {
            if(a->periodMS() == b->periodMS())
                return a->priority() > b->priority();
            else
                return a->periodMS() < b->periodMS();
        }
    };
    std::multiset<Model*,modelCMP> _models;

    // List of policies for each sensing window
    // Ordered by priority
    struct policyCMP {
        bool operator()(const Policy *a, const Policy *b)
        { return a->priority() > b->priority(); }
    };
    std::multiset<Policy*,policyCMP> _policies[MAX_WINDOW_CNT];

    // Map to temporaly store policies until we get an window ID for their handler
    std::map<int,std::vector<Policy*>> _periodToPolicyMap;

    void _finishRegisterPolicy();

    void _buildSchedules();

    static void _policyWindowHandler(int wid, PolicyManager *owner);

  protected:

	PolicyManager(SensingModule *sm);

	/*
	 * Called by System::start()
	 * You must implement this in order to setup your sensing windows
	 */
	virtual void setup() = 0;

	/*
	 * Called by System::stop() at the end
	 * Override to print execution repots and/or dump files with data
	 */
	virtual void report() {};

  protected:

	void registerPolicy(Policy *policy);
	void registerModel(Model *model);

  public:
	virtual ~PolicyManager();

	void start();
	void stop();

	void quit() const;//this will KILL the daemon processes

	sys_info_t* info() { return _sys_info;}

	SensingModule *sensingModule() const { return _sm; }
	SensingWindowManager *windowManager() { return &_win_manager; }
	const PerformanceData& sensedData() { return _sm->data(); }

	//requires the model_path parameter
	void enableReflection() const;

	// Returns the model before/after the given model in the hierarchy
	// or null if none
	Model* modelBefore(Model *m) const
	{
	    auto iter = _models.find(m);
	    if((iter == _models.end()) || (iter == _models.begin()))
	        return nullptr;
	    else return *(--iter);
	}
	Model* modelNext(Model *m) const
	{
	    auto iter = _models.find(m);
	    if(iter == _models.end()) return nullptr;
	    ++iter;
	    if(iter == _models.end()) return nullptr;
	    else return *iter;
	}

	// The first model and last models in the hierarchy.
	// The finer one is the one with the shortest period and highest priority.
	Model* modelCoarser() const { return *(_models.rbegin()); }
	Model* modelFiner() const { return *(_models.begin()); }

};

#endif

