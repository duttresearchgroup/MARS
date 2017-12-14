
#include "exec_sim.h"
#include "linsched_proxy.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <random>
#include <limits>
#include <sstream>

std::string
simulation_t::task_string(model_task_t *task)
{
    std::stringstream ss;
    ss << "t" << task->id << "(" << sim_state.benchmark[task] << ")";
    return ss.str();
}

inline
void
simulation_t::check_csv_data(model_task_t *task){
    task_name_t &taskName = sim_state.benchmark[task];
    //std::cout << taskName << "\n";
    //std::cout << task->curr_mapping << "\n";
    //std::cout << sim._core_list[task->curr_mapping].arch << "\n";
    //we have CSV data for the chosen arch
    assert(inputData.samples[taskName].find(task_curr_core_type(task))
            !=inputData.samples[taskName].end());
    //we have CSV data for the chose frequency
    assert(inputData.samples[taskName][task_curr_core_type(task)].find(task_curr_core_freq(task))
            !=inputData.samples[taskName][task_curr_core_type(task)].end());
}

void
simulation_t::sim_init(verbosity_level verbose)
{
    if(vb_lv(verbose,VB_BASIC)) std::cout << "Creating simulation data \n";

    assert(inputData.samples.size() > 0);

    bool err = false;
    for (auto bench : taskConf){
        if(inputData.samples.find(bench.first)==inputData.samples.end()){
            std::cerr << "Can't find benchmark " << bench.first << "\n";
            err = true;
        }
    }
    for (auto bench : inputData.samples){
        for (auto coreArch : coreConf) {
            if(bench.second.find(coreArch.first) == bench.second.end()){
                std::cerr << "No data for benchmark " << bench.first << "at arch " << archToString(coreArch.first) << "\n";
                err = true;
            }
        }
    }
    if(err) abort();

    sim_init_cores();

    sim_init_tasks();

    sim_state.currentTime = 0;
    sim_state.minor_epoch_count = 0;
    sim_state.major_epoch_count = 0;
    sim_state.commitedTime = 0;
}


void
simulation_t::sim_init_add_core_data(model_core_t *core)
{
    system_data.push_back(core_data_t(core));
    assert(core->info->position == (int)system_data.size()-1);
    core_data_t *coreData = &(system_data[system_data.size()-1]);

    for(int freq = 0; freq < SIZE_COREFREQ; ++freq){
        core_freq_t _freq = (core_freq_t)freq;
        coreData->_acc_instr = 0;
        coreData->_acc_idle_cycles[_freq] = 0;
        coreData->_acc_active_cycles[_freq] = 0;
        coreData->_acc_power_active_times_cycles[_freq] = 0;
        coreData->_acc_power_idle_times_cycles[_freq] = 0;
    }
    coreData->_acc_power_gated_time = 0;
}

void
simulation_t::sim_init_cores()
{
    //first create all cores
    for(auto coreType : coreConf){
        for (int i = 0; i < coreType.second.cluster_cnt(); ++i){
            for (int j = 0; j < coreType.second.cores_per_cluster(); ++j){
                _core_list.push_back(model_core_t());
                _core_info_list.push_back(core_info_t());
                _core_systask_list.push_back(model_systask_t());
                freq_domain_list.push_back(model_freq_domain_t());
                _freq_domain_info_list.push_back(freq_domain_info_t());
                power_domain_list.push_back(model_power_domain_t());
                _power_domain_info_list.push_back(power_domain_info_t());
            }
        }
    }


    //creates the domains
    model_core_t *cores = _core_list.data();
    core_info_t *cores_info = _core_info_list.data();
    model_systask_t *systasks = _core_systask_list.data();
    int core_idx = 0;
    _freq_domain_list_size = 0;
    _power_domain_list_size = 0;
    for(auto coreType : coreConf){
        for (int i = 0; i < coreType.second.cluster_cnt(); ++i){
            model_freq_domain_t* freq = &(freq_domain_list.data()[_freq_domain_list_size]);
            freq_domain_info_t* freq_info = &(_freq_domain_info_list.data()[_freq_domain_list_size]);
            freq_domain_info_init(freq_info,_freq_domain_list_size,nullptr,nullptr);
            vitamins_freq_domain_init(freq,freq_info,vitamins_arch_highest_freq_available(coreType.first));
            ++_freq_domain_list_size;
            for (int j = 0; j < coreType.second.cores_per_cluster(); ++j){
                core_info_t *core_info = &(cores_info[core_idx]);
                model_core_t *core = &(cores[core_idx]);
                model_systask_t *systask = &(systasks[core_idx]);

                power_domain_info_t *power_info = &(_power_domain_info_list.data()[_power_domain_list_size]);
                model_power_domain_t *power = &(power_domain_list.data()[_power_domain_list_size]);
                power_domain_info_init(power_info,_power_domain_list_size,freq_info);
                vitamins_power_domain_init(power,power_info);
                ++_power_domain_list_size;

                core_info_init(core_info,coreType.first,core_idx,freq_info,power_info);
                vitamins_core_init(core,core_info);
                vitamins_sys_task_init(systask,core);

                sim_init_add_core_data(core);

                ++core_idx;
            }
        }
    }
}

void
simulation_t::sim_init_tasks()
{
    int taskID = 0;
    for(auto taskName : taskConf){
        assert(inputData.samples.find(taskName.first)!=inputData.samples.end());
        for (int i = 0; i < taskName.second; ++i){
            model_task_t *task = new model_task_t;

            vitamins_task_init(task,++taskID);

            sim_state.currInstr[task] = 0;
            sim_state.currInstrOracle[task] = 0;
            sim_state.benchmark[task] = taskName.first;

            _initial_task_list.push_back(task);
            _current_task_list.push_back(task);
            task_sched_info[task] = new task_epoch_info;
            task_raw_counters[task] = new vitamins_task_sensed_data_raw_t;
        }
    }
}

void
simulation_t::reset_task_counters(simulation_t::vitamins_task_sensed_data_raw_t *counters)
{
    counters->sumInstr = 0;
    counters->sumMemInstr = 0;
    counters->sumBRInstr = 0;
    counters->sumBRMisspred = 0;
    counters->sumFPInstr = 0;
    for(int freq = 0; freq < SIZE_COREFREQ; ++freq){
        counters->sumCyclesActive[freq] = 0;
        counters->sumPowerTimesCycles[freq] = 0;
        counters->sumPowerActiveTimesCycles[freq] = 0;
    }
    counters->sumiTLBaccesses = 0;
    counters->sumiTLBmisses = 0;
    counters->sumdTLBaccesses = 0;
    counters->sumdTLBmisses = 0;
    counters->sumICacheHits = 0;
    counters->sumICacheMisses = 0;
    counters->sumDCacheHits = 0;
    counters->sumDCacheMisses = 0;
    counters->sumL2CacheHits = 0;
    counters->sumL2CacheMisses = 0;
    counters->sumNivcsw = 0;
    counters->sumNvcsw = 0;
}


