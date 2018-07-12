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

#include <cmath>

#include "trace_simulator.h"
#include <runtime/common/option_parser.h>
#include <runtime/common/strings.h>

void TraceSimulator::_init_sys_info()
{
    //first create all cores
    for(auto coreType : _coreConf){
        for (int i = 0; i < coreType.second.cluster_cnt(); ++i){
            for (int j = 0; j < coreType.second.cores_per_cluster(); ++j){
                _core_info_list.push_back(core_info_t());
                _freq_domain_info_list.push_back(freq_domain_info_t());
                _power_domain_info_list.push_back(power_domain_info_t());
            }
        }
    }

    //creates the domains
    core_info_t *cores_info = _core_info_list.data();
    int core_idx = 0;
    int freq_domain_list_size = 0;
    int power_domain_list_size = 0;
    for(auto coreType : _coreConf){
        for (int i = 0; i < coreType.second.cluster_cnt(); ++i){
            freq_domain_info_t* freq_info = &(_freq_domain_info_list.data()[freq_domain_list_size]);
            freq_domain_info_init(freq_info,freq_domain_list_size,nullptr,nullptr);
            ++freq_domain_list_size;
            for (int j = 0; j < coreType.second.cores_per_cluster(); ++j){
                core_info_t *core_info = &(cores_info[core_idx]);

                power_domain_info_t *power_info = &(_power_domain_info_list.data()[power_domain_list_size]);
                power_domain_info_init(power_info,power_domain_list_size,freq_info);
                ++power_domain_list_size;

                core_info_init(core_info,coreType.first,core_idx,freq_info,power_info);
                ++core_idx;
            }
        }
    }

    _sys_info.core_list = cores_info;
    _sys_info.core_list_size = core_idx;

    _sys_info.freq_domain_list = _freq_domain_info_list.data();
    _sys_info.freq_domain_list_size = freq_domain_list_size;

    _sys_info.power_domain_list = _power_domain_info_list.data();
    _sys_info.power_domain_list_size = power_domain_list_size;
}

void TraceSimulator::_init_task_info()
{
    assert_true(_traces->samples.size() > 0);

    bool err = false;
    for (auto bench : _taskConf){
        if(_traces->samples.find(bench.first) == _traces->samples.end()){
            pinfo("ERR: Can't find benchmark %s\n",bench.first.c_str());
            err = true;
        }
    }

    //Initialize freq map and ranges
    for(int i = 0; i < _sys_info.freq_domain_list_size; ++i){
        const freq_domain_info_t &fd = _sys_info.freq_domain_list[i];

        // Requirement to work with traces (all cores in the fd have same arch)
        assert_true(fd.__vitaminslist_head_cores != nullptr);
        core_arch_t fdArch = fd.__vitaminslist_head_cores->arch;
        for(int j = 0; j < _sys_info.core_list_size; ++j)
            if(_sys_info.core_list[j].freq->domain_id == _sys_info.freq_domain_list[i].domain_id)
                assert_true(_sys_info.core_list[j].arch == fdArch);

        //Initialize freq map and ranges
        _trace_freqs.emplace_back();
        if(_trace_freqs_per_arch.find(fdArch) == _trace_freqs_per_arch.end())
            _trace_freqs_per_arch[fdArch] = std::set<int>();
        for (auto bench : _traces->samples){
            auto iter = bench.second.find(archToString(fdArch));
            if(iter != bench.second.end()){
                for(auto freq : iter->second){
                    _trace_freqs[i].insert(freq.first);
                    _trace_freqs_per_arch[fdArch].insert(freq.first);
                }
            }
        }
        assert_true(_trace_freqs[i].size() >= 1);
        _trace_freqs_ranges.emplace_back();
        _trace_freqs_ranges[i].min = *(_trace_freqs[i].begin());
        _trace_freqs_ranges[i].max = *(_trace_freqs[i].rbegin());
        if(_trace_freqs[i].size() == 1)
            _trace_freqs_ranges[i].steps = 0;
        else
            _trace_freqs_ranges[i].steps = (_trace_freqs_ranges[i].max - _trace_freqs_ranges[i].min) / (_trace_freqs[i].size() - 1);

        _fd_execInfo.emplace_back(&fd,_trace_freqs_ranges[i].min);
    }

    for (auto bench : _traces->samples){
        for (auto coreArch : _coreConf) {
            auto iter = bench.second.find(archToString(coreArch.first));
            if(iter == bench.second.end()){
                pinfo("ERR: No data for benchmark %s at arch %s\n",bench.first.c_str(),archToString(coreArch.first));
                err = true;
            }
            else{
                for(auto freq : _trace_freqs_per_arch[coreArch.first]){
                    if(iter->second.find(freq) == iter->second.end()){
                        pinfo("ERR: No data for benchmark %s at arch %s freq %d\n",bench.first.c_str(),archToString(coreArch.first),freq);
                        err = true;
                    }
                }
            }
        }
    }
    if(err) arm_throw(TraceSimulator,"Traces missing information");
}

