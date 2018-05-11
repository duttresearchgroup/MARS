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

#ifndef OFFLINE_TRACE_SIMULATOR_H_
#define OFFLINE_TRACE_SIMULATOR_H_

#include <string>
#include <map>
#include <mutex>
#include <list>
#include <unordered_map>

#include <unistd.h>

#include <base/base.h>
#include <runtime/common/traceparser.h>
#include <runtime/common/semaphore.h>
#include <runtime/framework/policy.h>


#include "sensing_module.h"

class TraceSimulator
{

    struct EnumClassHash
    {
        template <typename T>
        std::size_t operator()(T t) const
        {
            return static_cast<std::size_t>(t);
        }
    };


  public:

    // Simulation config

    typedef std::map<std::string,int> task_sim_conf_t;//number of instances of each benchmark
    static inline
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
            assert_true((numCores % numClusters) == 0);
        };
        core_sim_cluster() :numClusters(0), numCores(0) {}

        inline int core_cnt() const { return numCores; }
        inline int cluster_cnt() const { return numClusters; }
        inline int cores_per_cluster() const { return numCores/numClusters; }
    };

    typedef std::map<core_arch_t,core_sim_cluster> core_sim_conf_t;//number of instances of each core type

    typedef TraceParser::Traces Traces;

  private:

    uint64_t _currentSimTimeMS;
    volatile bool _simulating;

    PolicyManager *_pm;
    OfflineSensingModule *_sm;
    const core_sim_conf_t _coreConf;
    const Traces *_traces;

    volatile int _sensingWindowID;
    Semaphore _sensingWindowsWait;//notify that a window is ready
    Semaphore _sensingWindowsWaitDone;//notify that a window is ready
    struct SensingWindow {
        int id;
        uint64_t when;
    };
    //map period to window; smaller periods go first when windows coincide
    std::map<int,SensingWindow> _sensingWindows;

    void _executeWindows();
    void _releaseWindows();


    struct Sleeper {
        Semaphore sem;
        Semaphore waiting;//1 if the thread is waiting
        uint64_t when;
        Sleeper() :sem(0),waiting(0),when(0) {}
    };
    std::unordered_set<Sleeper*> _sleepers;
    std::mutex _sleepersMtx;

    void _executeSleepers();
    void _releaseSleepers();

    const task_sim_conf_t _taskConf;

    std::vector<core_info_t> _core_info_list;
    std::vector<freq_domain_info_t> _freq_domain_info_list;
    std::vector<power_domain_info_t> _power_domain_info_list;
    sys_info_t _sys_info;

    perf_data_t _perf_data;

    std::unordered_map<std::string,std::string> _pm_opts;

    // Frequencies available per freq domain (set only when traces are given)
    std::vector<std::set<int>> _trace_freqs;
    std::vector<ActuationTypeInfo<ACT_FREQ_MHZ>::Ranges> _trace_freqs_ranges;
    std::unordered_map<core_arch_t,std::set<int>,EnumClassHash> _trace_freqs_per_arch;

    typedef std::vector<TraceParser::Sample>::iterator TraceSampleIter;
    struct task_trace_info {
        TraceSampleIter beginSample;
        TraceSampleIter endSample;
        TraceSampleIter theend;//ref to vec->end()
        // Traces not always align so this "how much" of the first and last
        // sample we consider
        double beginSamplePortion;//where should we start, i.e. 0 = do the whole sample
        double endSamplePortion;//where should we stop, i.e. 1 = do the whole sample

        bool valid; //are the iterators valid ??
    };

    struct fd_exec_info {
        const freq_domain_info_t *fd;
        int currFreq;

        fd_exec_info(const freq_domain_info_t *_fd, int freq)
            :fd(_fd), currFreq(freq)
        {}
    };

    struct task_exec_info {
        tracked_task_data_t *task;
        std::string name;
        int currCore;
        int nextCore;
        int64_t currIntr;
        bool finished;
        task_trace_info currTraceInfo;

        task_exec_info(const std::string &_name, tracked_task_data_t *_task, int _core)
            :task(_task), name(_name), currCore(_core), nextCore(_core),
             currIntr(0), finished(false)
        {}
    };
    void _currTraceInfoConsistencyCheck(task_exec_info &taskInfo);

    std::vector<fd_exec_info> _fd_execInfo;
    std::vector<task_exec_info> _task_execInfo;

    void _seachAndSetTrace(task_exec_info &taskInfo);

    bool _advanceTimeForTask(int timeMS, task_exec_info &taskInfo);

    void _commitTraces();

    void _commitWindow(int wid);

    void _advanceTime(int timeMS);

    void _init_sys_info();

    void _init_task_info();

    void _init_task_perf_data();

    void _init_opts();

    void _init_common()
    {
        _init_sys_info();
        perf_data_init(&_perf_data,&_sys_info);
        perf_data_reset_mapped_perfcnt(&_perf_data);
        _init_opts();
    }

    void _simulate(uint64_t forMS);


  public:

    TraceSimulator(const core_sim_conf_t &coreConf)
      :_currentSimTimeMS(0), _simulating(false), _pm(nullptr), _sm(nullptr),
       _coreConf(coreConf), _traces(nullptr)
    {
        _init_common();
    }

    TraceSimulator(const core_sim_conf_t &coreConf, const task_sim_conf_t &taskConf, const Traces& traces)
      :_currentSimTimeMS(0), _simulating(false), _pm(nullptr), _sm(nullptr),
       _coreConf(coreConf), _traces(&traces),
       _taskConf(taskConf)
    {
        _init_common();
        _init_task_info();
    }

    ~TraceSimulator()
    {
        if(_pm) delete _pm;
        //_sm is deleted by SensingWindowManager when _pm is deleted
        pinfo("SIM %5d: clean up complete \n",(int)_currentSimTimeMS);
    }


    void addOption(const std::string &opt, const std::string &val)
    {
        _pm_opts[opt] = val;
    }

    uint64_t simTimeMS() { return _currentSimTimeMS; }

    sys_info_t *info() { return &_sys_info;}

    perf_data_t *perf_data() { return &_perf_data;}

    void tracePerfCounter(perfcnt_t perfcnt);
    void tracePerfCounterResetAll();

    // Create the policy provided by the template parameter and
    // simulates for the amount of time provided or until all tasks finish.
    // If forMS==0, simulate until all tasks finish.
    // If no trace is given, just simulate for the amount of time provided.
    // Consecutive call to simulate are currently not supported
    // Options
    template<typename PolicyManagerT>
    void simulate(uint64_t forMS = 0)
    {
        try {
            OptionParser::init(_pm_opts);
            pinfo("SIM %5d: simulation started. Options \n",(int)_currentSimTimeMS);
            OptionParser::parser().printOpts();

            pinfo("SIM %5d: simulate creating interface \n",(int)_currentSimTimeMS);
            _sm = new OfflineSensingModule(this);
            pinfo("SIM %5d: simulate policy manager \n",(int)_currentSimTimeMS);
            _pm = new PolicyManagerT(_sm);

            _simulating = true;
            _pm->start();

            // Tasks are "created" after sensing started
            _init_task_perf_data();

            _simulate(forMS);

            _simulating = false;
            _pm->stop();
            pinfo("SIM %5d: simulate - stop complete \n",(int)_currentSimTimeMS);

        } arm_catch(ARM_CATCH_NO_EXIT);
    }

    void finishSimulationStop()
    {
        pinfo("SIM %5d: exit wrap-up\n",(int)_currentSimTimeMS);
        _releaseSleepers();
        _releaseWindows();
    }


    // For simulation purposes, we must wait until all threads that are
    // released from a sleep to block again before proceeding. So this
    // function assumes that a thread will block by calling sleepMS again,
    // therefore BLOCKING the simulation until this event happens.
    // Any thread blocked in this function is released when the simulation ends
    void sleepMS(int timeMS)
    {
        static thread_local Sleeper s;

        //pinfo("SIM %5d: TraceSimulator::sleepMS - time %d \n",(int)_currentSimTimeMS,timeMS);

        if(!_simulating){
            usleep(250000);//wait 250ms until _pm->stop() propagates to this thread
            pinfo("SIM %5d: TraceSimulator::sleepMS - exit due to sim end\n",(int)_currentSimTimeMS);
            return;
        }

        assert_true(s.sem.getCount() == 0);// Inconsistent otherwise
        // Time must be multiple o MINIMUM_WINDOW_PERIOD and greater than 0
        assert_true(timeMS > 0);
        assert_true((timeMS % MINIMUM_WINDOW_LENGTH_MS) == 0);
        s.when = _currentSimTimeMS + timeMS;

        //pinfo("**TraceSimulator::sleepMS - s.waiting.notify() \n");
        s.waiting.notify();

        //pinfo("**TraceSimulator::sleepMS - waiting for lock \n");
        {
            std::lock_guard<std::mutex> l(_sleepersMtx);
            _sleepers.insert(&s);
        }
        //pinfo("**TraceSimulator::sleepMS - s.sem.wait() \n");
        s.sem.wait();
    }

    // Assumes a single thread calls this function
    int waitForWindow()
    {
        _sensingWindowsWaitDone.notify();
        _sensingWindowsWait.wait();
        if(!_simulating)
            return WINDOW_EXIT;
        else
            return _sensingWindowID;
    }

    int registerWindow(int periodMS)
    {
        if((periodMS % MINIMUM_WINDOW_LENGTH_MS) != 0)
            return WINDOW_INVALID_PERIOD;

        if(_sensingWindows.size() == MAX_WINDOW_CNT)
            return WINDOW_MAX_NWINDOW;

        if(_sensingWindows.find(periodMS) != _sensingWindows.end())
            return WINDOW_EXISTS;

        assert_true(_currentSimTimeMS == 0);

        int id = _sensingWindows.size();
        _sensingWindows[periodMS] = {id,(uint64_t)periodMS};
        _perf_data.sensing_window_cnt = _sensingWindows.size();
        return id;
    }

    void setTaskCore(const tracked_task_data_t *task, core_info_t *core)
    {
        assert_true(task->task_idx < (int)_task_execInfo.size());
        assert_true(core != nullptr);
        _task_execInfo[task->task_idx].currCore = core->position;
        if(_task_execInfo[task->task_idx].finished)
            pinfo("SIM %5d: setting core for finished task %d!\n",(int)_currentSimTimeMS,task->task_idx);

    }
    core_info_t * getTaskCore(const tracked_task_data_t *task)
    {
        assert_true(task->task_idx < (int)_task_execInfo.size());
        if(_task_execInfo[task->task_idx].finished)
            pinfo("SIM %5d: getting core for finished task %d!\n",(int)_currentSimTimeMS,task->task_idx);

        assert_true(_task_execInfo[task->task_idx].currCore < _sys_info.core_list_size);
        return &(_sys_info.core_list[_task_execInfo[task->task_idx].currCore]);
    }

    void setDomainFreq(const freq_domain_info_t *fd, int freqMHZ)
    {
        assert_true(fd != nullptr);
        assert_true(fd->domain_id < _sys_info.freq_domain_list_size);
        assert_true(fd->domain_id < (int)_fd_execInfo.size());
        assert_true(fd->domain_id < (int)_trace_freqs_ranges.size());
        assert_true(fd->domain_id < (int)_trace_freqs.size());

        assert_true(freqMHZ >= _trace_freqs_ranges[fd->domain_id].min);
        assert_true(freqMHZ <= _trace_freqs_ranges[fd->domain_id].max);

        if(_trace_freqs[fd->domain_id].find(freqMHZ) == _trace_freqs[fd->domain_id].end()){
            int nearestFreq = _search_nearest(_trace_freqs[fd->domain_id],freqMHZ);
            pinfo("SIM %5d: fd %d freq %d not available. Setting freq to %d!\n",(int)_currentSimTimeMS,fd->domain_id,freqMHZ,nearestFreq);
            _fd_execInfo[fd->domain_id].currFreq = nearestFreq;
        }
        else
            _fd_execInfo[fd->domain_id].currFreq = freqMHZ;
    }
    int getDomainFreq(const freq_domain_info_t *fd)
    {
        assert_true(fd != nullptr);
        assert_true(fd->domain_id < _sys_info.freq_domain_list_size);
        assert_true(fd->domain_id < (int)_fd_execInfo.size());
        return _fd_execInfo[fd->domain_id].currFreq;
    }
    ActuationTypeInfo<ACT_FREQ_MHZ>::Ranges& getDomainRanges(const freq_domain_info_t *fd)
    {
        assert_true(fd != nullptr);
        assert_true(fd->domain_id < _sys_info.freq_domain_list_size);
        assert_true(fd->domain_id < (int)_trace_freqs_ranges.size());
        return _trace_freqs_ranges[fd->domain_id];
    }

  private:
    inline int _search_nearest(const std::set<int> &set, int search_val){
        if(search_val <= *(set.begin())) return *(set.begin());
        if(search_val >= *(set.rbegin())) return *(set.rbegin());
        auto upper = set.lower_bound(search_val);
        auto lower = upper;--lower;
        if((*upper-search_val) < (search_val-*lower))
            return *upper;
        else
            return *lower;
    }

};


#endif /* OFFLINE_TRACE_SIMULATOR_H_ */
