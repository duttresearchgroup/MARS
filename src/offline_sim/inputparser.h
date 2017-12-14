#ifndef __inputparser_h
#define __inputparser_h

#include <string>
#include <map>
#include <vector>
#include <cstdarg>

#include "core/core.h"

typedef std::string task_name_t;

struct task_csv_data_t {
	//may come from different trace formats
	//unavailable info should be set to -1
    core_arch_t conf_arch;
    core_freq_t conf_freq;
    double ipcActive;
    double ipcTotal;
    double avgDynPower;
    double avgLeakPower;
    double gatedSubThrLeakPower;
    double gateLeakPower;
    double l2TotalAvgPower;
    double l2SubThrLeakPower;
    double l2GateLeakPower;
    double totalPower;
    int64_t commitedInsts;
    int64_t quiesceCycles;
    int64_t idleCycles;
    int64_t busyCycles;
    int64_t totalActiveCycles;
    int64_t commitedMemRefs;
    int64_t commitedFPInsts;
    int64_t commitedBranches;
    int64_t branchMispredicts;
    int64_t itlbAccesses;
    int64_t itlbMisses;
    int64_t dtlbAccesses;
    int64_t dtlbMisses;
    int64_t iCacheHits;
    int64_t iCacheMisses;
    int64_t dCacheHits;
    int64_t dCacheMisses;
    int64_t l2CacheHits;
    int64_t l2CacheMisses;
};

struct task_data_t {
    double curr_time;  //time of this sample in seconds, startign at 0
    double curr_time_original;  //original time of this sample in seconds
    int64_t curr_instr; //num of instruction executed up to the time this sample was taken
    task_csv_data_t data;  //stats for the period between curr_time and the curr_time of the previous sample
    //If data format can be used for sim
    enum data_format_t{
        CSV_PRED_ONLY,
        CSV_PRED_AND_SIM,
    };
    data_format_t data_format;
};

typedef std::map<task_name_t,std::map<core_arch_t,std::map<core_freq_t, std::vector<task_data_t>* > > >
        sample_data_t;

struct input_data_t {
    sample_data_t samples;
    double sampleRate;
};


inline double freqToValMHz_d(core_freq_t freq){
    return freqToValMHz_i(freq);
}

inline double freqToPeriodPS_d(core_freq_t freq){
    return freqToPeriodPS_i(freq);
}

input_data_t * parse_csvs(std::vector<std::string> dirs);


#endif
