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

#ifndef __exec_sim___h
#define __exec_sim___h

#include <map>
#include <vector>
#include <sstream>
#include "core_legacy/core.h"
#include "inputparser.h"
#include "linsched_proxy.h"


typedef std::map<task_name_t,int> task_sim_conf_t;//number of instances of each benchmark
inline
std::string
task_sim_conf_name(task_sim_conf_t &conf){
    std::stringstream ss;
    for(auto bench : conf) {
        if(ss.str().size()>1) ss << "/";
        ss << bench.first;
    }
    return ss.str();
}

struct core_sim_cluster {
    int numClusters;
    int numCores;
    core_sim_cluster(const int &cores) :numClusters(cores), numCores(cores) {};
    core_sim_cluster(const int &clusters, const int &cores)
        :numClusters(clusters), numCores(cores)
    {
        assert((numCores % numClusters) == 0);
    };
    core_sim_cluster() :numClusters(0), numCores(0) {}

    inline int core_cnt() const { return numCores; }
    inline int cluster_cnt() const { return numClusters; }
    inline int cores_per_cluster() const { return numCores/numClusters; }
};

typedef std::map<core_arch_t,core_sim_cluster> core_sim_conf_t;//number of instances of each core type

class simulation_t {
public:
    struct core_data_t {
        model_core_t *core;
        core_data_t(model_core_t *_core = nullptr) :core(_core)
        {
            _power_gated_time = 0;
            _acc_power_gated_time = 0;
            mirror_acc_power_gated_time = 0;
        }

        core_data_t(const core_data_t&) = default;

        //for calculationg ips and power in the last run
        double								  _instr;
        std::map<core_freq_t,double> _idle_cycles;
        std::map<core_freq_t,double> _active_cycles;
        std::map<core_freq_t,double> _total_cycles;
        std::map<core_freq_t,double> _power_active_times_cycles;
        std::map<core_freq_t,double> _power_idle_times_cycles;//see above
        double                                _power_gated_time;

        //extract info from counters above
        double time_active();
        double time_idle();
        double time_off() { return _power_gated_time; }
        double time(){return time_idle()+time_active()+time_off();}
        double instructions();
        double ips();
        double power();
        double ips_watt() { return power()>0 ? ips()/power() : 0; }
        double avg_freqMHz();

        //accumulated values since simulation has started
        double								  _acc_instr;
        std::map<core_freq_t,double> _acc_idle_cycles;
        std::map<core_freq_t,double> _acc_active_cycles;
        std::map<core_freq_t,double> _acc_power_active_times_cycles;
        std::map<core_freq_t,double> _acc_power_idle_times_cycles;//see above
        double                                _acc_power_gated_time;

        //extract info from counters above
        double total_time_active();
        double total_time_idle();
        double total_time_off() { return _acc_power_gated_time; }
        double total_time(){return total_time_idle()+total_time_active()+total_time_off();}
        double total_avg_freqMHz();
        double total_avg_freq() { return total_avg_freqMHz()*1000000; }
        core_freq_t total_avg_freq_discrete();
        double total_avg_voltage(core_arch_t arch);
        double total_ips_active();
        double total_ips();
        double total_instructions();
        double total_power_active();
        double total_power_idle();
        double total_power();
        double total_energy() {return (total_time_idle() * total_power_idle()) + (total_time_active() * total_power_active()); }
        void dump();


        //a mirror off the _acc* during the last epoch the simulation had all tasks running
        double								  mirror_acc_instr;
        std::map<core_freq_t,double> mirror_acc_idle_cycles;
        std::map<core_freq_t,double> mirror_acc_active_cycles;
        std::map<core_freq_t,double> mirror_acc_power_active_times_cycles;
        std::map<core_freq_t,double> mirror_acc_power_idle_times_cycles;//see above
        double                                mirror_acc_power_gated_time;

        void set_mirrors();


    };

    struct vitamins_task_sensed_data_raw_t {
        uint64_t sumInstr;
        uint64_t sumMemInstr;
        uint64_t sumBRInstr;
        uint64_t sumBRMisspred;
        uint64_t sumFPInstr;
        uint64_t sumCyclesActive[SIZE_COREFREQ];
        uint64_t sumPowerTimesCycles[SIZE_COREFREQ];
        uint64_t sumPowerActiveTimesCycles[SIZE_COREFREQ];
        uint64_t sumiTLBaccesses;
        uint64_t sumiTLBmisses;
        uint64_t sumdTLBaccesses;
        uint64_t sumdTLBmisses;
        uint64_t sumICacheHits;
        uint64_t sumICacheMisses;
        uint64_t sumDCacheHits;
        uint64_t sumDCacheMisses;
        uint64_t sumL2CacheHits;
        uint64_t sumL2CacheMisses;
        //from linux scheduler
        uint32_t sumNivcsw;
        uint32_t sumNvcsw;

