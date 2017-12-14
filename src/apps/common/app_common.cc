
#include <sys/time.h>
#include <vector>
#include <algorithm>
#include <limits>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <pthread.h>
#include <semaphore.h>
#include <sstream>
#include <iostream>
#include <cassert>
#include <map>
#include <random>
#include <thread>

#include "offline_sim/semaphore.h"
#include "core/core.h"
#include "offline_sim/inputparser.h"
#include "offline_sim/exec_sim.h"
#include "../../sa_solver/solver_cinterface.h"
#include "../../sa_solver/solver_defines.h"
#include "app_common.h"
#include "bin_based_predictor_common.h"


struct __overhead {
    double acc;
    int cnt;
};

static std::map<map_algorithm,__overhead> _map_overhead;
static __overhead _pred_overhead = {0,0};

double get_map_overhead(map_algorithm a) { return _map_overhead[a].acc / _map_overhead[a].cnt; }
double get_pred_overhead() { return _pred_overhead.acc / _pred_overhead.cnt; }

void reset_overheads(){
	_pred_overhead = {0,0};
	_map_overhead.clear();
}

static
void
_add_overhead(map_algorithm a, double pred, double map)
{
    _pred_overhead.acc += pred;
    _pred_overhead.cnt += 1;

    if(_map_overhead.find(a) == _map_overhead.end()){
        _map_overhead[a] = {map,1};
    }
    else{
        _map_overhead[a].acc += map;
        _map_overhead[a].cnt += 1;
    }

}

void print_average(system_average_t &avg, std::string msg){
    std::cout << "Average for " << msg << "\n"
              << "\tIPS="<<avg.total_ips << "\n"
              << "\tPower="<<avg.total_power << "\n"
              << "\tIPS/Watt="<<avg.ips_watt() << "\n";
}

void print_average(system_average_t &avg){
    std::cout << "\tIPS="<<avg.total_ips << "\n"
              << "\tPower="<<avg.total_power << "\n"
              << "\tIPS/Watt="<<avg.ips_watt() << "\n"
              << "\tepochs="<<avg.epochs<< "\n"
              << "\tTime="<<avg.total_time<< "\n"
              << "\tEnergy="<<avg.total_energy<< "\n";
              //<< "\tIPS/Watt normalized="<<avg.ips_watt_norm << "\n";
}

bool map_algorithm_use_oracle[MAP_ALGORITHM_SIZE];

struct map_algorithm_use_oracle_init {
    map_algorithm_use_oracle_init(){
        for(int i = 0; i < MAP_ALGORITHM_SIZE; ++i) map_algorithm_use_oracle[i] = true;
    }
};
map_algorithm_use_oracle_init _map_algorithm_use_oracle_init;



static void assertor(int val,const char* file,int line,const char* exp){
    if((val == 0) && (file!=0)){
        std::cerr << "Assert failed at file "<<file<<" line "<<line<<":\n";
        std::cerr << exp << "\n";
    }
    assert(val);
}

/*
static void sasolver_dump_csv(const char* dumpCSVfile){
#ifdef LOG_RESULTS
    std::ofstream file;
    file.open (dumpCSVfile, std::ios::out);
    //n iter
    file << "Iterations;AcceptedValues;RejectedValues\n";
    for(unsigned int i = 0; i < vit_solver_get_solver_stat_iterations(); ++i){
        file << i << ";";
        if(vit_solver_get_solver_obj_func_accepted(i)) file << CONV_scaledINTany_DOUBLE(vit_solver_get_solver_obj_func_history_scaled(i)) << ";\n";
        else                          file << ";" << CONV_scaledINTany_DOUBLE(vit_solver_get_solver_obj_func_history_scaled(i)) << "\n";
    }
    file.flush();
    file.close();
#endif
}*/


simulation_t*
setup_simulation(map_algorithm algorithm,
        input_data_t &inputData, task_sim_conf_t &taskConf, core_sim_conf_t &coreConf,
        simulation_t::verbosity_level verbose,
        bool baseline_sys_overhead)
{
    simulation_t *sim = new simulation_t(inputData, taskConf, coreConf, map_algorithm_use_oracle[algorithm], verbose);

    if(baseline_sys_overhead){
    	//adds a 5% sys load overhead to the slowest core
    	model_sys_t *sys = sim->vitamins_sys();
    	for(int a = SIZE_COREARCH-1; a >= 0; --a){
        	for(int c = 0; c < sys->info->core_list_size; ++c){
        		if(sys->info->core_list[c].arch == a){
        			for(int f = 0; f < SIZE_COREFREQ; ++f){
        				if(vitamins_arch_freq_available((core_arch_t)a,(core_freq_t)f))
        					sys->core_systask_list[c].tlc[a][f] = CONV_DOUBLE_scaledUINT32(0.05);
        			}
        			return sim;
        		}
        	}
    	}
    }

    return sim;
}