//return the last instruciton executed
//or 0 if the task has finished in this window
int64_t
simulation_t::average_data(model_task_t *task,
        std::vector<task_data_t> &samples,std::vector<task_data_t>::iterator begin, int numOfSamples,
        core_arch_t arch, core_freq_t freq,
        task_epoch_info *sched_info)
{
    //std::cout << "Averaging out samples for " << task_string(sim,task)
    //          << " time=" << begin->curr_time
    //          << " instr=" << begin->curr_instr << "\n" ;

    assert(freq != COREFREQ_0000MHz);//cannot be power gated and running
    assert(numOfSamples >= 1);
    assert(begin->data_format == task_data_t::CSV_PRED_AND_SIM);
    assert(freq ==  begin->data.conf_freq);
    assert(task_curr_core_freq(task) == freq);
    assert(sched_info != nullptr);
    assert(task != nullptr);

    std::vector<task_data_t>::iterator curr = begin;
    std::vector<task_data_t>::iterator last = curr;
    int sampleCnt = 0;

    vitamins_task_sensed_data_raw_t *counters = &(sched_info->counters);

    uint64_t maxCyclesActive = std::numeric_limits<uint64_t>::max();
    maxCyclesActive = sched_info->sched.run_time * freqToValMHz_i(freq) * 1000000;

    //std::cout << "Task " << task_string(task) << " maxCyclesActive=" << maxCyclesActive << " sampleCnt=" << sampleCnt << " numOfSamples=" << numOfSamples << "\n";

    uint64_t sumCyclesActiveNext = 0;

    counters->last_dvfs_epoch_freq = freq;
    counters->last_dvfs_epoch_sumCyclesActive = 0;
    counters->last_dvfs_epoch_sumInstr = 0;
    counters->last_dvfs_epoch_sumPowerTimesCycles = 0;

    for (; (curr != samples.end()) && (sampleCnt < numOfSamples); ++curr, ++sampleCnt) {

        sumCyclesActiveNext += curr->data.busyCycles + curr->data.idleCycles;
        if((sumCyclesActiveNext > maxCyclesActive) && (sampleCnt >= 1)) break;

        counters->sumInstr += curr->data.commitedInsts;
        counters->sumMemInstr += curr->data.commitedMemRefs;
        counters->sumBRInstr += curr->data.commitedBranches;
        counters->sumBRMisspred += curr->data.branchMispredicts;
        counters->sumFPInstr += curr->data.commitedFPInsts;
        counters->sumCyclesActive[freq] += curr->data.busyCycles + curr->data.idleCycles;
        double pT = curr->data.avgDynPower + curr->data.avgLeakPower + curr->data.l2TotalAvgPower;
        double pI = curr->data.gateLeakPower + curr->data.gatedSubThrLeakPower + curr->data.l2GateLeakPower + curr->data.l2SubThrLeakPower;
        double u = (double)(curr->data.busyCycles + curr->data.idleCycles)/(double)(curr->data.busyCycles + curr->data.quiesceCycles + curr->data.idleCycles);
        double pA = (pT - (pI * (1-u)))/u;
        counters->sumPowerTimesCycles[freq] += (uint64_t)(pT * (curr->data.busyCycles + curr->data.quiesceCycles + curr->data.idleCycles));
        counters->sumPowerActiveTimesCycles[freq] += (uint64_t) (pA * (curr->data.busyCycles + curr->data.idleCycles));
        counters->sumiTLBaccesses += curr->data.itlbAccesses;
        counters->sumiTLBmisses += curr->data.itlbMisses;
        counters->sumdTLBaccesses += curr->data.dtlbAccesses;
        counters->sumdTLBmisses += curr->data.dtlbMisses;
        counters->sumICacheHits += curr->data.iCacheHits;
        counters->sumICacheMisses += curr->data.iCacheMisses;
        counters->sumDCacheHits += curr->data.dCacheHits;
        counters->sumDCacheMisses += curr->data.dCacheMisses;
        counters->sumL2CacheHits += curr->data.l2CacheHits;
        counters->sumL2CacheMisses += curr->data.l2CacheMisses;

        counters->last_dvfs_epoch_sumCyclesActive += curr->data.busyCycles + curr->data.idleCycles;
        counters->last_dvfs_epoch_sumInstr += curr->data.commitedInsts;
        counters->last_dvfs_epoch_sumPowerTimesCycles += (uint64_t)(pT * (curr->data.busyCycles + curr->data.quiesceCycles + curr->data.idleCycles));

        last = curr;
    }

    //std::cout << "Task " << task_string(task) << ": " << counters->sumCyclesActive[freq] << ", from " << begin->curr_instr <<  " to " << last->curr_instr << "\n";

    assert(sampleCnt >= 1);

    counters->sumNivcsw += sched_info->sched.nivcsw;
    counters->sumNvcsw += sched_info->sched.nvcsw;

    //check if has finished
    if (curr != samples.end()) return last->curr_instr; //last instruction executed
    else                       return 0;                //task finished
}

void
simulation_t::average_data_for_estm(model_task_t *task,
        std::vector<task_data_t> &samples,std::vector<task_data_t>::iterator begin, int numOfSamples,
        core_arch_t arch, core_freq_t freq)
{
    assert(freq != COREFREQ_0000MHz);//cannot be power gated and running
    //std::cout << "Averaging out samples for " << task_string(sim,task)
    //          << " time=" << begin->curr_time
    //          << " instr=" << begin->curr_instr << "\n" ;

    std::vector<task_data_t>::iterator curr = begin;
    std::vector<task_data_t>::iterator last = curr;
    int sampleCnt = 0;

    uint64_t sumInstr = 0;
    uint64_t sumCycles = 0;
    uint64_t sumCyclesActive = 0;
    uint64_t sumPowerTimesCycles = 0;
    uint64_t sumPowerActiveTimesCycles = 0;

    int64_t maxCyclesActive = std::numeric_limits<int64_t>::max();
    for (; (curr != samples.end()) && (sampleCnt < numOfSamples); ++curr, ++sampleCnt) {

        int64_t sumCyclesActiveNext = sumCyclesActive + curr->data.busyCycles + curr->data.idleCycles;
        if((sumCyclesActiveNext > maxCyclesActive) && (sampleCnt >= 1)) break;

        sumInstr += curr->data.commitedInsts;
        sumCycles += curr->data.busyCycles + curr->data.quiesceCycles + curr->data.idleCycles;
        sumCyclesActive = sumCyclesActiveNext;
        double pT = curr->data.avgDynPower + curr->data.avgLeakPower + curr->data.l2TotalAvgPower;
        double pI = curr->data.gateLeakPower + curr->data.gatedSubThrLeakPower + curr->data.l2GateLeakPower + curr->data.l2SubThrLeakPower;
        double u = (double)(curr->data.busyCycles + curr->data.idleCycles)/(double)(curr->data.busyCycles + curr->data.quiesceCycles + curr->data.idleCycles);
        double pA = (pT - (pI * (1-u)))/u;
        sumPowerTimesCycles += (uint64_t)(pT * (curr->data.busyCycles + curr->data.quiesceCycles + curr->data.idleCycles));
        sumPowerActiveTimesCycles += (uint64_t) (pA * (curr->data.busyCycles + curr->data.idleCycles));

        last = curr;
    }


    assert(sampleCnt >= 1);
    assert(numOfSamples >= 1);


    assert(task->tlc[arch][freq] == 0);

    assert(begin->data_format == task_data_t::CSV_PRED_AND_SIM);

    task->ips_active[arch][freq] = (int64_t) ((sumCyclesActive != 0)?(((double)sumInstr / (sumCyclesActive)) * freqToValMHz_i(freq)):0);

    task->power_active[arch][freq] = CONV_DOUBLE_scaledINT32((sumCyclesActive != 0)?((double)sumPowerActiveTimesCycles / (sumCyclesActive)):0);

    task->tlc[arch][freq] = CONV_DOUBLE_scaledINT32((double)sumCyclesActive / sumCycles);

    assert(task->tlc[arch][freq] > 0);

}


std::vector<task_data_t>::iterator
find_starting_sample(std::vector<task_data_t> &samples,  int64_t currInstr)
{
   // std::cout << "Searching out samples"
   //               << " currInstr=" << currInstr<< "\n" ;

    std::vector<task_data_t>::iterator startingSample = samples.begin();
    std::vector<task_data_t>::iterator startingSamplePrev = samples.end();

    //find the first one that cover the next instructions to be executed
    for(;startingSample != samples.end(); ++startingSample){
        //std::cout << "\t\t"<< startingSample->curr_instr<<"\n";
        if(startingSample->curr_instr >= currInstr) break;
        startingSamplePrev = startingSample;
    }

    //we handle this in the caller
    if(startingSample == samples.end()) {
        //std::cout << "DOne at starting sample. lenght="<< samples.size() <<"\n";
        return startingSample;
    }

    //best case
    //we are in synch and should start with the next one
    if(startingSample->curr_instr == currInstr) return ++startingSample;

    //if startingSamplePrev was not set
    //out only guess is the current one
    if(startingSamplePrev == samples.end()) return startingSample;

    //our target instruction is somewhere between startingSample and startingSamplePrev

    if((startingSample->curr_instr - currInstr) >= (currInstr - startingSamplePrev->curr_instr)){
        //closer to startingSample, so we still take the next one
        //now we "drop" up-to 1/2 of a sample
        return ++startingSample;
    }
    else{
        //closer to startingSamplePrev, in this case we take startingSample
        //it means that we are executing up-to 1/2 of extra instructions
        return startingSample;
    }

    assert(false);//shouldn't be here
    return startingSample;
}


//returns the currInstr after the elapsed time or 0 if the task finished
int64_t
simulation_t::search_samples_and_average(model_task_t *task, std::vector<task_data_t> &samples,
        int64_t currInstr, int windowSize,
        core_arch_t arch, core_freq_t freq,
        task_epoch_info *sched_info)
{

    std::vector<task_data_t>::iterator startingSample = find_starting_sample(samples,currInstr);

    if(startingSample == samples.end()){
        //this task should have already finished in this core, so we don't estimate
        return 0;
    }

    return average_data(task, samples, startingSample, windowSize, arch, freq, sched_info);

}

template<typename INT_TYPE, typename FLOAT_TYPE>
INT_TYPE round_nearest(FLOAT_TYPE c){
    /// for example, let's say "c = 2.9" ///
    INT_TYPE a = c; // a = 2.9, because 'a' is an 'int', it will take on the value 2 and nothing after the decimal
    c = c - a; // c = 2.9 - 2, now 'c' will equal .9
    int b = c; // b = .9, also because 'b' is an 'int' it will do the same thing 'a' did, so "b = 0"
    if (c > FLOAT_TYPE(.5)){ // "if (.9 > .5)" This statement checks to see which whole number 'c' is currently closest to via 0 or 1
        b = 1; // if it's over .5 then it's closer to 1
    }
    else{
        b = 0; // else it must be closer to 0
    }
    c = a + b; // "c = 2 + 1" Now 'c' is at the value 3
    return c; // return 3
}

static
int
window_size(double timeToAdvance, double sampleRate)
{
    double windowFP = timeToAdvance/sampleRate;
    double windowINT = round_nearest<long long int>(windowFP);
    double diff = 0;
    //std::cout << windowFP << "  " << windowINT << "\n";
    if(windowFP>windowINT) diff = windowFP-windowINT;
    else                   diff = windowINT-windowFP;
    //std::cout << windowFP << "\n";
    //std::cout << windowINT << "\n";
    //std::cout << timeToAdvance << "\n";
    //std::cout << sampleRate << "\n";
    assert((diff<=0.5)&&"windows are not aligned");
    return (int)windowINT;
}

