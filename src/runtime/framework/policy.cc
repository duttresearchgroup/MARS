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

#include <fstream>
#include <cstdio>

#include <sched.h>
#include <unistd.h>

#include <base/base.h>
#include <runtime/framework/policy.h>

#include <runtime/interfaces/common/pal/pal_setup.h>
#include <runtime/interfaces/common/sensing_window_defs.h>

#include <signal.h>

bool PolicyManager::_pm_created = false;

PolicyManager::PolicyManager()
    :_sys_info(*pal_sys_info(sysconf(_SC_NPROCESSORS_ONLN)))
{
	_init_common();

    uint32_t cksum = sys_info_cksum(&_sys_info);
    if(cksum != _win_manager->sensingModule()->data().sysChecksum()) arm_throw(DaemonSystemException,"Sys info cksum differs");

    _pm_pid = getpid();
	_pm_ready_file = OptionParser::parser().progName() + ".ready";
	pinfo("PolicyManager::PolicyManager() done\n");
}

#if defined(IS_OFFLINE_PLAT)
PolicyManager::PolicyManager(simulation_t *sim)
{
	_init_info(sim);

	_init_common();
}
#endif

void PolicyManager::_init_common()
{
    if(_pm_created) arm_throw(DaemonSystemException,"System already created");
    _pm_created = true;

    _win_manager = new SensingWindowManager();

    //additional check to make sure the domain/core ids match the idx
    for(int cpu = 0; cpu < _sys_info.core_list_size; ++cpu){
        if(_sys_info.core_list[cpu].position != cpu) arm_throw(DaemonSystemException,"Sys info assumptions wrong");
    }
    for(int power_domain = 0; power_domain < _sys_info.power_domain_list_size; ++power_domain){
        if(_sys_info.power_domain_list[power_domain].domain_id != power_domain) arm_throw(DaemonSystemException,"Sys info assumptions wrong");
    }
    for(int freq_domain = 0; freq_domain < _sys_info.freq_domain_list_size; ++freq_domain){
        if(_sys_info.freq_domain_list[freq_domain].domain_id != freq_domain) arm_throw(DaemonSystemException,"Sys info assumptions wrong");
    }

    //Does required setup for actuators
    ActuationInterface::construct(_sys_info);
}

PolicyManager::~PolicyManager()
{
    if(ReflectiveEngine::enabled())
        ReflectiveEngine::disable();
    ActuationInterface::destruct();
	_pm_created = false;
	delete _win_manager;
}

void PolicyManager::_sensing_setup_common()
{
	//enables the perfcnts we are sampling

	//we always do instr and busy cy
	_win_manager->sensingModule()->tracePerfCounter(PERFCNT_INSTR_EXE);
	_win_manager->sensingModule()->tracePerfCounter(PERFCNT_BUSY_CY);
}

void PolicyManager::registerPolicy(Policy *policy)
{
    assert_true(policy != nullptr);
    auto i = _periodToPolicyMap.find(policy->periodMS());
    if(i == _periodToPolicyMap.end()){
        _periodToPolicyMap[policy->periodMS()] = std::vector<Policy*>();
        i = _periodToPolicyMap.find(policy->periodMS());
    }
    i->second.push_back(policy);

    // Also register as a model
    registerModel(policy);

    // We will finish registering policies later with
    // _finishRegisterPolicy
}

void PolicyManager::_finishRegisterPolicy()
{
    //Set a window handler for each period with the priority
    //of the highest priority policy
    for(auto i : _periodToPolicyMap){
        Policy::Priority maxPrio = Policy::PRIORITY_MIN;
        for(auto p : i.second)
            if(p->priority() > maxPrio)
                maxPrio = p->priority();

        //create the window handler
        auto winfo = _win_manager->addSensingWindowHandler(i.first,this,_policyWindowHandler,maxPrio);

        assert_true(winfo->wid >= 0);
        assert_true(winfo->wid < MAX_WINDOW_CNT);

        //Create list of policies for this window
        for(auto p : i.second)
            _policies[winfo->wid].insert(p);
    }
}

void PolicyManager::_buildSchedules()
{
    if(!ReflectiveEngine::enabled()) return;

    for(auto model : _models)
        ReflectiveEngine::get().buildSchedule(model);
}

void PolicyManager::_policyWindowHandler(int wid, PolicyManager *owner)
{
    // Executes all policies for this window
    for(auto p : owner->_policies[wid]){
        ReflectiveEngine::Context::PolicyScope polScp(p);
        p->setCurrentTimeMS(p->nextExecTimeMS());

        if(ReflectiveEngine::enabled()){
            // Pure models don't have a handler executing them, so we increment
            // their current time here (for all models finer-grained then this policy)
            for(auto model : owner->_models){
                if(model == (Model*)p) break; // further models are coarser-grained
                if(!model->pureModel()) continue;
                while(model->nextExecTimeMS() <= p->currExecTimeMS())
                    model->setCurrentTimeMS(model->nextExecTimeMS());
            }

            // Reset models before running the policy
            // to clean up any tryActuate issued in the scope of
            // a previous policy
            ReflectiveEngine::get().resetModelsOnHandler();
        }

        // Now execute the policy
        p->execute(wid);
    }
}

void PolicyManager::registerModel(Model *model)
{
    assert_true(model != nullptr);
    _models.insert(model);
    model->sys_info = info();
    model->pm = this;
    model->onRegister();
}

void PolicyManager::start()
{
    _sensing_setup_common();
	setup();
	_finishRegisterPolicy();
	_buildSchedules();

	//saves the sys_info
	SysInfoPrinter sip(_sys_info); sip.printToOutdirFile();

	_win_manager->startSensing();

	//creates a file that users can check to see if the daemon is ready
	//also stores the daemon pid
    std::ofstream fs(_pm_ready_file);
    fs << _pm_pid;
    fs.close();
}

void PolicyManager::stop()
{
    _win_manager->stopSensing();
	report();
	//removes the file created by start
	std::remove(_pm_ready_file.c_str());
}

void PolicyManager::quit() const
{
    kill(_pm_pid,SIGQUIT);
    for (;;) pause();

}

void PolicyManager::enableReflection() const
{
    ReflectiveEngine::enable(&_sys_info);
}