simulation_t*
setup_simulation(input_data_t &inputData, task_sim_conf_t &taskConf, core_sim_conf_t &coreConf,
        simulation_t::verbosity_level verbose,
        bool baseline_sys_overhead)
{
    simulation_t *sim = new simulation_t(inputData, taskConf, coreConf, false, verbose);

    if(baseline_sys_overhead){
    	//adds a 5% sys load overhead to the slowest core
    	model_sys_t *sys = sim->vitamins_sys();
    	for(int a = SIZE_COREARCH-1; a >= 0; --a){
        	for(int c = 0; c < sys->info->core_list_size; ++c){
        		if(sys->info->core_list[c].arch == a){
        			for(int f = 0; f < SIZE_COREFREQ; ++f){
        				if(vitamins_arch_freq_available((core_arch_t)a,(core_freq_t)f))
        					sys->core_systask_list[c].tlc[a][f] = CONV_DOUBLE_scaledUINT32(0.05);
        			}
        			return sim;
        		}
        	}
    	}
    }

    return sim;
}

void
run_map_algorithm_once(simulation_t* sim,
        map_algorithm algorithm, dvfs_algorithm_t dvfs,
        int map_epoch_ms, int dvfs_epoch_ms, simulation_t::verbosity_level verbose,
        bool print_pred_errors)
{
    assert(((map_epoch_ms%dvfs_epoch_ms) == 0)&&"Map epoch must be a multiple of the dvfs epoch");
    int map_dvfs_ratio = map_epoch_ms/dvfs_epoch_ms;
    double dvfs_epoch_sec = dvfs_epoch_ms / 1000.0;

    if(algorithm == SA_SOLVER){
        if(!vit_solver_create(&std::malloc, &std::free, &assertor, MAX_NUM_CORES, MAX_NUM_TASKS)){
            std::cout << "Couldn't create solver stuff\n";
            abort();
        }
    }

    double pred_time = 0;
    double map_time = 0;
    double timer = 0;


    //tell vitamins about runtime
    vitamins_dvfs_set_dvfs_epoch(dvfs_epoch_ms*1000);
    vitamins_dvfs_set_map_epoch(map_epoch_ms*1000);


    //initial mapping
    vitamins_initial_map_task(sim->vitamins_sys());
    for(auto task : sim->task_list_vector()){
        if(simulation_t::vb_lv(verbose,simulation_t::VB_BASIC))
            std::cout << sim->task_string(task)
                      << " is being mapped to core " << task_next_core_idx(task) << "\n";
    }

    //dvfs policy
    assert((algorithm != OPTIMAL_SHARED_FREQ) || ((algorithm == OPTIMAL_SHARED_FREQ) && (dvfs == DVFS_MANUAL)));
    vitamins_dvfs_set_global_policy(dvfs);

    if(!map_algorithm_use_oracle[algorithm] && print_pred_errors){
    	vitamins_bin_predictor_task_error_reset();
    }

    while(true){

        bool has_tasks = true;
        for(int i = 0; (i < map_dvfs_ratio) && has_tasks; ++i){

            has_tasks = sim->advance_time(dvfs_epoch_sec,verbose);

            if(i==(map_dvfs_ratio-1) || !has_tasks)
                sim->end_epoch(verbose);//must be called after the last call to advance_time but before dvfs

            for(auto core : sim->core_list_vector())
                vitamins_dvfs_set_freq(&core);

        }


        if(simulation_t::vb_lv(verbose,simulation_t::VB_BASIC)) sim->print_system_data(true,false);

        if(!has_tasks) break;


        timer = get_real_time_us();
        if(!map_algorithm_use_oracle[algorithm])
            vitamins_bin_predictor(sim->vitamins_sys());
        pred_time += get_real_time_us() - timer;

        timer = get_real_time_us();
        switch (algorithm) {
            case SPARTA:
                vitamins_sparta_map(sim->vitamins_sys());
                break;
            case SPARTA_AGINGAWARE:
                vitamins_sparta_agingaware_map(sim->vitamins_sys());
                break;
            case MTS:
                vitamins_map_mts(sim->vitamins_sys());
                break;
            case OPTIMAL:
                vitamins_optimal_map(sim->vitamins_sys());
                break;
            case OPTIMAL_SHARED:
                vitamins_optimal_shared_map(sim->vitamins_sys());
                break;
            case OPTIMAL_SHARED_FREQ:
                vitamins_optimal_shared_freq_map(sim->vitamins_sys());
                break;
            case OPTIMAL_SPARTA:
                vitamins_optimal_sparta_map(sim->vitamins_sys());
                break;
            case VANILLA:
                vitamins_vanilla_map(sim->vitamins_sys());
                break;
            case VANILLA_SHARED:
                vitamins_vanilla_shared_map(sim->vitamins_sys());
                break;
            case VANILLA_SHARED_AGINGAWARE:
                vitamins_vanilla_shared_agingaware_map(sim->vitamins_sys());
                 break;
            case GTS:
                vitamins_gts_map(sim->vitamins_sys());
                break;
            case GTS_SHARED:
                vitamins_gts_shared_map(sim->vitamins_sys());
                break;
            case GTS_SHARED_AGINGAWARE:
                vitamins_gts_shared_agingaware_map(sim->vitamins_sys());
                break;
            case SA_SOLVER:
                vitamins_sasolver_map(sim->vitamins_sys());
                break;
            case ROUND_ROBIN:
                vitamins_roundrobin_map(sim->vitamins_sys());
                break;
            case CTRL_CACHE:
                vitamins_ctrl_cache_map(sim->vitamins_sys());
                break;
            default:
                assert(false&&"Unknown algorithm");
                break;
        }
        map_time += get_real_time_us() - timer;

        timer = get_real_time_us();
        if(!map_algorithm_use_oracle[algorithm])
            vitamins_bin_predictor_commit(sim->vitamins_sys());
        pred_time += get_real_time_us() - timer;

        if(simulation_t::vb_lv(verbose,simulation_t::VB_BASIC)) {
            std::cout << "Next mapping is\n";
            sim->print_next_mapping();
        }
    }

    _add_overhead(algorithm,pred_time,map_time);

    if(simulation_t::vb_lv(verbose,simulation_t::VB_BASIC)) sim->print_system_data(false,true);

    if(algorithm == SA_SOLVER){
        vitamins_sasolver_cleanup();
        vit_solver_destroy();
    }

    if(!map_algorithm_use_oracle[algorithm] && print_pred_errors){
    	vitamins_bin_predictor_error_report(sim->vitamins_initial_sys());
    }
}