void TraceSimulator::_init_task_perf_data()
{
    // Initialize task perf_data
    int taskID = 0;
    int cpu = 0;
    for(auto taskName : _taskConf){

        if(taskID >= MAX_CREATED_TASKS)
            arm_throw(TraceSimulator,"Number of tasks must be <= %d",MAX_CREATED_TASKS);

        assert_true(_traces->samples.find(taskName.first)!=_traces->samples.end());

        for (int i = 0; i < taskName.second; ++i){
                tracked_task_data_t &task = _perf_data.created_tasks[taskID];

                task.this_task_pid = taskID;
                task.task_idx = taskID;
                for(unsigned k = 0; k < TASK_NAME_SIZE; ++k){
                    if(k < taskName.first.size())
                        task.this_task_name[k] = taskName.first.at(k);
                    else
                        task.this_task_name[k] = '\0';
                }
                task.this_task_name[TASK_NAME_SIZE-1] = '\0';


                perf_data_reset_task(&_perf_data,taskID);
                _task_execInfo.emplace_back(taskName.first,&task,cpu);
                cpu = (cpu + 1) % _sys_info.core_list_size;

                // TODO beats support ??
                task.num_beat_domains = 0;
                task.parent_has_beats = false;

                task.task_finished = false;
                task.tsk_model = nullptr;

                taskID += 1;
                _perf_data.created_tasks_cnt += 1;
        }
    }
}

void TraceSimulator::_seachAndSetTrace(task_exec_info &taskInfo)
{
    //searches trace that contains the current instruction
    assert_true(!taskInfo.finished);

    auto tskIter = _traces->samples.find(taskInfo.name);
    assert_true(tskIter != _traces->samples.end());

    assert_true(taskInfo.currCore < _sys_info.core_list_size);
    core_info_t &core = _sys_info.core_list[taskInfo.currCore];

    auto archIter = tskIter->second.find(archToString(core.arch));
    assert_true(archIter != tskIter->second.end());

    assert_true(core.freq->domain_id < (int)_fd_execInfo.size());
    assert_true(core.freq->domain_id < (int)_trace_freqs.size());
    assert_true(_fd_execInfo.size() ==  _trace_freqs.size());
    int freq = _fd_execInfo[core.freq->domain_id].currFreq;
    assert_true(_trace_freqs[core.freq->domain_id].find(freq) != _trace_freqs[core.freq->domain_id].end());

    auto freqIter = archIter->second.find(freq);
    assert_true(freqIter != archIter->second.end());

    TraceSampleIter beginIter;
    for(beginIter = freqIter->second->begin(); beginIter != freqIter->second->end(); ++beginIter)
        if(beginIter->curr_instr > taskInfo.currIntr)
            break;
    assert_true(beginIter != freqIter->second->end());//task cannot be finished

    int64_t beginIterFirstInstr = (beginIter == freqIter->second->begin()) ? 0 : (--beginIter)->curr_instr + 1;
    assert_true(beginIterFirstInstr < beginIter->curr_instr);
    assert_true(beginIterFirstInstr <= taskInfo.currIntr);

    double beginPortion = (double)(taskInfo.currIntr - beginIterFirstInstr)/(double)(beginIter->curr_instr-beginIterFirstInstr);
    assert_true(beginPortion <= 1);
    assert_true(beginPortion >= 0);

    pinfo("SIM %5d: _seachAndSetSample(tsk=%d cpu=%d arch=%s freq=%d) sample=%d instrRange=[%d,%d] portion=%f \n",(int)_currentSimTimeMS,
            taskInfo.task->task_idx, taskInfo.currCore, archToString(core.arch), freq,
            (int)(beginIter - freqIter->second->begin()), (int)beginIterFirstInstr, (int)beginIter->curr_instr, beginPortion );

    taskInfo.currTraceInfo = task_trace_info{beginIter,beginIter,freqIter->second->end(),
        beginPortion,beginPortion,true};
}

