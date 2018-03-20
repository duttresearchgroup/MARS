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

#include <runtime/daemon/deamonizer.h>
#include <runtime/common/reports.h>
#include <runtime/interfaces/actuation_interface.h>
#include <runtime/framework/models/hw_model.h>

class PredictorTestSystem : public PolicyManager {
protected:
    static const int WINDOW_LENGTH_MS = 500;

    virtual void setup();

    static void window_handler(int wid,PolicyManager *owner);

    struct PredData {
        double ipc;
        double power;
        int task;
        bool valid;
    };

private:
    ExecutionTrace _execTrace;
    FrequencyActuator _freqAct;
    TaskMapActuator _mapAct;
    StaticHWModel _hwModel;
    PredData _predData;

public:
    PredictorTestSystem() :PolicyManager(),
        _execTrace("trace"),
        _freqAct(*info()), _mapAct(*info()),
        _hwModel(info()),
        _predData({0,0,0,false})
    {};

};

void PredictorTestSystem::setup()
{
    sensingModule()->enablePerTaskSensing();

    sensingModule()->tracePerfCounter(PERFCNT_INSTR_EXE);
    sensingModule()->tracePerfCounter(PERFCNT_BUSY_CY);
    sensingModule()->tracePerfCounter(PERFCNT_BRANCH_MISPRED);
    sensingModule()->tracePerfCounter(PERFCNT_L1DCACHE_MISSES);
    sensingModule()->tracePerfCounter(PERFCNT_LLCACHE_MISSES);

    windowManager()->addSensingWindowHandler(WINDOW_LENGTH_MS,this,window_handler);

    //sets all domains to the same frequency
    constexpr int freqMHz = 1000;
    _freqAct.setFrameworkMode();
    for(int domain_id = 0; domain_id < info()->power_domain_list_size; ++domain_id){
        actuate<ACT_FREQ_MHZ>(info()->freq_domain_list[domain_id], freqMHz);
    }
}

void PredictorTestSystem::window_handler(int wid,PolicyManager *owner)
{
    PredictorTestSystem *self = dynamic_cast<PredictorTestSystem*>(owner);
    // Picks the task with the greatest number of instructions
    // and make sure it stays at least two epochs in one cpu type
    // before being migrated to another one


    constexpr int littleCpu = 2;
    constexpr int bigCpu = 5;
    constexpr int maxEpochs = 2;
    static int currCpu = littleCpu;
    static int epochs = maxEpochs;

    const PerformanceData &data = owner->sensedData();
    const tracked_task_data_t *highestTask = nullptr;
    uint64_t highestInstrCnt = 0;
    for(int i = 0; i < data.numCreatedTasks(); ++i){
        const tracked_task_data_t *tsk = &(data.task(i));
        uint64_t tskInstr = sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,tsk,wid);
        if(tskInstr > highestInstrCnt){
            highestInstrCnt = tskInstr;
            highestTask = tsk;
        }
    }
    if(highestTask != nullptr){
        auto trace = self->_execTrace.getHandle(wid);

        //pinfo("Sample idx %d\n",trace.sampleIdx());

        trace("task_pid") = highestTask->this_task_pid;
        trace("task_exec_cpu") = sense<SEN_LASTCPU>(highestTask,wid);
        trace("task_exec_instr") = sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,highestTask,wid);
        auto freqMhz = sense<SEN_FREQ_MHZ>(
                owner->info()->core_list[sense<SEN_LASTCPU>(highestTask,wid)].freq,
                wid
            );
        auto ipc = sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,highestTask,wid)
                                   / (double)sense<SEN_PERFCNT>(PERFCNT_BUSY_CY,highestTask,wid);
        auto power = sense<SEN_POWER_W>(
                owner->info()->core_list[sense<SEN_LASTCPU>(highestTask,wid)].power,
                wid
            );
        trace("task_exec_cpufreq") = freqMhz;
        trace("task_exec_ipc") = ipc;
        trace("task_exec_cluster_power") = power;


        if(self->_predData.valid && (self->_predData.task == highestTask->task_idx)){
            trace("task_exec_pred_ipc") = self->_predData.ipc;
            trace("task_exec_pred_cluster_power") = self->_predData.power;
            trace("task_exec_pred_ipc_error") = (1-(self->_predData.ipc / ipc))*100;
            trace("task_exec_pred_cluster_power_error") = (1-(self->_predData.power/power))*100;

            self->_hwModel.feedback(highestTask,wid,&(owner->info()->core_list[currCpu]),freqMhz,
                    ipc,
                    //Assumes other cores are idle and deducts the idle power
                    power - (self->_hwModel.idlePower(&(owner->info()->core_list[currCpu]),freqMhz) * (owner->info()->core_list[currCpu].power->core_cnt-1))
            );
        }

        if(epochs == maxEpochs){
            epochs = 0;
            if(currCpu == littleCpu) currCpu = bigCpu;
            else currCpu = littleCpu;

            actuate<ACT_TASK_MAP>(&(owner->info()->core_list[currCpu]),highestTask);
        }
        else
            ++epochs;

        self->_predData.ipc = self->_hwModel.predictIPC(highestTask,wid,&(owner->info()->core_list[currCpu]),freqMhz);
        self->_predData.power = self->_hwModel.predictPower(highestTask,wid,&(owner->info()->core_list[currCpu]),freqMhz);
        //Assumes other cores are idle and adds up the idle power
        self->_predData.power += self->_hwModel.idlePower(&(owner->info()->core_list[currCpu]),freqMhz) * (owner->info()->core_list[currCpu].power->core_cnt -1);
        self->_predData.task = highestTask->task_idx;
        self->_predData.valid = true;
    }
    else
        self->_predData.valid = false;
}


int main(int argc, char * argv[]){
    daemon_setup(argc,argv);
    daemon_run_sys<PredictorTestSystem>();
    return 0;
}