system_average_t
run_map_algorithm(
        map_algorithm ma, dvfs_algorithm_t dvfs,
        input_data_t &inputData, task_sim_conf_t &taskConf, core_sim_conf_t &coreConf,
        int map_epoch_ms, int dvfs_epoch_ms,
        int numOfRuns, simulation_t::verbosity_level verbose,
        bool print_pred_errors,
        bool baseline_sys_overhead)
{

    system_average_t avg = {0,0,0,0,0,0,0,0,0};
    int nRuns = 0;
    int maxRuns = simulation_t::vb_lv(verbose,simulation_t::VB_BASIC) ? 1 : numOfRuns;

    for(; nRuns < maxRuns; ++nRuns){
        simulation_t *sim = setup_simulation(ma,inputData,taskConf,coreConf,verbose,baseline_sys_overhead);
        run_map_algorithm_once(sim,ma,dvfs,map_epoch_ms,dvfs_epoch_ms,verbose,print_pred_errors);
        system_average_t aux = sim->get_system_average();
        delete sim;
        //std::cout << "Average IPS " << aux.total_ips << " Power " << aux.total_power << "\n";

        avg.total_ips += aux.total_ips;
        avg.total_power += aux.total_power;
        avg.total_energy += aux.total_energy;
        avg.total_time += aux.total_time;
        avg.epochs += aux.epochs;
    }

    avg.total_ips /= nRuns;
    avg.total_power /= nRuns;
    avg.epochs /= nRuns;

    return avg;
}


double get_real_time_ms(){
    struct timeval now;
    gettimeofday(&now, NULL);
    double ms_now = (now.tv_sec * 1000) + ((double)now.tv_usec / 1000.0);
    return ms_now;
}

double get_real_time_us(){
    struct timeval now;
    gettimeofday(&now, NULL);
    double us_now = (now.tv_sec * 1000000) + (double)now.tv_usec;
    return us_now;
}

/*
static double calc_complete(double current, double total){
    return (current*100)/total;
}

static double calc_timeleft_min(double complete){
    static double last_time = 0;
    static double last_complete = 0;

    double curr_time = get_real_time_ms();
    double spent = curr_time - last_time;
    last_time  = curr_time;

    double cmpl_period = complete - last_complete;
    last_complete = complete;

    double percent_per_ms = cmpl_period / spent;

    double time_left_ms = (100-complete) / percent_per_ms;

    return (((time_left_ms/1000)/60));
}
*/