void TraceSimulator::_commitTraces()
{
    for(auto &task : _task_execInfo){
        if(task.currTraceInfo.valid){
            pinfo("SIM %5d: _commitTraces() committing task = %d \n",(int)_currentSimTimeMS,task.task->task_idx);
            //TODO fill up the _acc* for all sensing windows
            //TODO increment the task currIntr
            //Would run LinSched here to see exactly by how much we increase
            //the task currIntr how much of the traces to commit

            //forces us to look-up new traces next time we advance time
            //since the current ones are already committed
            task.currTraceInfo.valid = false;
        }
        else{
            pinfo("SIM %5d: _commitTraces() task = %d : nothing to commit \n",(int)_currentSimTimeMS,task.task->task_idx);
        }
    }
}

// No locks necessary in offline
static inline void perf_data_commit_acc_lock(int, int,unsigned long *)
{  }
static inline void perf_data_commit_acc_unlock(int, int,unsigned long *)
{  }

void TraceSimulator::_commitWindow(int wid)
{
    pinfo("SIM %5d: _commitWindow() committing wid = %d \n",(int)_currentSimTimeMS,wid);

    // impossible in offline sim
    assert_false(_perf_data.sensing_windows[wid].___reading);

    _perf_data.sensing_windows[wid].___updating = true;


    perf_data_commit_cpu_window(&_sys_info,
            &_perf_data,wid,_currentSimTimeMS,
            &perf_data_commit_acc_lock,
            &perf_data_commit_acc_unlock);


    perf_data_commit_tasks_window(&_sys_info,
            &_perf_data,wid,_currentSimTimeMS,
            &perf_data_commit_acc_lock,
            &perf_data_commit_acc_unlock);

    _perf_data.sensing_windows[wid].num_of_samples += 1;

    _perf_data.sensing_windows[wid].___updating = false;
}

void TraceSimulator::_currTraceInfoConsistencyCheck(task_exec_info &taskInfo)
{

    assert_true(taskInfo.currCore < _sys_info.core_list_size);
    core_info_t *core = &(_sys_info.core_list[taskInfo.currCore]);

    task_trace_info &ti = taskInfo.currTraceInfo;
    assert_true(ti.valid);

    assert_true(ti.beginSample->curr_instr >= taskInfo.currIntr);
    assert_true(streq(ti.beginSample->_data.arch,archToString(core->arch)));
    assert_true(ti.beginSample->_data.freq == _fd_execInfo[core->freq->domain_id].currFreq);
    assert_true(streq(ti.beginSample->_data.task,taskInfo.name));

    assert_true(ti.endSample->curr_instr >= taskInfo.currIntr);
    assert_true(streq(ti.endSample->_data.arch,archToString(core->arch)));
    assert_true(ti.endSample->_data.freq == _fd_execInfo[core->freq->domain_id].currFreq);
    assert_true(streq(ti.endSample->_data.task,taskInfo.name));
}

bool TraceSimulator::_advanceTimeForTask(int timeMS, task_exec_info &taskInfo)
{
    pinfo("SIM %5d: _advanceTimeForTask() task = %d\n",(int)_currentSimTimeMS,taskInfo.task->task_idx);

    if(!taskInfo.currTraceInfo.valid)
        _seachAndSetTrace(taskInfo);

    _currTraceInfoConsistencyCheck(taskInfo);

    task_trace_info &ti = taskInfo.currTraceInfo;

    int timeLeftToAdvance = timeMS;
    while(timeLeftToAdvance > 0){
        double sampleTotalTime = ti.endSample->sense<SEN_TOTALTIME_S>() * 1000;

        //time available in the current sample
        int timeLeft = std::round(sampleTotalTime * (1 - ti.endSamplePortion));

        if(timeLeftToAdvance >= timeLeft){
            //Update and go to next sample
            ti.endSamplePortion = 0;
            ++ti.endSample;
            timeLeftToAdvance -= timeLeft;
        }
        else{
            //Advance within sample
            ti.endSamplePortion += timeLeftToAdvance / sampleTotalTime;
            assert_true(ti.endSamplePortion <= 1.0);
            timeLeftToAdvance = 0;
        }
        if(ti.endSample == ti.theend){
            pinfo("SIM %5d: _advanceTimeForTask() task = %d finished !\n",(int)_currentSimTimeMS,taskInfo.task->task_idx);
            return true;
        }
    }
    pinfo("SIM %5d: _advanceTimeForTask() task = %d sample time = %f\n",(int)_currentSimTimeMS,taskInfo.task->task_idx,
            ti.endSample->curr_time + (ti.endSample->sense<SEN_TOTALTIME_S>() * ti.endSamplePortion));
    return false;
}