        //the most recent data from when these counters where last updated
        //all except last_dvfs_epoch_freq are reset every dvfs epoch
        core_freq_t last_dvfs_epoch_freq;
        uint64_t last_dvfs_epoch_sumCyclesActive;
        uint64_t last_dvfs_epoch_sumInstr;
        uint64_t last_dvfs_epoch_sumPowerTimesCycles;
    };

private:
    const task_sim_conf_t &taskConf;
    const core_sim_conf_t &coreConf;
    input_data_t &inputData;

    const bool _using_oracle;

    bool system_data_mirrors_set;


    std::vector<model_task_t*> _initial_task_list;
    std::vector<model_task_t*> _current_task_list;
    std::vector<model_task_t*> _removed_at_epoch_task_list;

    static void reset_task_counters(vitamins_task_sensed_data_raw_t *counters);

    struct task_epoch_info {
        struct task_sched_info sched;
        vitamins_task_sensed_data_raw_t counters;
    };

    std::map<model_task_t*,task_epoch_info*> task_sched_info;
    std::map<model_task_t*,vitamins_task_sensed_data_raw_t*> task_raw_counters;

    std::vector<core_info_t> _core_info_list;
    std::vector<model_core_t> _core_list;
    std::vector<model_systask_t> _core_systask_list;
    std::vector<freq_domain_info_t> _freq_domain_info_list;
    std::vector<model_freq_domain_t> freq_domain_list;
    int _freq_domain_list_size;
    std::vector<power_domain_info_t> _power_domain_info_list;
    std::vector<model_power_domain_t> power_domain_list;
    int _power_domain_list_size;


    struct sim_state_t {
        std::map<model_task_t*,int64_t> currInstr;//only for the current active tasks
        std::map<model_task_t*,int64_t> currInstrOracle;//only for the current active tasks
        std::map<model_task_t*,double> finishTime;//only set once the task ends. The map is initially empty
        std::map<model_task_t*,std::string> benchmark;//the benchmark data for each task
        double currentTime; //seconds
        double commitedTime;
        int minor_epoch_count;
        int major_epoch_count;
    };

    sim_state_t sim_state;

    //these stats are only valid after a call to end_epoch()
    std::vector<core_data_t> system_data;

    sys_info_t vitamins_sys_info_wrapper;
    model_sys_t vitamins_sys_wrapper;

public:

    typedef enum {
      VB_NONE=0,
      VB_BASIC=1,
      VB_FULL=2
    } verbosity_level;

    static inline bool vb_lv(verbosity_level verbosity,verbosity_level min) { return verbosity >= min; }

    simulation_t(input_data_t &_inputData, const task_sim_conf_t &_taskConf, const core_sim_conf_t &_coreConf, const bool using_oracle, verbosity_level verbose)
        :taskConf(_taskConf), coreConf(_coreConf), inputData(_inputData), _using_oracle(using_oracle), system_data_mirrors_set(false)
    {
        sim_init(verbose);
    }

    bool advance_time(double timeToAdvance, verbosity_level verbose);
    bool advance_time_standalone(double timeToAdvance, verbosity_level verbose);

    void end_epoch(verbosity_level verbose);

    simulation_t* clone();

    ~simulation_t(){
        for (auto task : task_sched_info) delete task_sched_info[task.first];
        for (auto task : _initial_task_list) delete task;
    }

    inline model_task_t** task_list() { return _current_task_list.data(); }
    inline int task_list_size() { return _current_task_list.size(); }
    const std::vector<model_task_t*>& task_list_vector() { return _current_task_list;}

    inline core_info_t* core_info_list() { return _core_info_list.data(); }
    inline model_core_t* core_list() { return _core_list.data(); }
    inline int core_list_size() { return _core_list.size(); }
    const std::vector<model_core_t>& core_list_vector() { return _core_list;}

    inline freq_domain_info_t* freq_domain_info_list() { return _freq_domain_info_list.data(); }
    inline int freq_domain_list_size() { return _freq_domain_list_size; }