/*
static void optmimal_reporter(int64_t curr,int64_t max){
    //std::cout << "\t|";
    //for(int i = 0; i < mapping_size; ++i)
    //    std::cout << " " << mapping[i];
    //std::cout << "|\n";
    double complete = calc_complete(curr,max);
    int completeInt = (int)complete;
    if((complete == completeInt) && ((completeInt % 25)==0))
        std::cout << "\t" << curr << " out of \t" << max << " solutions tested. Time left " << calc_timeleft_min(complete) << " mins\n";
}
*/

bool _run_sparta_debug(simulation_t *_sim, double epoch, bool optimal){

    std::string opt = optimal ? "Optimal" : "SPARTA";
    std::cout << "################\n";
    std::cout << "#"<<opt<<" epoch\n";
    std::cout << "################\n";

    simulation_t *sim = _sim->clone();

    for(auto core : sim->core_list_vector())
        vitamins_dvfs_set_predicted_freq(&core);

    if(!sim->advance_time(epoch,simulation_t::VB_BASIC)) return false;
    sim->end_epoch(simulation_t::VB_BASIC);

    std::cout << "Epoch results:\n";
    sim->print_system_data(true,false);

    std::cout << "Current mapping:\n";
    sim->print_curr_mapping();

    if(optimal){
        vitamins_optimal_map(sim->vitamins_sys());
    }
    else{
        vitamins_sparta_map(sim->vitamins_sys());
    }

    std::cout << "Next mapping:\n";
    sim->print_next_mapping();

    for(auto core : sim->core_list_vector())
        vitamins_dvfs_set_predicted_freq(&core);

    if(!sim->advance_time(epoch,simulation_t::VB_NONE)) return false;
    sim->end_epoch(simulation_t::VB_NONE);

    std::cout << "Result of next mapping:\n";
    sim->print_system_data(true,false);

    delete sim;

    return true;
}

system_average_t run_sparta_debug(input_data_t &inputData, task_sim_conf_t &taskConf, core_sim_conf_t &coreConf, double epoch)
{

    //same as run_optimal with an extra call to debug

    simulation_t *sim = new simulation_t(inputData, taskConf, coreConf, true, simulation_t::VB_NONE);

    //initial mapping
    vitamins_initial_map_task(sim->vitamins_sys());

    bool do_optimal = false;

    while(true){
        _run_sparta_debug(sim,epoch,do_optimal);

        if(!sim->advance_time(epoch,simulation_t::VB_NONE)) break;
        vitamins_optimal_map(sim->vitamins_sys());
        for(auto core : sim->core_list_vector())
            vitamins_dvfs_set_predicted_freq(&core);
    }
    system_average_t avg = sim->get_system_average();

    delete sim;

    return avg;
}

static void _run_in_thread(
        system_average_t &result, map_algorithm ma, dvfs_algorithm_t dvfs,
        input_data_t &inputData, task_sim_conf_t &taskConf, core_sim_conf_t &coreConf, int map_epoch_ms, int dvfs_epoch_ms, int numRuns)
{
    result = run_map_algorithm(ma,dvfs,inputData,taskConf,coreConf,map_epoch_ms,dvfs_epoch_ms,numRuns,simulation_t::VB_NONE);
}

std::thread*
run_map_algorithm_in_thread(system_average_t &result, map_algorithm ma, dvfs_algorithm_t dvfs,
              input_data_t &inputData, task_sim_conf_t &taskConf, core_sim_conf_t &coreConf, int map_epoch_ms, int dvfs_epoch_ms, int numRuns)
{
    std::thread *thr = new std::thread(_run_in_thread,
                        std::ref(result), ma, dvfs,
                        std::ref(inputData),std::ref(taskConf),std::ref(coreConf),map_epoch_ms,dvfs_epoch_ms,numRuns);
    return thr;
}


void print_total_avgs(std::vector<results_summary_t> &results){

    std::map<std::string,double> avgs_ips;
    std::map<std::string,double> avgs_ips_watt;
    std::map<std::string,double> count;

    for (auto result : results){
        for(auto bench : result){
            for(auto opt : bench.second){
                avgs_ips_watt[opt.first] += opt.second.ips_watt_norm;
                avgs_ips[opt.first] += opt.second.ips_norm;
                count[opt.first] += 1;
            }
        }
    }

    std::cout << "Overall average normalized IPS:\n";
    for(auto x : avgs_ips){
        double data = x.second / count[x.first];
        std::cout << "\t" << x.first << " = " << data << "\n";
    }
    std::cout << "Overall average normalized IPS/Watt:\n";
    for(auto x : avgs_ips_watt){
        double data = x.second / count[x.first];
        std::cout << "\t" << x.first << " = " << data << "\n";
    }

}

