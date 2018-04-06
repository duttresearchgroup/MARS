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

#include <sys/time.h>
#include <vector>
#include <algorithm>
#include <limits>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sstream>
#include <iostream>
#include <cassert>
#include <map>
#include <random>
#include <thread>
#include <unistd.h>

#include "../runtime/systems/tracing.h"
#include <runtime/common/semaphore.h>
//#include "offline_sim/sensing_module.h"
#include "core/core.h"
#include "offline_sim/inputparser.h"
#include "offline_sim/exec_sim.h"
//#include "../sa_solver/solver_cinterface.h"
//#include "../sa_solver/solver_defines.h"
#include "common/app_common.h"
//#include "bin_based_predictor_common.h"

//#include "runtime/framework/system.h"

static System *rtsys = nullptr;
static simulation_t *sim = nullptr;

static void daemon_exit(int sig) {
	try {
		switch (sig) {
		case SIGINT:
			pinfo("Exit signal received...\n");
			rtsys->stop();
			delete rtsys;
			delete sim;
			pinfo("Cleaning up done\n");
			exit(EXIT_SUCCESS);
			break;
		default:
			pinfo("Wasn't expecting signal %d !!!\n",sig);
			exit(EXIT_FAILURE);
		}
	} arm_catch(ARM_CATCH_NO_EXIT);

	exit(EXIT_FAILURE);
}

void fuck_off(input_data_t *inputs){

    ////////////////////////////////////////////////////////////////////
    ////          SET SIM SYSTEM CONF
    ////////////////////////////////////////////////////////////////////
    core_sim_conf_t coreConf;
//    coreConf[COREARCH_Exynos5422_BIG_LITTLE] = 1;
//    coreConf[COREARCH_Exynos5422_BIG_MEDIUM] = 4;
    coreConf[COREARCH_Exynos5422_BIG_BIG] = 1;
//    coreConf[COREARCH_Exynos5422_BIG_HUGE] = 4;

	core_freq_t freqs[1] = {COREFREQ_2000MHZ};
	vitamins_arch_freq_available_set(COREARCH_Exynos5422_BIG_BIG,freqs,1);

    results_summary_t resultsLight;

    task_sim_conf_t taskConf;
    taskConf.clear();
    taskConf["low_ipc_badcache-high_load"] = 1;
    resultsLight[task_sim_conf_name(taskConf)] = results_summary_per_bench_t();

    ////////////////////////////////////////////////////////////////////
    ////          RUN SIM
    ////////////////////////////////////////////////////////////////////
//    sim = new simulation_t(*inputs, taskConf, coreConf, false, simulation_t::VB_BASIC);
    sim = setup_simulation(*inputs, taskConf, coreConf, simulation_t::VB_BASIC);

    //initial mapping
    vitamins_initial_map_task(sim->vitamins_sys());
    for(auto task : sim->task_list_vector()){
//        if(simulation_t::vb_lv(verbose,simulation_t::VB_BASIC))
            std::cout << sim->task_string(task)
                      << " is being mapped to core " << task_next_core_idx(task) << "\n";
    }

    vitamins_dvfs_set_global_policy(DVFS_MANUAL);
    for(auto core : sim->core_list_vector())
    	vitamins_dvfs_manual_freq(&core,freqs[0]);

//    while(true){
//
//    	bool has_tasks = true;
//
//    	has_tasks = sim->advance_time(dvfs_epoch_sec,verbose);
//
//    	sim->end_epoch(verbose);//must be called after the last call to advance_time but before dvfs
//
//    	if(simulation_t::vb_lv(verbose,simulation_t::VB_BASIC)) sim->print_system_data(true,false);
//
//    	if(!has_tasks) break;
//    	if(simulation_t::vb_lv(verbose,simulation_t::VB_BASIC)) {
//    		std::cout << "Next mapping is\n";
//    		sim->print_next_mapping();
//    	}
//    }
//    if(simulation_t::vb_lv(verbose,simulation_t::VB_BASIC)) sim->print_system_data(false,true);
//
//    system_average_t aux = sim->get_system_average();
//    delete sim;
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
////          RUN DAEMON
////////////////////////////////////////////////////////////////////
	//all inits
//	daemon_init(argc,argv);
	//grab SIGQUIT for clean exit
	signal(SIGINT, daemon_exit);

	//init params
	std::string c = "trace_core=0";
	std::string c0 = "prog";
	std::string c1 = "outdir=./bullshit_test";
	std::string c2 = "mode=tracing";
	std::string c3 = "idlepower_filename=idlepower_mimo_ctrl.data";
	const char *args[5] = {c0.c_str(), c.c_str(), c1.c_str(), c2.c_str(), c3.c_str()};
    if(!init_rt_config_params(5,args)) arm_throw(DaemonInitException,"Error parsing daemon initialization params");

    rt_param_print();

//    static System *rtsys = init_rtsys_from_mode();
	rtsys =  new TracingSystem(sim);
    rtsys->sensingModule()->setSim(sim);
    rtsys->start();

	//sensing window threads are running. Quits from signal handler
	for (;;) pause();

	//should not reach here
////////////////////////////////////////////////////////////////////

    std::cout << "\n\n";


}

int main(){

    input_data_t *inputs = parse_csvs({"traces/gem5-alpha-mimo-ctrl-mar17-badcache"});
    vitamins_init_idle_power_fromfile("idlepower_mimo_ctrl.data");

    fuck_off(inputs);

    delete inputs;

}