    inline power_domain_info_struct* power_domain_info_list() { return _power_domain_info_list.data(); }
    inline int power_domain_list_size() { return _power_domain_list_size; }

    const vitamins_task_sensed_data_raw_t* task_counters(model_task_t *task) {
    	return task_raw_counters[task];
    }

    inline model_sys_t* vitamins_sys() {
    	vitamins_sys_wrapper.info = &vitamins_sys_info_wrapper;
    	vitamins_sys_wrapper.task_list = task_list();
    	vitamins_sys_wrapper.task_list_size = task_list_size();
    	vitamins_sys_wrapper.info->core_list = core_info_list();
    	vitamins_sys_wrapper.core_systask_list = _core_systask_list.data();
    	vitamins_sys_wrapper.info->core_list_size = core_list_size();
    	vitamins_sys_wrapper.info->freq_domain_list = freq_domain_info_list();
    	vitamins_sys_wrapper.info->freq_domain_list_size = freq_domain_list_size();
    	vitamins_sys_wrapper.info->power_domain_list = power_domain_info_list();
    	vitamins_sys_wrapper.info->power_domain_list_size = power_domain_list_size();
    	return &vitamins_sys_wrapper;
    }

    inline model_sys_t* vitamins_initial_sys() {
    	vitamins_sys();
    	vitamins_sys_wrapper.task_list = _initial_task_list.data();
    	vitamins_sys_wrapper.task_list_size = _initial_task_list.size();
    	return &vitamins_sys_wrapper;
    }

    std::string task_string(model_task_t *task);

    std::string& task_name(model_task_t *task) { return sim_state.benchmark[task];}

    void print_curr_mapping();
    void print_next_mapping();

    void print_system_data(bool print_epoch, bool print_total);

    struct system_average_t {
        double total_ips;
        double total_power;
        double total_time;
        double total_energy;
        int epochs;

        double ips_watt() { return total_ips/total_power; }

        double ips_watt_norm;
        double ips_norm;
        double time_norm;
        double energy_norm;
    };

    system_average_t get_system_average();

    core_data_t& get_core_data(int core) { return system_data[core]; }

    double getTime() {
    	return sim_state.currentTime;
    }

private:
    void sim_init_add_core_data(model_core_t *core);
    void sim_init_cores();
    void sim_init_tasks();
    void sim_init(verbosity_level verbose);

    int64_t average_data(
            model_task_t *task,
            std::vector<task_data_t> &samples,std::vector<task_data_t>::iterator begin, int numOfSamples,
            core_arch_t arch, core_freq_t freq,
            task_epoch_info *sched_info);

    void average_data_for_estm(
            model_task_t *task,
            std::vector<task_data_t> &samples,std::vector<task_data_t>::iterator begin, int numOfSamples,
            core_arch_t arch, core_freq_t freq);

    void check_csv_data(model_task_t *task);

    int64_t search_samples_and_average(model_task_t *task, std::vector<task_data_t> &samples,
            int64_t currInstr, int windowSize,
            core_arch_t arch, core_freq_t freq,
            task_epoch_info *sched_info);

    void reset_minor();
    void commit_minor(double timeToAdvance,verbosity_level verbose);
    void reset_major(bool enable_oracle);
    void commit_major(verbosity_level verbose);

    void commit_task(model_task_t *task, verbosity_level verbose,bool task_finished);
    void commit_task_counters(model_task_t *task);
    void commit_cpu_minor_counters(double timeToAdvance);
    void commit_cpu_major_counters();

    void commit_cpu_minor_stats(double timeToAdvance,verbosity_level verbose);
    void commit_cpu_major_stats();
    void commit_cpu_epoch_stats();
    void commit_cpu_overall_stats();

    void commit_minor_check_error(double timeToAdvance);
    double compute_discrepancy(double timeToAdvance,int core);
    model_task_t* largest_discrepancy(double timeToAdvance,int core);
    void fix_discrepancy(model_task_t* task, double error);

    void print_mapping(bool curr);

    void advance_time_preconditions(double timeToAdvance);

    void run_initial_estimation(double timeToAdvance, int window, model_task_t* task);

    int64_t run_final_estimation(int window, model_task_t* task, double estm_max_runtime);

    void run_oracle(double timeToAdvance,model_task_t* task);

    void run_linsched(double timeToAdvance, verbosity_level verbose);
};


#endif