inline
void
simulation_t::print_mapping(bool curr){
    for (auto core : _core_list){
    	if(vitamins_load_tracker_get() == LT_CFS)
    		std::cout << "\tcore "
        		  << core.info->position
        		  << " util=" << core.load_tracking.common.load
        		  << " freq=" << freqToString(core.info->freq->this_domain->last_pred_freq)
				  << " power=" << core_total_power(&(_core_list.data()[core.info->position]))
				  << "\n";
    	else
    		std::cout << "\tcore "
    				<< core.info->position
    		        << " util=" << core.load_tracking.common.load
    				<< "\n";

        bool notask = true;
        for(auto t : _current_task_list){
            model_core_t* cmap = curr ? task_curr_core(t) : task_next_core(t);
            if(cmap->info->position == core.info->position){
            	notask = false;
            	if(vitamins_load_tracker_get() == LT_CFS)
            		std::cout << "\t\t" << task_string(t)
                    << " IPS=" << t->ips_active[core.info->arch][cmap->info->freq->this_domain->last_pred_freq]
                    << " power=" << t->power_active[core.info->arch][cmap->info->freq->this_domain->last_pred_freq]
                    << " tlc=" << t->tlc[core.info->arch][cmap->info->freq->this_domain->last_pred_freq]
					<< " share=" << vitamins_load_tracker_task_load(t) << "\n";
            	else
            		std::cout << "\t\t" << task_string(t)
            		<< " share=" << vitamins_load_tracker_task_load(t) << "\n";
            }
        }
        if(notask) std::cout <<"\t\tno task\n";
    }
    if(vitamins_load_tracker_get() == LT_CFS)
    	std::cout << "\tTotalIPS=" << system_total_ips(vitamins_sys())
    		   << " TotalPower=" << system_total_power(vitamins_sys())
    		   << "\n";
}

void
simulation_t::print_curr_mapping()
{
    print_mapping(true);
}
void
simulation_t::print_next_mapping()
{
    print_mapping(false);
}

void
simulation_t::advance_time_preconditions(double timeToAdvance)
{
    //checks
    assert(timeToAdvance>0);

    assert(_current_task_list.size() > 0);

    //check if the available flags match the next mapping
    //for(auto taskA : _current_task_list) assert(task_next_core(taskA)->load_tracking.common.load != 0);
    //for(auto taskA : _current_task_list) assert(task_next_core(taskA)->load_tracking.common.task_cnt >= 1);

    //core occupation can be set to anything larger then 0 if there are threads on it,
    //but it cannot be larger than 1.0 (100%)
    //for(auto core : _core_list) assert(core.load_tracking.common.load <= CONV_DOUBLE_scaledINT32(1.0));

    //number of occpied cores must be <= the number of tasks
    //unsigned occcores = 0;
    //for(auto c : _core_list) occcores += (c.load_tracking.common.load==0) ? 0 : 1;
    //assert(occcores <= _current_task_list.size());
}


void
simulation_t::run_initial_estimation(double timeToAdvance, int window, model_task_t* task)
{
    core_arch_t currTypeArch = task_curr_core_type(task);
    core_freq_t currTypeFreq = task_curr_core_freq(task);
    int64_t currInstr = sim_state.currInstr[task];

    for(int i = 0;  i < SIZE_COREARCH ; ++i)
        for(int j = 0;  j < SIZE_COREFREQ ; ++j)
            task->tlc[i][j] = 0; //this is for bug catching in the future

    //run once as if the task has full access to the core in order to get task->tlc
    std::vector<task_data_t> &samples = *(inputData.samples[sim_state.benchmark[task]][currTypeArch][currTypeFreq]);

    std::vector<task_data_t>::iterator startingSample = find_starting_sample(samples,currInstr);

    if(startingSample == samples.end()) return;

    average_data_for_estm(task, samples, startingSample, window, currTypeArch, currTypeFreq);
}

int64_t
simulation_t::run_final_estimation(int window, model_task_t* task, double estm_max_runtime)
{
    core_arch_t currTypeArch = task_curr_core_type(task);
    core_freq_t currTypeFreq = task_curr_core_freq(task);
    int64_t currInstr = sim_state.currInstr[task];

    std::vector<task_data_t> &samples = *(inputData.samples[sim_state.benchmark[task]][currTypeArch][currTypeFreq]);
    return  search_samples_and_average(task,
                                       samples, currInstr, window,
                                       currTypeArch, currTypeFreq,
                                       task_sched_info[task]);
}

void
simulation_t::run_oracle(double timeToAdvance,model_task_t* task){
    //generate the entire data array
    int window = window_size(timeToAdvance,inputData.sampleRate);

    for (auto arch : coreConf){
        auto allTaskData = inputData.samples[sim_state.benchmark[task]];
        for(auto sampleData : allTaskData[arch.first]){
            std::vector<task_data_t> &samples = *(sampleData.second);
            std::vector<task_data_t>::iterator startingSample = find_starting_sample(samples,sim_state.currInstrOracle[task]);

            if(startingSample == samples.end()) continue;

            average_data_for_estm(task, samples, startingSample, window, arch.first, sampleData.first);
        }
    }

    sim_state.currInstrOracle[task] = sim_state.currInstr[task];
}

void
simulation_t::run_linsched(double timeToAdvance, verbosity_level verbose)
{
    Linsched proxy(_core_list.size());

    int maxTasksPerCore = 0;

    for(unsigned i = 0; i < _core_list.size(); ++i){
        int cnt = 0;
        for (auto task : _current_task_list)
            if(task_curr_core_idx(task) == (int)i)
                ++cnt;
        maxTasksPerCore = (cnt > maxTasksPerCore ? cnt : maxTasksPerCore);
    }

    for (auto task : _current_task_list){
        core_arch_t currTypeArch = task_curr_core_type(task);
        core_freq_t currTypeFreq = task_curr_core_freq(task);
        if(task->tlc[currTypeArch][currTypeFreq] == 0){
            //may happend when the task is about to finish
            task->tlc[currTypeArch][currTypeFreq] = CONV_DOUBLE_scaledINT32(0.01);//give it 1% util
        }
        double tlc = CONV_scaledINTany_DOUBLE(task->tlc[currTypeArch][currTypeFreq]);
        task->tlc[currTypeArch][currTypeFreq] = 0;
        if(vb_lv(verbose,VB_FULL)) std::cout << task_string(task) << " has tlc of " << tlc << "\n";

        double runPeriod = timeToAdvance*tlc;
        double sleepPeriod = timeToAdvance*(1-tlc);
        //cannot be less then 1ms (linsched shit)
        assert(runPeriod > 0);
        if(runPeriod < 0.001){
            sleepPeriod *= 0.001/runPeriod;
            runPeriod = 0.001;
        }
        //std::cout << task_string(simData,task) << " has a run_period of " << runPeriod << "\n";

        task_sched_info[task]->sched.task_id =
                proxy.create_task(runPeriod,sleepPeriod, task_curr_core_idx(task),0);
    }
    double simTimeScaling = maxTasksPerCore * 10;

    proxy.sim(timeToAdvance*simTimeScaling);

    for (auto task : _current_task_list){
        proxy.task_info(task_sched_info[task]->sched.task_id,&(task_sched_info[task]->sched));
        if(vb_lv(verbose,VB_FULL)) std::cout << task_string(task) << " on core " << task_curr_core_idx(task) << " has these stats:" <<
                " run_time="<< task_sched_info[task]->sched.run_time <<
                " run_time_share="<< task_sched_info[task]->sched.run_time/(timeToAdvance*simTimeScaling) <<
                " pcount= " << task_sched_info[task]->sched.pcount <<
                " nvcsw= " << task_sched_info[task]->sched.nvcsw <<
                " nivcsw= " << task_sched_info[task]->sched.nivcsw << "\n";

        //fucking task must run at least once
        task_sched_info[task]->sched.run_time /= simTimeScaling;
        assert(task_sched_info[task]->sched.run_time > 0);

    }
}

void simulation_t::reset_minor()
{
    for(auto task : _current_task_list){
        reset_task_counters(&(task_sched_info[task]->counters));
        task_sched_info[task]->counters.last_dvfs_epoch_freq = SIZE_COREFREQ;
        task_sched_info[task]->counters.last_dvfs_epoch_sumCyclesActive = 0;
        task_sched_info[task]->counters.last_dvfs_epoch_sumInstr = 0;
        task_sched_info[task]->counters.last_dvfs_epoch_sumPowerTimesCycles = 0;
    }

    //they are still in the cores task list, so we set this to 0 to prevent them tricking the dvfs policy
    for(auto task : _removed_at_epoch_task_list){
        reset_task_counters(&(task_sched_info[task]->counters));
        task_sched_info[task]->counters.last_dvfs_epoch_freq = SIZE_COREFREQ;
        task_sched_info[task]->counters.last_dvfs_epoch_sumCyclesActive = 0;
        task_sched_info[task]->counters.last_dvfs_epoch_sumInstr = 0;
        task_sched_info[task]->counters.last_dvfs_epoch_sumPowerTimesCycles = 0;
    }
}

