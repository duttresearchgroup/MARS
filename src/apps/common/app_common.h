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

#ifndef __APP_COOMMON_H
#define __APP_COOMMON_H

#include <thread>

#include "core_legacy/core.h"
#include "offline_sim/inputparser.h"
#include "offline_sim/exec_sim.h"

typedef simulation_t::system_average_t system_average_t;

void print_average(system_average_t &avg, std::string msg);
void print_average(system_average_t &avg);

double get_real_time_ms();
double get_real_time_us();

enum map_algorithm {
    SPARTA = 0,
    SPARTA_AGINGAWARE,
    MTS,
    OPTIMAL,
    OPTIMAL_SHARED,
    OPTIMAL_SHARED_FREQ,
    OPTIMAL_SPARTA,
    VANILLA,
    VANILLA_SHARED,
    VANILLA_SHARED_AGINGAWARE,
    GTS,
    GTS_SHARED,
    GTS_SHARED_AGINGAWARE,
    SA_SOLVER,
	ROUND_ROBIN,
	CTRL_CACHE,

    MAP_ALGORITHM_SIZE
};

extern bool map_algorithm_use_oracle[MAP_ALGORITHM_SIZE];

simulation_t*
setup_simulation(map_algorithm algorithm,
        input_data_t &inputData, task_sim_conf_t &taskConf, core_sim_conf_t &coreConf,
        simulation_t::verbosity_level verbose,
        bool baseline_sys_overhead=false);

simulation_t*
setup_simulation(input_data_t &inputData, task_sim_conf_t &taskConf, core_sim_conf_t &coreConf,
        simulation_t::verbosity_level verbose,
        bool baseline_sys_overhead=false);

void
run_map_algorithm_once(simulation_t* sim,
        map_algorithm algorithm, dvfs_algorithm_t dvfs,
        int map_epoch_ms, int dvfs_epoch_ms, simulation_t::verbosity_level verbose,
        bool print_pred_errors=false);

system_average_t
run_map_algorithm(
        map_algorithm ma, dvfs_algorithm_t dvfs,
        input_data_t &inputData, task_sim_conf_t &taskConf, core_sim_conf_t &coreConf,
        int map_epoch_ms, int dvfs_epoch_ms,
        int numOfRuns, simulation_t::verbosity_level verbose,
        bool print_pred_errors=false,
        bool baseline_sys_overhead=false);

system_average_t
run_sparta_debug(input_data_t &inputData, task_sim_conf_t &taskConf, core_sim_conf_t &coreConf, double epoch);

std::thread*
run_map_algorithm_in_thread(system_average_t &result, map_algorithm ma, dvfs_algorithm_t dvfs,
              input_data_t &inputData, task_sim_conf_t &taskConf, core_sim_conf_t &coreConf, int map_epoch_ms, int dvfs_epoch_ms, int numRuns);

//// Result printing helpers

typedef std::map<std::string,system_average_t>
        results_summary_per_bench_t;
typedef std::map<std::string,results_summary_per_bench_t>
        results_summary_t;

struct norm_metric_data {
    double ips;
    double ips_watt;
};

void norm_metric_helper_init(norm_metric_data &helper_data,system_average_t &result);
void norm_metric_helper_set(norm_metric_data &helper_data,system_average_t &result);
void norm_metric_helper_commit(results_summary_per_bench_t &commit_target,std::string &commit_name,system_average_t &result);
void norm_metric_helper_set_commit(norm_metric_data &helper_data,results_summary_per_bench_t &commit_target,std::string commit_name,system_average_t result);
void norm_metric_helper_init_set_commit(norm_metric_data &helper_data,results_summary_per_bench_t &commit_target,std::string commit_name,system_average_t result);


//////////

void print_total_avgs(std::vector<results_summary_t> &results);
void print_totals(results_summary_t &results);

struct task_conf_helper{
    task_name_t name;
    int count;
};
task_sim_conf_t create_task_conf_helper(std::vector<task_conf_helper> benchmarks);

double get_map_overhead(map_algorithm a);
double get_pred_overhead();
void reset_overheads();

bool check_inputs(input_data_t &inputData);

#endif
