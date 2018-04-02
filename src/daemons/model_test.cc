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

#include <unistd.h>

#include <runtime/daemon/deamonizer.h>
#include <runtime/common/reports.h>
#include <runtime/framework/actuation_interface.h>
#include "../runtime/framework/models/baseline_model.h"

class ModelTest : public PolicyManager {
protected:
    static const int WINDOW_LENGTH_MS = 200;

    virtual void setup();

    static void window_handler(int wid,PolicyManager *owner);

    struct PredData {
        int cpu;
        typename SensingTypeInfo<SEN_PERFCNT>::ValType instr;
        double busyTime;
        double totalTime;
        double power;
        int freq;
        double load;
        double ipc;
        double ipc_raw;
        double tlc_raw;
        bool valid;
    };

private:

    std::unordered_map<const tracked_task_data_t*,ExecutionTrace*> _taskExecTraces;
    std::unordered_map<const core_info_t*,ExecutionTrace*> _coreExecTraces;
    std::unordered_map<const freq_domain_info_t*,ExecutionTrace*> _fdExecTraces;
    std::unordered_map<const power_domain_info_t*,ExecutionTrace*> _pdExecTraces;

    ExecutionTrace::ExecutionTraceHandle& getHandle(const tracked_task_data_t *task, int wid)
    {
        auto iter = _taskExecTraces.find(task);
        if(iter != _taskExecTraces.end())
            return iter->second->getHandle(wid);
        else{
            ExecutionTrace *execTrace = new ExecutionTrace("trace.pid"+std::to_string(task->this_task_pid)+"."+task->this_task_name);
            _taskExecTraces[task] = execTrace;
            return execTrace->getHandle(wid);
        }
    }
    ExecutionTrace::ExecutionTraceHandle& getHandle(const core_info_t *core, int wid)
    {
        auto iter = _coreExecTraces.find(core);
        if(iter != _coreExecTraces.end())
            return iter->second->getHandle(wid);
        else{
            ExecutionTrace *execTrace = new ExecutionTrace("trace.core"+std::to_string(core->position));
            _coreExecTraces[core] = execTrace;
            return execTrace->getHandle(wid);
        }
    }
    ExecutionTrace::ExecutionTraceHandle& getHandle(const freq_domain_info_t *fd, int wid)
    {
        auto iter = _fdExecTraces.find(fd);
        if(iter != _fdExecTraces.end())
            return iter->second->getHandle(wid);
        else{
            ExecutionTrace *execTrace = new ExecutionTrace("trace.fd"+std::to_string(fd->domain_id));
            _fdExecTraces[fd] = execTrace;
            return execTrace->getHandle(wid);
        }
    }
    ExecutionTrace::ExecutionTraceHandle& getHandle(const power_domain_info_t *pd, int wid)
    {
        auto iter = _pdExecTraces.find(pd);
        if(iter != _pdExecTraces.end())
            return iter->second->getHandle(wid);
        else{
            ExecutionTrace *execTrace = new ExecutionTrace("trace.pd"+std::to_string(pd->domain_id));
            _pdExecTraces[pd] = execTrace;
            return execTrace->getHandle(wid);
        }
    }

    std::unordered_map<const void*,PredData> _predData;

    template<typename Rsc>
    PredData& getPredData(const Rsc *rsc)
    {
        auto iter = _predData.find(rsc);
        if(iter != _predData.end())
            return iter->second;
        else{
            _predData[rsc] = {0,0,0,0,0,0,0,0,0,0,0};
            return _predData[rsc];
        }
    }

public:
    ModelTest() :PolicyManager()
    {};

    ~ModelTest()
    {
        for(auto i : _taskExecTraces) delete i.second;
        for(auto i : _coreExecTraces) delete i.second;
        for(auto i : _fdExecTraces) delete i.second;
        for(auto i : _pdExecTraces) delete i.second;
    }

};