void simulation_t::commit_task(model_task_t *task, verbosity_level verbose, bool task_finished)
{
    task_sensed_data_t *ipc_stack = &(task->sensed_data);
    vitamins_task_sensed_data_raw_t *counters = task_raw_counters[task];

    double active_time = 0;
    uint64_t active_cy = 0;
    double avg_freq = 0;
    for(int _f = 0; _f < SIZE_COREFREQ; ++_f){
        core_freq_t f = (core_freq_t)_f;
        active_time += counters->sumCyclesActive[f] * (1 / (freqToValMHz_d(f)*1000000));
        active_cy += counters->sumCyclesActive[f];
        avg_freq += freqToValMHz_d(f) * counters->sumCyclesActive[f];
    }
    avg_freq /= active_cy;
    double ips = (active_time==0) ? 0 : counters->sumInstr/active_time;
    ips /= 1000000; //Minsts/sec or insts/usec

    ipc_stack->ipc_active = (active_cy == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumInstr) / active_cy;

    ipc_stack->ips_active = (uint32_t)ips;

    ipc_stack->avg_freq_mhz = (uint32_t)avg_freq;
    ipc_stack->avg_freq = valMHzToClosestFreq(task_curr_core_type(task),(uint32_t)avg_freq);

    //if(!task_finished){
    //	double ips_check = CONV_scaledINTany_INTany(ipc_stack->ipc_active * ipc_stack->avg_freq_mhz);
    //	assert(std::max(ips_check,ips)/std::min(ips_check,ips) < 1.2);
    //}

    ipc_stack->memInstr_share = (counters->sumInstr == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumMemInstr) / counters->sumInstr;

    ipc_stack->brInstr_share = (counters->sumInstr == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumBRInstr) / counters->sumInstr;

    ipc_stack->fpInstr_share = (counters->sumInstr == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumFPInstr) / counters->sumInstr;

    ipc_stack->br_misspredrate = (counters->sumBRInstr == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumBRMisspred) / counters->sumBRInstr;

    ipc_stack->br_misspreds_perinstr = (counters->sumInstr == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumBRMisspred) / counters->sumInstr;

    ipc_stack->iTLB_missrate = (counters->sumiTLBaccesses == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumiTLBmisses) / counters->sumiTLBaccesses;

    ipc_stack->dTLB_missrate = (counters->sumdTLBaccesses == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumdTLBmisses) / counters->sumdTLBaccesses;

    int64_t auxSum;
    auxSum = counters->sumICacheHits + counters->sumICacheMisses;
    ipc_stack->icache_missrate = (auxSum == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumICacheMisses) / auxSum;

    auxSum = counters->sumDCacheHits + counters->sumDCacheMisses;
    ipc_stack->dcache_missrate = (auxSum == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumDCacheMisses) / auxSum;

    auxSum = counters->sumL2CacheHits + counters->sumL2CacheMisses;
    ipc_stack->l2cache_local_missrate = (auxSum == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumL2CacheMisses) / auxSum;

    ipc_stack->l2cache_global_missrate = (counters->sumMemInstr == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumL2CacheMisses) / counters->sumMemInstr;

    ipc_stack->dcache_misses_perinstr = (counters->sumInstr == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumDCacheMisses) / counters->sumInstr;

    ipc_stack->l2cache_misses_perinstr = (counters->sumInstr == 0) ? 0 :
            CONV_INTany_scaledINTany(counters->sumL2CacheMisses) / counters->sumInstr;


    ipc_stack->nivcsw = counters->sumNivcsw;
    ipc_stack->nvcsw = counters->sumNvcsw;


    double proc_time_share_acc = active_time / sim_state.commitedTime;
    task->sensed_data.proc_time_share = CONV_DOUBLE_scaledINT32(proc_time_share_acc);
    update_proc_time_share_avg(task->sensed_data);

    if(vb_lv(verbose,VB_BASIC))
        std::cout << "\t" << task_string(task) << " share=" << task->sensed_data.proc_time_share << " share_acc=" << task->sensed_data.proc_time_share_avg << "\n";
}

void simulation_t::commit_task_counters(model_task_t *task)
{
    vitamins_task_sensed_data_raw_t *epoch_counters = &(task_sched_info[task]->counters);
    vitamins_task_sensed_data_raw_t *total_counters = task_raw_counters[task];

    if(epoch_counters->last_dvfs_epoch_sumCyclesActive > 0){
        assert(epoch_counters->last_dvfs_epoch_freq != SIZE_COREFREQ);
        assert(epoch_counters->last_dvfs_epoch_freq != COREFREQ_0000MHz);
        assert(epoch_counters->last_dvfs_epoch_sumInstr > 0);
        assert(epoch_counters->last_dvfs_epoch_sumPowerTimesCycles > 0);

        total_counters->sumInstr += epoch_counters->sumInstr;
        total_counters->sumMemInstr += epoch_counters->sumMemInstr;
        total_counters->sumBRInstr += epoch_counters->sumBRInstr;
        total_counters->sumBRMisspred += epoch_counters->sumBRMisspred;
        total_counters->sumFPInstr += epoch_counters->sumFPInstr;


        total_counters->sumCyclesActive[epoch_counters->last_dvfs_epoch_freq] += epoch_counters->sumCyclesActive[epoch_counters->last_dvfs_epoch_freq];
        total_counters->sumPowerTimesCycles[epoch_counters->last_dvfs_epoch_freq] += epoch_counters->sumPowerTimesCycles[epoch_counters->last_dvfs_epoch_freq];
        total_counters->sumPowerActiveTimesCycles[epoch_counters->last_dvfs_epoch_freq] += epoch_counters->sumPowerActiveTimesCycles[epoch_counters->last_dvfs_epoch_freq];
        total_counters->sumiTLBaccesses += epoch_counters->sumiTLBaccesses;
        total_counters->sumiTLBmisses += epoch_counters->sumiTLBmisses;
        total_counters->sumdTLBaccesses += epoch_counters->sumdTLBaccesses;
        total_counters->sumdTLBmisses += epoch_counters->sumdTLBmisses;
        total_counters->sumICacheHits += epoch_counters->sumICacheHits;
        total_counters->sumICacheMisses += epoch_counters->sumICacheMisses;
        total_counters->sumDCacheHits += epoch_counters->sumDCacheHits;
        total_counters->sumDCacheMisses += epoch_counters->sumDCacheMisses;
        total_counters->sumL2CacheHits += epoch_counters->sumL2CacheHits;
        total_counters->sumL2CacheMisses += epoch_counters->sumL2CacheMisses;
        total_counters->sumNivcsw += epoch_counters->sumNivcsw;
        total_counters->sumNvcsw += epoch_counters->sumNvcsw;

    }
    total_counters->last_dvfs_epoch_freq  = epoch_counters->last_dvfs_epoch_freq;
    total_counters->last_dvfs_epoch_sumCyclesActive = epoch_counters->last_dvfs_epoch_sumCyclesActive;
    total_counters->last_dvfs_epoch_sumInstr = epoch_counters->last_dvfs_epoch_sumInstr;
    total_counters->last_dvfs_epoch_sumPowerTimesCycles = epoch_counters->last_dvfs_epoch_sumPowerTimesCycles;
}

double simulation_t::compute_discrepancy(double timeToAdvance,int core)
{
    core_freq_t f = vitamins_dvfs_get_freq(&(_core_list[core]));
    double active_cycles = 0;
    double total_expected_time = 0;

    for(auto task : _current_task_list){
        if(task_curr_core_idx(task) == (int)core){
            vitamins_task_sensed_data_raw_t *sensed_data_raw = &(task_sched_info[task]->counters);

            //check task didn't execute
            if(sensed_data_raw->last_dvfs_epoch_freq == SIZE_COREFREQ){
                //(in)sanity checks
                assert(sensed_data_raw->last_dvfs_epoch_sumCyclesActive == 0);
                assert(sim_state.currInstr[task] == 0);
            }
            else{
                assert(sensed_data_raw->last_dvfs_epoch_freq == f);
                active_cycles += sensed_data_raw->last_dvfs_epoch_sumCyclesActive;
            }

            if(sensed_data_raw->last_dvfs_epoch_sumCyclesActive > 0){
                assert(task_sched_info[task]->sched.run_time > 0);
            }

            total_expected_time += task_sched_info[task]->sched.run_time;

        }
    }
    if(f == COREFREQ_0000MHz) {
        assert(active_cycles == 0);
        return 0;
    }

    double total_cycles = (timeToAdvance / (1.0/(freqToValMHz_d(f)*1000000)));

    double util = active_cycles / total_cycles;
    double expected_util = total_expected_time/timeToAdvance;

    if(util >= expected_util) return 0;//this shouldn't happen very often

    return 1 - (util/expected_util);
}

