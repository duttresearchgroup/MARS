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
{
	_init_info();

	_init_common();

    uint32_t cksum = sys_info_cksum(&_sys_info);
    if(cksum != _win_manager->sensingModule()->data().sysChecksum()) arm_throw(DaemonSystemException,"Sys info cksum differs");

    _pm_pid = getpid();
	_pm_ready_file = rt_param_daemon_file() + ".ready";
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
}

void PolicyManager::_init_info()
{
	int online_cpus = sysconf(_SC_NPROCESSORS_ONLN);

	assert_false(online_cpus > MAX_NR_CPUS);
	assert_false(MAX_NUM_TASKS <= online_cpus);

	_sys_info.core_list = &(_core_info_list[0]);
	_sys_info.core_list_size = online_cpus;

	pal_setup_freq_domains_info(&_sys_info);
	pal_setup_power_domains_info(&_sys_info);

	for(int core = 0; core < online_cpus; ++core){
		core_info_init(&(_core_info_list[core]), pal_core_arch(core), core, pal_core_freq_domain(core), pal_core_power_domain(core));
	}
}

#if defined(IS_OFFLINE_PLAT)
void PolicyManager::_init_info(simulation_t *sim)
{
	int online_cpus = sim->core_list_size();

	assert_false(online_cpus > MAX_NR_CPUS);
	assert_false(MAX_NUM_TASKS <= online_cpus);

	_sys_info.core_list = &(_core_info_list[0]);
	_sys_info.core_list_size = online_cpus;

	_sys_info.freq_domain_list_size = sim->freq_domain_list_size();
//	for (int f = 0; f < _sys_info.freq_domain_list_size; f++) {
//		&(_sys_info.freq_domain_list[f]) = &(sim->freq_domain_info_list()[0]);
//	}
	_sys_info.freq_domain_list = &(sim->freq_domain_info_list()[0]);


	_sys_info.power_domain_list = &(sim->power_domain_info_list()[0]);
	_sys_info.power_domain_list_size = sim->power_domain_list_size();

	for(int core = 0; core < online_cpus; ++core){
		_core_info_list[core] = sim->core_info_list()[core];
	}
}
#endif

PolicyManager::~PolicyManager()
{
	//pinfo("%s called\n",__PRETTY_FUNCTION__);
	_pm_created = false;
	delete _win_manager;
}

void PolicyManager::_sensing_setup_common()
{
	//enables the perfcnts we are sampling

	//we always do instr and busy cy
	_win_manager->sensingModule()->tracePerfCounter(PERFCNT_INSTR_EXE);
	_win_manager->sensingModule()->tracePerfCounter(PERFCNT_BUSY_CY);

	for(int i = 0; i < SIZE_PERFCNT; ++i){
		if(i == PERFCNT_BUSY_CY) continue;
		if(i == PERFCNT_INSTR_EXE) continue;
		if(rt_param_trace_perfcnt((perfcnt_t)i))
			_win_manager->sensingModule()->tracePerfCounter((perfcnt_t)i);
	}
}

void PolicyManager::start()
{
	_sensing_setup_common();
	setup();

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

void PolicyManager::quit()
{
    kill(_pm_pid,SIGQUIT);
}