void ModelTest::setup()
{
    //pinfo("Waiting 10s for GDB to attach\n");
    //sleep(10);

    sensingModule()->enablePerTaskSensing();

    sensingModule()->tracePerfCounter(PERFCNT_INSTR_EXE);
    sensingModule()->tracePerfCounter(PERFCNT_BUSY_CY);
    sensingModule()->tracePerfCounter(PERFCNT_BRANCH_MISPRED);
    sensingModule()->tracePerfCounter(PERFCNT_L1DCACHE_MISSES);
    sensingModule()->tracePerfCounter(PERFCNT_LLCACHE_MISSES);

    windowManager()->addSensingWindowHandler(WINDOW_LENGTH_MS,this,window_handler);

    //sets all domains to the same frequency
    constexpr int freqMHz = 1000;
    for(int domain_id = 0; domain_id < info()->power_domain_list_size; ++domain_id){
        actuate<ACT_FREQ_MHZ>(&(info()->freq_domain_list[domain_id]), freqMHz);
    }

    enableReflection();
}

static inline int nextFreq(const freq_domain_info_t* fd)
{
    auto &ranges = ActuationInterface::actuationRanges<ACT_FREQ_MHZ>(fd);
    int currFreq = ActuationInterface::actuationVal<ACT_FREQ_MHZ>(fd);

    currFreq += ranges.steps*3;

    if(currFreq > ranges.max)
        currFreq = ranges.min;

    return currFreq;
}