model_task_t* simulation_t::largest_discrepancy(double timeToAdvance,int core)
{
    core_freq_t f = vitamins_dvfs_get_freq(&(_core_list[core]));
    double total_cycles = (timeToAdvance / (1.0/(freqToValMHz_d(f)*1000000)));
    double max_error = 0;
    model_task_t* max_error_task = nullptr;

    //we don't fix core with that had a finished task
    for(auto task : _current_task_list){
        if(task_curr_core_idx(task) == (int)core){
            if(sim_state.currInstr[task] == 0) return nullptr;
        }
    }

    for(auto task : _current_task_list){
        if(task_curr_core_idx(task) == (int)core){
            vitamins_task_sensed_data_raw_t *sensed_data_raw = &(task_sched_info[task]->counters);
            if((sensed_data_raw->last_dvfs_epoch_freq != SIZE_COREFREQ) && (sensed_data_raw->last_dvfs_epoch_sumCyclesActive > 0)){
                assert(sensed_data_raw->last_dvfs_epoch_freq == f);
                double util =  sensed_data_raw->last_dvfs_epoch_sumCyclesActive/total_cycles;
                double expected_util = task_sched_info[task]->sched.run_time/timeToAdvance;

                if(util >= expected_util)
                    util = expected_util;//shouldn't happend very often

                double error = 1- (util/expected_util);

                if(error > max_error){
                    max_error = error;
                    max_error_task = task;
                }

            }
        }
    }

    assert(max_error_task != nullptr);

    return max_error_task;
}

void simulation_t::fix_discrepancy(model_task_t* task, double error)
{
    vitamins_task_sensed_data_raw_t *epoch_counters = &(task_sched_info[task]->counters);
    double fix = 1 + error;

    assert(epoch_counters->last_dvfs_epoch_freq != SIZE_COREFREQ);
    assert(epoch_counters->last_dvfs_epoch_freq != COREFREQ_0000MHz);

    epoch_counters->sumInstr = (uint64_t)(fix * epoch_counters->sumInstr);
    epoch_counters->sumMemInstr = (uint64_t)(fix * epoch_counters->sumMemInstr);
    epoch_counters->sumBRInstr = (uint64_t)(fix * epoch_counters->sumBRInstr);
    epoch_counters->sumBRMisspred = (uint64_t)(fix * epoch_counters->sumBRMisspred);
    epoch_counters->sumFPInstr = (uint64_t)(fix * epoch_counters->sumFPInstr);
    epoch_counters->sumCyclesActive[epoch_counters->last_dvfs_epoch_freq] = (uint64_t)(fix * epoch_counters->sumCyclesActive[epoch_counters->last_dvfs_epoch_freq]);
    epoch_counters->sumPowerTimesCycles[epoch_counters->last_dvfs_epoch_freq] = (uint64_t)(fix * epoch_counters->sumPowerTimesCycles[epoch_counters->last_dvfs_epoch_freq]);
    epoch_counters->sumPowerActiveTimesCycles[epoch_counters->last_dvfs_epoch_freq] = (uint64_t)(fix * epoch_counters->sumPowerActiveTimesCycles[epoch_counters->last_dvfs_epoch_freq]);
    epoch_counters->sumiTLBaccesses = (uint64_t)(fix * epoch_counters->sumiTLBaccesses);
    epoch_counters->sumiTLBmisses = (uint64_t)(fix * epoch_counters->sumiTLBmisses);
    epoch_counters->sumdTLBaccesses = (uint64_t)(fix * epoch_counters->sumdTLBaccesses);
    epoch_counters->sumdTLBmisses = (uint64_t)(fix * epoch_counters->sumdTLBmisses);
    epoch_counters->sumICacheHits = (uint64_t)(fix * epoch_counters->sumICacheHits);
    epoch_counters->sumICacheMisses = (uint64_t)(fix * epoch_counters->sumICacheMisses);
    epoch_counters->sumDCacheHits = (uint64_t)(fix * epoch_counters->sumDCacheHits);
    epoch_counters->sumDCacheMisses = (uint64_t)(fix * epoch_counters->sumDCacheMisses);
    epoch_counters->sumL2CacheHits = (uint64_t)(fix * epoch_counters->sumL2CacheHits);
    epoch_counters->sumL2CacheMisses = (uint64_t)(fix * epoch_counters->sumL2CacheMisses);
    epoch_counters->sumNivcsw = (uint32_t)(fix * epoch_counters->sumNivcsw);
    epoch_counters->sumNvcsw = (uint32_t)(fix * epoch_counters->sumNvcsw);

    epoch_counters->last_dvfs_epoch_sumCyclesActive = (uint64_t)(fix * epoch_counters->last_dvfs_epoch_sumCyclesActive);
    epoch_counters->last_dvfs_epoch_sumInstr = (uint64_t)(fix * epoch_counters->last_dvfs_epoch_sumInstr);
    epoch_counters->last_dvfs_epoch_sumPowerTimesCycles = (uint64_t)(fix * epoch_counters->last_dvfs_epoch_sumPowerTimesCycles);
}

void simulation_t::commit_minor_check_error(double timeToAdvance)
{
    for (unsigned int i  = 0; i < system_data.size(); ++i){
        const double error_target = 0.01;

        double discrepancy = compute_discrepancy(timeToAdvance,i);
        while(discrepancy > error_target){
            model_task_t *task = largest_discrepancy(timeToAdvance,i);

            //unable to fix
            if(task == nullptr) break;

            fix_discrepancy(task,discrepancy);

            discrepancy = compute_discrepancy(timeToAdvance,i);
        }
    }
}

void simulation_t::commit_cpu_minor_counters(double timeToAdvance)
{
    //cpu dvfs sensing data
    for (unsigned int i  = 0; i < system_data.size(); ++i){
    	core_freq_t f = vitamins_dvfs_get_freq(&(_core_list[i]));
    	double active_time = 0;
    	double sum_power = 0;
    	int64_t active_cy = 0;
    	int64_t instr = 0;
    	for(auto task : _current_task_list){
    		if(task_curr_core_idx(task) == (int)i){
    			active_cy += task_raw_counters[task]->last_dvfs_epoch_sumCyclesActive;
    			active_time += task_raw_counters[task]->last_dvfs_epoch_sumCyclesActive * (1 / (freqToValMHz_d(f)*1000000));
    			instr += task_raw_counters[task]->last_dvfs_epoch_sumInstr;
    			sum_power += task_raw_counters[task]->last_dvfs_epoch_sumPowerTimesCycles;
    		}
    	}
    	_core_list[i].sensed_data.last_dvfs_epoch_freq = f;
    	_core_list[i].sensed_data.last_dvfs_epoch_sumCyclesActive = active_cy;

    	double ips = (active_time==0) ? 0 : instr/active_time;
    	_core_list[i].sensed_data.last_dvfs_epoch_ips = ips/1000000;

    	double cycles = (timeToAdvance / (1.0/(freqToValMHz_d(f)*1000000)));
//    	uint32_t avg_power = CONV_DOUBLE_scaledUINT32(sum_power/cycles);
    	_core_list[i].sensed_data.last_dvfs_epoch_avg_power = sum_power/cycles;
    }
}

void simulation_t::commit_cpu_major_counters()
{
    //cpu dvfs sensing data
    for (unsigned int i  = 0; i < system_data.size(); ++i){
    	double time = system_data[i].time();
    	assert(std::fabs(time-sim_state.commitedTime) < 0.000001);
    	_core_list[i].sensed_data.avg_load = CONV_DOUBLE_scaledUINT32(system_data[i].time_active() / time);
    	_core_list[i].sensed_data.num_tasks = 0;//set later
    }
    //num of tasks that executed on each core this epoch
    for (auto task : _current_task_list) {
        task_curr_core(task)->sensed_data.num_tasks += 1;
    }
    for(auto task : _removed_at_epoch_task_list) {
    	task_curr_core(task)->sensed_data.num_tasks += 1;
    }

    for (int i  = 0; i < _freq_domain_list_size; ++i){
    	model_freq_domain_t *fd = &(freq_domain_list[i]);
    	double avg_freq = 0;
    	for (unsigned int j  = 0; j < system_data.size(); ++j){
    		if(_core_list[j].info->freq->domain_id == fd->info->domain_id){
    			assert(_core_list[j].info->freq->this_domain == fd);
    			if(avg_freq == 0)
    				avg_freq = system_data[j].avg_freqMHz();
    			else
    				assert(std::fabs(avg_freq - system_data[j].avg_freqMHz())< 0.000001);
    		}
    	}
    	fd->sensed_data.avg_freqMHz = CONV_DOUBLE_scaledUINT32(avg_freq);
    }

    for (int i  = 0; i < _power_domain_list_size; ++i){
    	model_power_domain_t *pd = &(power_domain_list[i]);
    	double power = 0;
    	for (unsigned int j  = 0; j < system_data.size(); ++j){
    		if(_core_list[j].info->power->domain_id == pd->info->domain_id){
    			assert(_core_list[j].info->power->this_domain == pd);
    			power += system_data[j].power();
    		}
    	}
    	pd->sensed_data.avg_power = CONV_DOUBLE_scaledUINT32(power);
    }

}