void TraceSimulator::_advanceTime(int timeMS)
{
    for(auto &task : _task_execInfo){
        pinfo("SIM %5d: _advanceTime() task = %d\n",(int)_currentSimTimeMS,task.task->task_idx);

        if(task.finished) continue;

        task.finished = _advanceTimeForTask(timeMS,task);
    }
}


void TraceSimulator::_init_opts()
{
    _pm_opts["outdir"] = "outdir";
}

void TraceSimulator::tracePerfCounter(perfcnt_t perfcnt)
{
    //If traces are provided, the perfcnt must be in the traces
    if(_traces){
        if(!_traces->havePerfcnt(perfcnt))
            arm_throw(TraceSimulatorException,"Perfcnt %s not available",perfcnt_str(perfcnt));
    }

    if(!perf_data_map_perfcnt(&_perf_data,perfcnt))
        arm_throw(TraceSimulatorException,"Could not enable perfcnt %s",perfcnt_str(perfcnt));
}

void TraceSimulator::tracePerfCounterResetAll()
{
    perf_data_reset_mapped_perfcnt(&_perf_data);
}

void TraceSimulator::_executeSleepers()
{
    //pinfo("**TraceSimulator::_executeSleepers - waiting for lock \n");
    _sleepersMtx.lock();
    for(auto sleeper : _sleepers){
        if(sleeper->when == _currentSimTimeMS){
            //pinfo("**TraceSimulator::_executeSleepers - sleeper->waiting.tryWait()\n");
            bool passed = sleeper->waiting.tryWait();
            assert_true(passed);

            //pinfo("**TraceSimulator::_executeSleepers - sleeper->sem.notify()\n");
            sleeper->sem.notify();

            //pinfo("**TraceSimulator::_executeSleepers - sleeper->waiting.wait()\n");
            _sleepersMtx.unlock();
            sleeper->waiting.wait();
            _sleepersMtx.lock();
            //pinfo("**TraceSimulator::_executeSleepers - sleeper->waiting.notify()\n");
            sleeper->waiting.notify();

        }
    }
    _sleepersMtx.unlock();
}

void TraceSimulator::_releaseSleepers()
{
    std::lock_guard<std::mutex> l(_sleepersMtx);
    for(auto sleeper : _sleepers)
        sleeper->sem.notify();
}

void TraceSimulator::_executeWindows()
{
    for(auto window = _sensingWindows.begin(); window != _sensingWindows.end(); ++window)
    {
        if(window->second.when == _currentSimTimeMS){
            window->second.when += window->first;

            _sensingWindowsWaitDone.wait();

            _sensingWindowID = window->second.id;
            _commitTraces();
            _commitWindow(_sensingWindowID);
            _sensingWindowsWait.notify();

            _sensingWindowsWaitDone.wait();
            _sensingWindowsWaitDone.notify();
        }
    }
}

void TraceSimulator::_releaseWindows()
{
    _sensingWindowID = WINDOW_EXIT;
    _sensingWindowsWait.notify();
}


void TraceSimulator::_simulate(uint64_t forMS)
{
    while(_currentSimTimeMS <= forMS){
        pinfo("SIM %5d: _simulate\n",(int)_currentSimTimeMS);

        // any periodic sensor waiting
       _executeSleepers();

       // TODO implement window pre-sensing
       //    - advance trace time / switch traces here
       //    - commit window data at _executeWindows

       _executeWindows();

        // TODO implement actuations

        _currentSimTimeMS += MINIMUM_WINDOW_LENGTH_MS;
        _perf_data.num_of_minimum_periods += 1;
        _advanceTime(MINIMUM_WINDOW_LENGTH_MS);
    }
    pinfo("SIM %5d: _simulate - exiting\n",(int)_currentSimTimeMS);
}

//forces us to look-up new traces next time