void ModelTest::window_handler(int wid,PolicyManager *owner)
{
    ModelTest *self = dynamic_cast<ModelTest*>(owner);

    const PerformanceData &data = owner->sensedData();

    for(int domain_id = 0; domain_id < self->info()->freq_domain_list_size; ++domain_id){
        tryActuate<ACT_FREQ_MHZ>(&(self->info()->freq_domain_list[domain_id]),
                nextFreq(&(self->info()->freq_domain_list[domain_id])));
    }

    //pinfo("## Working with %d tasks\n",data.numCreatedTasks());
    for(int i = 0; i < data.numCreatedTasks(wid); ++i){
        const tracked_task_data_t *tsk = &(data.task(i));
        auto &trace = self->getHandle(tsk,wid);

        if(sense<SEN_LASTCPU>(tsk) == -1) continue;

        trace("cpu_sensed") = sense<SEN_LASTCPU>(tsk);
        trace("instr_sensed") = sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,tsk);
        trace("busyTime_sensed") = sense<SEN_BUSYTIME_S>(tsk);
        trace("totalTime_sensed") = sense<SEN_TOTALTIME_S>(tsk);
        trace("load_sensed") = sense<SEN_BUSYTIME_S>(tsk) / sense<SEN_TOTALTIME_S>(tsk);
        trace("nivcsw_sensed") = sense<SEN_NIVCSW>(tsk);
        trace("nvcsw_sensed") = sense<SEN_NVCSW>(tsk);
        trace("load_sensed") = sense<SEN_BUSYTIME_S>(tsk) / sense<SEN_TOTALTIME_S>(tsk);
        if(sense<SEN_PERFCNT>(PERFCNT_BUSY_CY,tsk,wid) != 0)
            trace("ipc_sensed") = sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,tsk,wid)
                                           / (double)sense<SEN_PERFCNT>(PERFCNT_BUSY_CY,tsk,wid);

        auto &predData = self->getPredData(tsk);

        if(predData.valid){
            trace("cpu_pred") = predData.cpu;
            trace("instr_pred") = predData.instr;
            trace("busyTime_pred") = predData.busyTime;
            trace("totalTime_pred") = predData.totalTime;
            trace("load_pred") = predData.load;
            trace("ipc_pred") = predData.ipc;
            trace("ipc_raw_pred") = predData.ipc_raw;
            trace("tlc_raw_pred") = predData.tlc_raw;
        }

        predData.cpu = senseIf<SEN_LASTCPU>(tsk);
        predData.instr = senseIf<SEN_PERFCNT>(PERFCNT_INSTR_EXE,tsk);
        predData.busyTime = senseIf<SEN_BUSYTIME_S>(tsk);
        predData.totalTime = senseIf<SEN_TOTALTIME_S>(tsk);
        predData.load = senseIf<SEN_BUSYTIME_S>(tsk) / senseIf<SEN_TOTALTIME_S>(tsk);
        if(senseIf<SEN_PERFCNT>(PERFCNT_BUSY_CY,tsk))
            predData.ipc = senseIf<SEN_PERFCNT>(PERFCNT_INSTR_EXE,tsk)
                                  / (double)senseIf<SEN_PERFCNT>(PERFCNT_BUSY_CY,tsk);
        DefaultBaselineModel &model = ReflectiveEngine::get().baselineModel();
        //pinfo("### tsk %d cpu=%d next=%d\n",tsk->task_idx, sense<SEN_LASTCPU>(tsk),senseIf<SEN_LASTCPU>(tsk));
        predData.ipc_raw =  model.predictIPC(tsk,wid,&(self->info()->core_list[senseIf<SEN_LASTCPU>(tsk)]),senseIf<SEN_FREQ_MHZ>(self->info()->core_list[senseIf<SEN_LASTCPU>(tsk)].freq));
        predData.tlc_raw =  model.predictTLC(tsk,wid,&(self->info()->core_list[senseIf<SEN_LASTCPU>(tsk)]),senseIf<SEN_FREQ_MHZ>(self->info()->core_list[senseIf<SEN_LASTCPU>(tsk)].freq));
        predData.valid = true;
    }

    for(int i = 0; i < self->info()->core_list_size; ++i){
        const core_info_t *core = &(self->info()->core_list[i]);
        auto &trace = self->getHandle(core,wid);
        trace("busyTime_sensed") = sense<SEN_BUSYTIME_S>(core);
        trace("totalTime_sensed") = sense<SEN_TOTALTIME_S>(core);
        trace("load_sensed") = sense<SEN_BUSYTIME_S>(core) / sense<SEN_TOTALTIME_S>(core);

        int nTasks = 0;
        for(int i = 0; i < data.numCreatedTasks(); ++i)
            if(sense<SEN_LASTCPU>(&(data.task(i))) == core->position)
                ++nTasks;
        trace("nTasks_sensed") = nTasks;

        auto &predData = self->getPredData(core);

        if(predData.valid){
            trace("busyTime_pred") = predData.busyTime;
            trace("totalTime_pred") = predData.totalTime;
            trace("load_pred") = predData.load;
        }

        predData.busyTime = senseIf<SEN_BUSYTIME_S>(core);
        predData.totalTime = senseIf<SEN_TOTALTIME_S>(core);
        predData.load = senseIf<SEN_BUSYTIME_S>(core) / senseIf<SEN_TOTALTIME_S>(core);
        predData.valid = true;
    }

    for(int i = 0; i < self->info()->freq_domain_list_size; ++i){
        const freq_domain_info_t *fd = &(self->info()->freq_domain_list[i]);
        auto &trace = self->getHandle(fd,wid);
        trace("freq_sensed") = sense<SEN_FREQ_MHZ>(fd);

        auto &predData = self->getPredData(fd);

        if(predData.valid){
            trace("freq_pred") = predData.freq;
        }

        predData.freq = senseIf<SEN_FREQ_MHZ>(fd);
        predData.valid = true;
    }

    for(int i = 0; i < self->info()->power_domain_list_size; ++i){
        const power_domain_info_t *pd = &(self->info()->power_domain_list[i]);
        auto &trace = self->getHandle(pd,wid);
        trace("power_sensed") = sense<SEN_POWER_W>(pd);

        auto &predData = self->getPredData(pd);

        if(predData.valid){
            trace("power_pred") = predData.power;
        }

        predData.power = senseIf<SEN_POWER_W>(pd);
        predData.valid = true;
    }

    for(int domain_id = 0; domain_id < self->info()->freq_domain_list_size; ++domain_id){
        actuate<ACT_FREQ_MHZ>(&(self->info()->freq_domain_list[domain_id]),
                nextFreq(&(self->info()->freq_domain_list[domain_id])));
    }

}

int main(int argc, char * argv[]){
    daemon_setup(argc,argv);
    daemon_run_sys<ModelTest>();
    return 0;
}