void simulation_t::commit_cpu_minor_stats(double timeToAdvance,verbosity_level verbose)
{
    for (unsigned int i  = 0; i < system_data.size(); ++i){
        core_freq_t f = vitamins_dvfs_get_freq(&(_core_list[i]));

        //need this for debugging only
        double active_cy = 0;
        for(auto task : _current_task_list){
            if(task_curr_core_idx(task) == (int)i){
                //check task didn't execute
                if(task_raw_counters[task]->last_dvfs_epoch_freq == SIZE_COREFREQ){
                    //(in)sanity checks
                    assert(task_raw_counters[task]->last_dvfs_epoch_sumCyclesActive == 0);
                    assert(sim_state.currInstr[task] == 0);
                }
                else{
                    assert(task_raw_counters[task]->last_dvfs_epoch_freq == f);
                    active_cy += task_raw_counters[task]->last_dvfs_epoch_sumCyclesActive;
                }

            }
        }

        if(f == COREFREQ_0000MHz){
            system_data[i]._power_gated_time += timeToAdvance;
            assert(active_cy == 0);
            if(vb_lv(verbose,VB_BASIC))
                std::cout << "\tCore " << i << " is power gated\n";
        }
        else{
            double cycles = (timeToAdvance / (1.0/(freqToValMHz_d(f)*1000000)));
            system_data[i]._total_cycles[f] = system_data[i]._total_cycles[f] + cycles;
            //for debuging
            if(vb_lv(verbose,VB_BASIC))
                std::cout << "\tCore " << i << " util=" << active_cy/cycles
                          << " @ " << freqToValMHz_d(f) << "MHz / "
                          << "arch = " << archToString(_core_list[i].info->arch) << " / "
                          << "freq_domain = " << _core_list[i].info->freq->domain_id <<  "\n";
        }


    }
}

void simulation_t::commit_minor(double timeToAdvance,verbosity_level verbose)
{

    commit_minor_check_error(timeToAdvance);

    for(auto task : _current_task_list){
        commit_task_counters(task);
    }
    for(auto task : _removed_at_epoch_task_list){
        commit_task_counters(task);
    }

    commit_cpu_minor_counters(timeToAdvance);

    sim_state.commitedTime += timeToAdvance;

    commit_cpu_minor_stats(timeToAdvance,verbose);
}

void simulation_t::reset_major(bool enable_oracle)
{
    //std::cout << "New epoch ! Reseting counters\n";
    for(auto task : _current_task_list)
        reset_task_counters(task_raw_counters[task]);

    for (unsigned int i  = 0; i < system_data.size(); ++i){
        for(int _f = 0; _f < SIZE_COREFREQ; ++_f){
            core_freq_t f = (core_freq_t)_f;
            system_data[i]._instr = 0;
            system_data[i]._total_cycles[f] = 0;
            system_data[i]._active_cycles[f] = 0;
            system_data[i]._idle_cycles[f] = 0;
            system_data[i]._power_active_times_cycles[f] = 0;
            system_data[i]._power_idle_times_cycles[f] = 0;
        }
        system_data[i]._power_gated_time = 0;
    }

    _removed_at_epoch_task_list.clear();
}

void simulation_t::commit_major(verbosity_level verbose)
{

    for(auto task : _current_task_list){
        commit_task(task,verbose,false);
    }
    for(auto task : _removed_at_epoch_task_list){
        commit_task(task,verbose,true);
    }

    commit_cpu_major_stats();

    commit_cpu_major_counters();

}

void simulation_t::commit_cpu_major_stats()
{
	commit_cpu_epoch_stats();
	commit_cpu_overall_stats();
}

void simulation_t::commit_cpu_epoch_stats()
{
	for (unsigned int i  = 0; i < system_data.size(); ++i){
		core_arch_t arch = _core_list[i].info->arch;

		for(auto task : _current_task_list)
			if(task_curr_core_idx(task) == (int)i)
				system_data[i]._instr += task_raw_counters[task]->sumInstr;
		for(auto task : _removed_at_epoch_task_list)
			if(task_curr_core_idx(task) == (int)i)
				system_data[i]._instr += task_raw_counters[task]->sumInstr;

		for(int _f = 0; _f < SIZE_COREFREQ; ++_f){
			core_freq_t f = (core_freq_t)_f;
			if(system_data[i]._total_cycles[f] == 0) continue;

			//acc core stats from the tasks
			for(auto task : _current_task_list)
				if(task_curr_core_idx(task) == (int)i){
					system_data[i]._active_cycles[f] = task_raw_counters[task]->sumCyclesActive[f] + system_data[i]._active_cycles[f];
					system_data[i]._power_active_times_cycles[f] = task_raw_counters[task]->sumPowerActiveTimesCycles[f] + system_data[i]._power_active_times_cycles[f];
				}
			for(auto task : _removed_at_epoch_task_list)
				if(task_curr_core_idx(task) == (int)i){
					system_data[i]._active_cycles[f] = task_raw_counters[task]->sumCyclesActive[f] + system_data[i]._active_cycles[f];
					system_data[i]._power_active_times_cycles[f] = task_raw_counters[task]->sumPowerActiveTimesCycles[f] + system_data[i]._power_active_times_cycles[f];
				}


			//adds up idle/inactive time to cpus (does not include power gated time)
			double activeFraction = system_data[i]._active_cycles[f] / system_data[i]._total_cycles[f];
			double idle_cycles = system_data[i]._total_cycles[f] * (1-activeFraction);
			double power_idle = CONV_scaledINTany_DOUBLE(arch_idle_power_scaled(arch,f));

			system_data[i]._idle_cycles[f] = system_data[i]._idle_cycles[f] + idle_cycles;
			system_data[i]._power_idle_times_cycles[f] = system_data[i]._power_idle_times_cycles[f] + (idle_cycles*power_idle);
		}
	}
}

void simulation_t::commit_cpu_overall_stats()
{
    if((_current_task_list.size() != _initial_task_list.size()) && !system_data_mirrors_set){
        //first epoch with a finished task
        //save the accumulators from the last epoch, which had all tasks active
        system_data_mirrors_set = true;
        for (unsigned int i  = 0; i < system_data.size(); ++i) system_data[i].set_mirrors();
    }

    for (unsigned int i  = 0; i < system_data.size(); ++i){
        for(int _f = 0; _f < SIZE_COREFREQ; ++_f){
            core_freq_t f = (core_freq_t)_f;

            system_data[i]._acc_active_cycles[f] = system_data[i]._acc_active_cycles[f] + system_data[i]._active_cycles[f];
            system_data[i]._acc_power_active_times_cycles[f] = system_data[i]._acc_power_active_times_cycles[f] + system_data[i]._power_active_times_cycles[f];

            system_data[i]._acc_idle_cycles[f] = system_data[i]._acc_idle_cycles[f] + system_data[i]._idle_cycles[f];
            system_data[i]._acc_power_idle_times_cycles[f] = system_data[i]._acc_power_idle_times_cycles[f] + system_data[i]._power_idle_times_cycles[f];
        }
        system_data[i]._acc_instr += system_data[i]._instr;
        system_data[i]._acc_power_gated_time = system_data[i]._acc_power_gated_time + system_data[i]._power_gated_time;
    }
}



/*
 * advance the current time by timeToAdvance
 *
 * for each task, search the vectors of inputData to findout in which instruction it should be
 * after executing for the amount of time specified by timeToAdvance
 *
 * the data inside simData.current_task_list of each task should contain the average of all the data samples
 * between curr_time and currentTime+timeToAdvance
 *
 * tasks that finish are removed from simData.current_task_list
 *
 * simData.system_data contains average system metrics between currentTime+timeToAdvance
 *
 *
 * currentTime is set to currentTime+timeToAdvance
 *
 */
bool
simulation_t::advance_time(double timeToAdvance, verbosity_level verbose)
{

    advance_time_preconditions(timeToAdvance);

    double nextTime = sim_state.currentTime + timeToAdvance;

    if(vb_lv(verbose,VB_BASIC))
        std::cout << "Advancing time by " << timeToAdvance << " sec. Curr time = " << sim_state.currentTime << " next time = " << nextTime << "\n";

    int window = window_size(timeToAdvance,inputData.sampleRate);
    assert((window >= 1)&&"Sampling rate is to low for the chosen epoch length");//if epoch < 50ms this may be violated

    std::vector<model_task_t*> toRemove;

    //set initial cpu stats
    reset_minor();
    if(sim_state.commitedTime == 0){
        reset_major(_using_oracle);
    }

    for (auto task : _current_task_list){
        //implement the mapping
    	task_commit_map(task);

        //wake core if it was power-gated
        if(vitamins_dvfs_is_power_gated(task_curr_core(task)))
            vitamins_dvfs_wakeup_core(task_curr_core(task));

        check_csv_data(task);

        //run once as if the task has full access to the core in order to get task->tlc
        //and get the map of tasks on each cpu
        run_initial_estimation(timeToAdvance,window,task);
    }

    //use linsched to get the proc_time share for all tasks
    //this is set at task->tlc replacing the value set during the initial estimation
    //also sets core.sensed_data.tasks_cnt
    run_linsched(timeToAdvance, verbose);


    for (auto task : _current_task_list){
        //run final estimation
        int64_t futureCurrInstr = run_final_estimation(window,task,timeToAdvance);
        //update the state
        if (futureCurrInstr != 0){
            sim_state.currInstr[task] = futureCurrInstr;
        }
        else{
            toRemove.push_back(task);
            //this will hold the finishing state
            sim_state.currInstr[task] = futureCurrInstr;
            sim_state.finishTime[task] = nextTime;
        }
    }


    //account minor stats
    commit_minor(timeToAdvance,verbose);

    //removed finished tasks
    for (auto toRemoveTask : toRemove){
        auto toRemovePos = _current_task_list.begin();
        for (;toRemovePos != _current_task_list.end(); ++toRemovePos){
            if(*toRemovePos == toRemoveTask) break;
        }
        assert(toRemovePos != _current_task_list.end());
        if(vb_lv(verbose,VB_BASIC))
            std::cout << "\t" << task_string(toRemoveTask)
                  << " has finished at time " << sim_state.finishTime[toRemoveTask] << " at core " << task_curr_core_idx(toRemoveTask) << "\n";
        _current_task_list.erase(toRemovePos);
        assert(task_curr_core(toRemoveTask) ==  task_next_core(toRemoveTask));
        _removed_at_epoch_task_list.push_back(toRemoveTask);
    }

    sim_state.currentTime = nextTime;
    sim_state.minor_epoch_count += 1;

    return _current_task_list.size() > 0;

}