void print_totals(results_summary_t &results){
    std::map<std::string,double> ips_watt;
    std::map<std::string,double> ips;
    std::map<std::string,double> count;
    for(auto bench : results){
        for(auto opt : bench.second){
            ips_watt[opt.first] += opt.second.ips_watt();
            ips[opt.first] += opt.second.total_ips;
            count[opt.first] += 1;
        }
    }
    for(auto x : ips_watt){
        double _ips_watt = ips_watt[x.first] / count[x.first];
        double _ips = ips[x.first] / count[x.first];
        std::cout << "\t" << x.first << " IPS = " << _ips <<  " IPS/Watt = " << _ips_watt << "\n";
    }
}

task_sim_conf_t create_task_conf_helper(std::vector<task_conf_helper> benchmarks)
{
    task_sim_conf_t taskConf;
    //assert(((num_tasks % benchmarks.size())==0)&&"Tasks must be equaly distributed");
    for(auto t : benchmarks){
        taskConf[t.name] = t.count;
    }
    return taskConf;
}


void norm_metric_helper_init(norm_metric_data &helper_data,system_average_t &result)
{
    helper_data.ips = result.total_ips;
    helper_data.ips_watt = result.ips_watt();
}
void norm_metric_helper_set(norm_metric_data &helper_data,system_average_t &result)
{
    result.ips_watt_norm = result.ips_watt()/helper_data.ips_watt;
    result.ips_norm = result.total_ips/helper_data.ips;
}
void norm_metric_helper_commit(results_summary_per_bench_t &commit_target, std::string &commit_name,system_average_t &result)
{
    print_average(result);
    commit_target[commit_name] = result;
}
void norm_metric_helper_set_commit(norm_metric_data &helper_data,results_summary_per_bench_t &commit_target,std::string commit_name,system_average_t result)
{
    norm_metric_helper_set(helper_data,result);
    norm_metric_helper_commit(commit_target,commit_name,result);
}
void norm_metric_helper_init_set_commit(norm_metric_data &helper_data,results_summary_per_bench_t &commit_target,std::string commit_name,system_average_t result)
{
    norm_metric_helper_init(helper_data,result);
    norm_metric_helper_set(helper_data,result);
    norm_metric_helper_commit(commit_target,commit_name,result);
}


bool check_inputs(input_data_t &inputData)
{
    std::set<core_freq_t> freqs;
    std::set<core_arch_t> archs;

    std::set<task_name_t> missingTasks;
    std::set<core_arch_t> missingArchs;
    std::set<core_freq_t> missingFreqs;

    bool ret = true;

    for(auto task : inputData.samples){
        for(auto arch : task.second){
            for(auto freq : arch.second){
                if((freq.second != nullptr) && (freq.second->size() > 0)){
                    archs.insert(arch.first);
                    freqs.insert(freq.first);
                }
            }
        }
    }

    for(auto task : inputData.samples){

        for(auto arch : archs){
            if(task.second.find(arch) == task.second.end()){
                std::cout << "Task " << task.first << " does not have any data for arch " << archToString(arch) << "\n";
                missingTasks.insert(task.first);
                missingArchs.insert(arch);
                ret = false;
            }
            else{
                for(auto freq : freqs){
                    if(task.second[arch].find(freq) != task.second[arch].end())
                        if((task.second[arch][freq] != nullptr) && (task.second[arch][freq]->size() > 0))
                            continue;

                    std::cout << "Task " << task.first << " does not have data for arch " << archToString(arch) << " freq " << freqToString(freq) << "\n";
                    missingTasks.insert(task.first);
                    missingArchs.insert(arch);
                    missingFreqs.insert(freq);
                    ret = false;
                }

            }
        }
    }

    if(!ret){
        std::cout << "Following tasks have some data missing: ";
        for(auto task : missingTasks) std::cout << task << " ";
        std::cout << "\nFollowing archs have some data missing: ";
        for(auto arch : missingArchs) std::cout << archToString(arch) << " ";
        std::cout << "\nFollowing freqs have some data missing: ";
        for(auto freq : missingFreqs) std::cout << freqToString(freq) << " ";
        std::cout << "\n";
    }

    return ret;
}
