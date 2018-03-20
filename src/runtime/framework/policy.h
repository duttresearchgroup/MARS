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
#include <runtime/interfaces/performance_data.h>

#include "actuation_interface.h"
#include "sensing_interface.h"
#include "model.h"

class Policy : public Model {
    friend class PolicyManager;

  protected:
    Policy(int periodMS, Priority priority = PRIORITY_DEFAULT)
      :Model(periodMS,priority)
    {
    }

  public:
    virtual ~Policy(){}
};

class PolicyManager : public ActuationInterface, public SensingInterface {
  private:
	//used to check only one system object should exist
	static bool _pm_created;

	sys_info_t _sys_info;
	core_info_t _core_info_list[MAX_NR_CPUS];

	void _init_common();
	void _init_info();
#if defined(IS_OFFLINE_PLAT)
	void _init_info(simulation_t *sim);
#endif
	void _sensing_setup_common();

	int _pm_pid;
	std::string _pm_ready_file;

    SensingWindowManager *_win_manager;

    // List of models
    // Ordered by period and then priority
    struct modelCMP {
        bool operator()(const Model *a, const Model *b)
        {
            if(a->periodMS() == b->periodMS())
                return a->priority() >= b->priority();
            else
                return a->periodMS() < b->periodMS();
        }
    };
    std::multiset<Model*,modelCMP> _models;

    // List of policies for each sensing window
    // Ordered by priority
    struct policyCMP {
        bool operator()(const Policy *a, const Policy *b)
        { return a->priority() >= b->priority(); }
    };
    std::multiset<Policy*,policyCMP> _policies[MAX_WINDOW_CNT];

    // Map to temporaly store policies until we get an window ID for their handler
    std::map<int,std::vector<Policy*>> _periodToPolicyMap;

    void _finishRegisterPolicy();

    static void _policyWindowHandler(int wid, PolicyManager *owner);

  protected:

	PolicyManager();
#if defined(IS_OFFLINE_PLAT)
	PolicyManager(simulation_t *sim);
#endif

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
	void quit();

	void registerPolicy(Policy *policy);
	void registerModel(Model *model);

  public:
	virtual ~PolicyManager();

	void start();
	void stop();

	sys_info_t* info() { return &_sys_info;}
	virtual model_sys_t* model() {return nullptr;}

	SensingModule *sensingModule() const { return _win_manager->sensingModule(); }
	SensingWindowManager *windowManager() { return _win_manager; }
	const PerformanceData& sensedData() { return sensingModule()->data(); }

};

#endif