/*
 * advance the current time by timeToAdvance (standalone)
 *
 * same as above, except we continuously accumulate sensor data
 * to emulate system
 */
bool
simulation_t::advance_time_standalone(double timeToAdvance, verbosity_level verbose)
{

//    advance_time_preconditions(timeToAdvance);

    double nextTime = sim_state.currentTime + timeToAdvance;

    if(vb_lv(verbose,VB_BASIC))
        std::cout << "Advancing time by " << timeToAdvance << " sec. Curr time = " << sim_state.currentTime << " next time = " << nextTime << "\n";

    int window = window_size(timeToAdvance,inputData.sampleRate);
    assert((window >= 1)&&"Sampling rate is to low for the chosen epoch length");//if epoch < 50ms this may be violated

    std::vector<model_task_t*> toRemove;

    //set initial cpu stats
    reset_minor();
    if(sim_state.commitedTime == 0){
        reset_major(_using_oracle);
        reset_minor();
    }

    for (auto task : _current_task_list){
        //implement the mapping
    	task_commit_map(task);

        //wake core if it was power-gated
        if(vitamins_dvfs_is_power_gated(task_curr_core(task)))
            vitamins_dvfs_wakeup_core(task_curr_core(task));

        check_csv_data(task);

        //run once as if the task has full access to the core in order to get task->tlc
        //and get the map of tasks on each cpu
        run_initial_estimation(timeToAdvance,window,task);
    }

    //use linsched to get the proc_time share for all tasks
    //this is set at task->tlc replacing the value set during the initial estimation
    //also sets core.sensed_data.tasks_cnt
    run_linsched(timeToAdvance, verbose);


    for (auto task : _current_task_list){
        //run final estimation
        int64_t futureCurrInstr = run_final_estimation(window,task,timeToAdvance);
        //update the state
        if (futureCurrInstr != 0){
            sim_state.currInstr[task] = futureCurrInstr;
        }
        else{
            toRemove.push_back(task);
            //this will hold the finishing state
            sim_state.currInstr[task] = futureCurrInstr;
            sim_state.finishTime[task] = nextTime;
        }
    }

    //account minor stats
    commit_minor(timeToAdvance,verbose);

    //removed finished tasks
    for (auto toRemoveTask : toRemove){
        auto toRemovePos = _current_task_list.begin();
        for (;toRemovePos != _current_task_list.end(); ++toRemovePos){
            if(*toRemovePos == toRemoveTask) break;
        }
        assert(toRemovePos != _current_task_list.end());
        if(vb_lv(verbose,VB_BASIC))
            std::cout << "\t" << task_string(toRemoveTask)
                  << " has finished at time " << sim_state.finishTime[toRemoveTask] << " at core " << task_curr_core_idx(toRemoveTask) << "\n";
//        _current_task_list.erase(toRemovePos);
        assert(task_curr_core(toRemoveTask) ==  task_next_core(toRemoveTask));
        _removed_at_epoch_task_list.push_back(toRemoveTask);
    }

    sim_state.currentTime = nextTime;
    sim_state.minor_epoch_count += 1;

    return _current_task_list.size() > _removed_at_epoch_task_list.size();

}
/////////////////////////////////////////////////////////////////////////////

void
simulation_t::end_epoch(verbosity_level verbose)
{
    //run oracle for all tasks that executed during the epoch (if enabled)
    if(_using_oracle){
        for (auto task : _current_task_list)
            run_oracle(sim_state.commitedTime,task);

        for(auto task : _removed_at_epoch_task_list)
            run_oracle(sim_state.commitedTime,task);
    }

    //acouunt major epoch
    commit_major(verbose);

    sim_state.commitedTime = 0;
    sim_state.major_epoch_count += 1;

    //for(auto data : system_data){
    //    data.dump();
    //}
}

void
simulation_t::core_data_t::set_mirrors()
{
    assert(mirror_acc_power_gated_time == 0);

    mirror_acc_instr = _acc_instr;
    mirror_acc_idle_cycles = _acc_idle_cycles;
    mirror_acc_active_cycles = _acc_active_cycles;
    mirror_acc_power_active_times_cycles = _acc_power_active_times_cycles;
    mirror_acc_power_idle_times_cycles = _acc_power_idle_times_cycles;//see above
    mirror_acc_power_gated_time = _acc_power_gated_time;
}

void
simulation_t::print_system_data(bool print_epoch, bool print_total)
{
    if(print_epoch){
        std::cout << "Last epoch summary:\n";
        double ips = 0;
        double power = 0;
        for(unsigned int i = 0; i < _core_list.size(); ++i){
            std::cout << "Core[" << i << "] type "
                    <<  archToString(_core_list[i].info->arch)
                    << " freq. " << freqToString(vitamins_dvfs_get_freq(&(_core_list[i]))) << "\n";
            std::cout << "\tIPS=" << system_data[i].ips();
            std::cout << " Power=" << system_data[i].power();
            std::cout << " IPS/Watt=" << system_data[i].ips_watt()<<"\n";
            ips += system_data[i].ips();
            power += system_data[i].power();
        }
        std::cout << "System IPS=" << ips
                  << " Power=" << power
                  << " IPS/Watt=" << ips/power << "\n";

    	if (true) {
    		std::stringstream filename;
    		//	filename << (taskConf.begin())->first << "_" /*inputData.samples*/ << archToString(_core_list[0].info->arch) << ".csv";
    		filename << (taskConf.begin())->first << "_" /*inputData.samples*/ << "MIMO" << ".csv";
    		std::stringstream os;
    		for (auto task : _current_task_list) {
        		os << freqToValMHz_i(vitamins_dvfs_get_freq(task->_curr_mapping_)) << ";";
				os << task->_curr_mapping_->info->arch << ";";
				os << system_data[task->_curr_mapping_->info->position].ips() << ";";
    		}
    		os << power << "\n";
    		std::ofstream fileout(filename.str(),std::ofstream::app);
    		fileout.write(os.str().c_str(),os.str().size());
    		fileout.close();
    	}
    }
    if(print_total){
        std::cout << "Summary for whole execution:\n";
        double ips = 0;
        double power = 0;
        double energy = 0;
        double ipsActv = 0;
        double powerActv = 0;
        for(unsigned int i = 0; i < _core_list.size(); ++i){
            std::cout << "Core[" << i << "] type "
                    <<  archToString(_core_list[i].info->arch)
                    << " freq. " << freqToString(vitamins_dvfs_get_freq(&(_core_list[i]))) << "\n";
            std::cout << "\tIPS=" << system_data[i].total_ips_active() << "/" << system_data[i].total_ips();
            std::cout << " Power=" << system_data[i].total_power_active() << "/"<<system_data[i].total_power();
            std::cout << " IPS/Watt=" << system_data[i].total_ips_active()/system_data[i].total_power_active()
                                      << "/"
                                      << system_data[i].total_ips()/system_data[i].total_power();
            std::cout << " Runtime(s):"
                      << " active=" << system_data[i].total_time_active()
                      << " idle=" << system_data[i].total_time_idle()
                      << " off="  << system_data[i].total_time_off()
                      << " total=" <<  system_data[i].total_time_active() + system_data[i].total_time_idle() + system_data[i].total_time_off() <<  "\n";
            ips += system_data[i].total_ips();
            power += system_data[i].total_power();
            ipsActv += system_data[i].total_ips_active();
            powerActv += system_data[i].total_power_active();
            energy += system_data[i].total_energy();
            //system_data[i].dump();
        }
        std::cout << "System IPS=" << ipsActv << "/" << ips
                  << " Power=" << powerActv << "/" << power
                  << " IPS/Watt=" << ipsActv/powerActv << "/" << ips/power
                  << " Energy=" << energy << "\n";
    }
}

simulation_t::system_average_t
simulation_t::get_system_average()
{
    system_average_t avg = {0,0,0,0,0,0,0,0,0};

    for(auto core : system_data){
        avg.total_ips += core.total_ips();
        avg.total_power += core.total_power();
        avg.total_energy += core.total_energy();
    }
    avg.total_time = sim_state.currentTime;
    avg.epochs = sim_state.major_epoch_count;

    return avg;
}

double
simulation_t::core_data_t::time_active()
{
    double auxTime = 0;
    for(auto val : _active_cycles){
        if(val.second == 0) continue;
        auxTime += val.second*(1/(freqToValMHz_d(val.first)*1000000));
    }
    return auxTime;
}

double
simulation_t::core_data_t::time_idle()
{
    double auxTime = 0;
    for(auto val : _idle_cycles){
        if(val.second == 0) continue;
        auxTime += val.second*(1/(freqToValMHz_d(val.first)*1000000));
    }
    return auxTime;
}
double
simulation_t::core_data_t::total_time_active()
{
    double auxTime = 0;
    for(auto val : _acc_active_cycles){
        if(val.second == 0) continue;
        auxTime += val.second*(1/(freqToValMHz_d(val.first)*1000000));
    }
    return auxTime;
}

double
simulation_t::core_data_t::total_time_idle()
{
    double auxTime = 0;
    for(auto val : _acc_idle_cycles){
        if(val.second == 0) continue;
        auxTime += val.second*(1/(freqToValMHz_d(val.first)*1000000));
    }
    return auxTime;
}

double
simulation_t::core_data_t::avg_freqMHz()
{
    double auxFreq = 0;
    double auxTime = 0;
    for(auto val : _active_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
        auxFreq += freqToValMHz_d(val.first)*time;
    }
    for(auto val : _idle_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
        auxFreq += freqToValMHz_d(val.first)*time;
    }
    if(auxTime == 0) return 0;
    return auxFreq / auxTime;
}

double
simulation_t::core_data_t::total_avg_freqMHz()
{
    double auxFreq = 0;
    double auxTime = 0;
    for(auto val : _acc_active_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
        auxFreq += freqToValMHz_d(val.first)*time;
    }
    for(auto val : _acc_idle_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
        auxFreq += freqToValMHz_d(val.first)*time;
    }
    return auxFreq / auxTime;
}

core_freq_t
simulation_t::core_data_t::total_avg_freq_discrete()
{
    std::map<core_freq_t,double> time;
    double largest_time = 0;
    core_freq_t largest_freq = SIZE_COREFREQ;
    for(auto val : _acc_active_cycles){
        time[val.first] = val.second*(1/freqToValMHz_d(val.first));
    }
    for(auto val : _acc_idle_cycles){
        time[val.first] += val.second*(1/freqToValMHz_d(val.first));
        if(time[val.first] > largest_time){
            largest_time = time[val.first];
            largest_freq = val.first;
        }
    }
    assert(largest_freq != SIZE_COREFREQ);
    return largest_freq;
}

double
simulation_t::core_data_t::total_avg_voltage(core_arch_t arch)
{
    double auxVoltage = 0;
    double auxTime = 0;
    for(auto val : _acc_active_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
        auxVoltage += vitamins_freq_to_mVolt_map(arch,val.first)*time;
    }
    for(auto val : _acc_idle_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
        auxVoltage += vitamins_freq_to_mVolt_map(arch,val.first)*time;
    }
    return (auxVoltage / auxTime)/1000;
}

double
simulation_t::core_data_t::total_ips_active()
{
	double auxTime = 0;
    for(auto val : _acc_active_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
    }
    return (auxTime>0) ? (_acc_instr / auxTime) : 0;
}

double
simulation_t::core_data_t::total_ips()
{
	double auxTime = 0;
    for(auto val : _acc_active_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
    }
    for(auto val : _acc_idle_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
    }
    auxTime += _acc_power_gated_time*1000000;//ips is 0 when power gated, so we just add up to time
    return (auxTime>0) ? (_acc_instr / auxTime) : 0;
}

double
simulation_t::core_data_t::total_instructions()
{
    return _acc_instr;
}

double
simulation_t::core_data_t::instructions()
{
    return _instr;
}

double
simulation_t::core_data_t::ips()
{
	double auxTime = 0;
    for(auto val : _active_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
    }
    for(auto val : _idle_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
    }
    auxTime += _power_gated_time*1000000;//ips is 0 when power gated, so we just add up to time
    return (auxTime>0) ? (_instr / auxTime) : 0;
}


double
simulation_t::core_data_t::total_power()
{
    double auxPower = 0;
    double auxTime = 0;
    for(auto val : _acc_active_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
        double power = (_acc_power_active_times_cycles[val.first] / _acc_active_cycles[val.first]);
        auxPower += power*time;
    }
    for(auto val : _acc_idle_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
        double power = (_acc_power_idle_times_cycles[val.first] / _acc_idle_cycles[val.first]);
        auxPower += power*time;
    }
    auxTime += _acc_power_gated_time*1000000;//power is 0 when power gated, so we just add up to time

    return (auxTime>0) ? (auxPower / auxTime) : 0;
}

double
simulation_t::core_data_t::power()
{
    double auxPower = 0;
    double auxTime = 0;
    for(auto val : _active_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
        double power = (_power_active_times_cycles[val.first] / _active_cycles[val.first]);
        auxPower += power*time;
    }
    for(auto val : _idle_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
        double power = (_power_idle_times_cycles[val.first] / _idle_cycles[val.first]);
        auxPower += power*time;
    }
    auxTime += _power_gated_time*1000000;//power is 0 when power gated, so we just add up to time

    return (auxTime>0) ? (auxPower / auxTime) : 0;
}

double
simulation_t::core_data_t::total_power_idle()
{
    double auxPower = 0;
    double auxTime = 0;
    for(auto val : _acc_idle_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
        double power = (_acc_power_idle_times_cycles[val.first] / _acc_idle_cycles[val.first]);
        auxPower += power*time;
    }
    return (auxTime>0) ? (auxPower / auxTime) : 0;
}

double
simulation_t::core_data_t::total_power_active()
{
    double auxPower = 0;
    double auxTime = 0;
    for(auto val : _acc_active_cycles){
        double time = val.second*(1/freqToValMHz_d(val.first));
        if(time<=0) continue;
        auxTime += time;
        double power = (_acc_power_active_times_cycles[val.first] / _acc_active_cycles[val.first]);
        auxPower += power*time;
    }
    return (auxTime>0) ? (auxPower / auxTime) : 0;
}

void
simulation_t::core_data_t::dump()
{
    std::cout << "core " << core->info->position << " dump\n";
    for(auto val : _total_cycles){
        std::cout << "\t_instr="<<_instr<<"\n";
        std::cout << "\t_idle_cycles["<<freqToString(val.first)<<"]="<<_idle_cycles[val.first]<<"\n";
        std::cout << "\t_active_cycles["<<freqToString(val.first)<<"]="<<_active_cycles[val.first]<<"\n";
        std::cout << "\t_total_cycles["<<freqToString(val.first)<<"]="<<_total_cycles[val.first]<<"\n";
        std::cout << "\t_power_active_times_cycles["<<freqToString(val.first)<<"]="<<_power_active_times_cycles[val.first]<<"\n";
        std::cout << "\t_acc_instr="<<_acc_instr<<"\n";
        std::cout << "\t_acc_idle_cycles["<<freqToString(val.first)<<"]="<<_acc_idle_cycles[val.first]<<"\n";
        std::cout << "\t_acc_active_cycles["<<freqToString(val.first)<<"]="<<_acc_active_cycles[val.first]<<"\n";
        std::cout << "\t_acc_power_active_times_cycles["<<freqToString(val.first)<<"]="<<_acc_power_active_times_cycles[val.first]<<"\n";
        std::cout << "\t_acc_power_idle_times_cycles["<<freqToString(val.first)<<"]="<<_acc_power_idle_times_cycles[val.first]<<"\n";
    }
}

simulation_t* simulation_t::clone()
{
    simulation_t *other = new simulation_t(inputData,taskConf,coreConf,_using_oracle,VB_NONE);

    assert(_initial_task_list.size() == other->_initial_task_list.size());
    for(unsigned i = 0; i < _initial_task_list.size(); ++i){
        assert(sim_state.benchmark[_initial_task_list[i]]
               ==
               other->sim_state.benchmark[other->_initial_task_list[i]]);

        *(other->_initial_task_list[i]) = *(_initial_task_list[i]);
        clear_object_default(other->_initial_task_list[i]);

        other->sim_state.currInstr[other->_initial_task_list[i]] = sim_state.currInstr[_initial_task_list[i]];
        other->sim_state.finishTime[other->_initial_task_list[i]] = sim_state.finishTime[_initial_task_list[i]];

    }
    other->sim_state.currentTime = sim_state.currentTime;
    other->sim_state.major_epoch_count = sim_state.major_epoch_count;
    other->sim_state.minor_epoch_count = sim_state.minor_epoch_count;

    assert(_core_list.size() == other->_core_list.size());
    for(unsigned i = 0; i < _core_list.size(); ++i){
        assert(other->_core_list[i].info->arch == _core_list[i].info->arch);
        assert(other->_core_list[i].info->position == _core_list[i].info->position);

        other->_core_list[i].load_tracking.cfs = _core_list[i].load_tracking.cfs;
        other->_core_list[i].load_tracking.defaut = _core_list[i].load_tracking.defaut;
        other->_core_list[i].info->freq->this_domain->freq = _core_list[i].info->freq->this_domain->freq;
        clear_object_default(&(other->_core_list[i]));
    }

    assert(system_data.size() == other->system_data.size());
    for (unsigned i =0; i < system_data.size(); ++i){
        other->system_data[i] = system_data[i];
    }

    return other;
}
