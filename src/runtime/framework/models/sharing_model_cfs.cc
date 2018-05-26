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

#include <runtime/interfaces/sensing_module.h>
#include <runtime/framework/sensing_interface.h>
#include "sharing_model_cfs.h"

bool LinuxCFSModel::noSharing(const tracked_task_data_t *task, int wid)
{
    const PerformanceData &data = PerformanceData::localData();
    for(int i = 0; i < data.numCreatedTasks(wid); ++i){
        const tracked_task_data_t *other = &(data.task(i));
        if((other != task) &&
           (SensingInterface::Impl::sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,other,wid)>0) &&
           (SensingInterface::Impl::sense<SEN_LASTCPU>(other,wid) == SensingInterface::Impl::sense<SEN_LASTCPU>(task,wid)))
            return false;
    }
    return true;
}

double LinuxCFSModel::estimate_tlc(const tracked_task_data_t *task, int wid){
    //estimate based on context switch counters and utilization
    int cswSum = SensingInterface::Impl::sense<SEN_NIVCSW>(task,wid) + SensingInterface::Impl::sense<SEN_NVCSW>(task,wid);
    double cswRatio = SensingInterface::Impl::sense<SEN_NIVCSW>(task,wid) / (double)cswSum;
    int cpu = SensingInterface::Impl::sense<SEN_LASTCPU>(task,wid);
    assert_true((cpu >= 0) && (cpu < sys_info->core_list_size));
    double procTimeShare = SensingInterface::Impl::sense<SEN_BUSYTIME_S>(task,wid) / SensingInterface::Impl::sense<SEN_TOTALTIME_S>(&(sys_info->core_list[cpu]),wid);

    // ammout of cpu used is the lower bound for TLC;
    // or if the task did not share the CPU then TLC = ammout of cpu used
    // or if we don't have enough CSWs
    double tlc = 0;
    if(noSharing(task,wid) || (cswRatio < procTimeShare) || (cswSum < 2))
        tlc = procTimeShare;
    else
        tlc = cswRatio;

    //pinfo("task pid %d tlc=%f cswRatio=%d/(%d+%d)=%f load=%f no_sh=%d\n",
    //        task->this_task_pid,
    //        tlc,
    //        SensingInterface::Impl::sense<SEN_NIVCSW>(task,wid),SensingInterface::Impl::sense<SEN_NIVCSW>(task,wid),SensingInterface::Impl::sense<SEN_NVCSW>(task,wid),cswRatio,
    //        procTimeShare,
    //        noSharing(task,wid));

    //ajust in case of extreme error; minimum allowable is MIN_LOAD
    if(tlc > 1.0)
        tlc = 1.0;
    if(tlc <  MIN_LOAD)
        tlc = MIN_LOAD;

    return tlc;
}

double LinuxCFSModel::predict_tlc(const tracked_task_data_t *task, int wid, double predIPS)
{
    // The current TLC
    double refTLC = estimate_tlc(task,wid);

    // if the demand is 1.0 sharp, we can skip some computation
    if(refTLC >= FULL_LOAD) return refTLC;
    // busy time accuracy not enough for decent ips estim.
    if(SensingInterface::Impl::sense<SEN_BUSYTIME_S>(task,wid) < 1e-6) return MIN_LOAD;

    // Obtain the current IPS
    double refIPS = SensingInterface::Impl::sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,task,wid) / SensingInterface::Impl::sense<SEN_BUSYTIME_S>(task,wid);

    // use the demand to create a virtual active and sleep part
    // scale the active part according to performance ratio between
    // refIPS and predIPS
    // recalculate the utilization
    double activePart = (refTLC * refIPS) / predIPS;
    double sleepPart = 1-refTLC;

    return activePart / (activePart+sleepPart);
}

void LinuxCFSModel::updateCoreLoad(const core_info_t *core, int freq)
{
    core_load_info &cfs = load_info[core->position];

    cfs.equal_share = 0;
    cfs.residual_share = 0;
    cfs.task_cnt = 0;
    cfs.app_tlc_sum = 0;
    cfs.total_tlc_sum = 0;
    cfs.tlc_sum_residual = 0;

    const PerformanceData &data = PerformanceData::localData();
    for(int i = 0; i < data.numCreatedTasks(ReflectiveEngine::currentWID()); ++i){
        const tracked_task_data_t *task = &(data.task(i));
        if(ReflectiveEngine::get().predict<SEN_LASTCPU>(task) == core->position){
            auto taskTLC = baselineModel->predictTLC(task,ReflectiveEngine::currentWID(),core,freq);
            if(taskTLC != 0){
                cfs.app_tlc_sum += taskTLC;
                cfs.task_cnt += 1;
            }
        }
    }
    cfs.total_tlc_sum = cfs.app_tlc_sum;

    assert_false((cfs.task_cnt == 0) && (cfs.app_tlc_sum > 0));
    assert_false((cfs.task_cnt != 0) && (cfs.app_tlc_sum == 0));

    if(cfs.task_cnt > 0)
        cfs.equal_share = 1.0 / (double) (cfs.task_cnt);

    if((cfs.total_tlc_sum >= 1) && (cfs.task_cnt > 0)){

        for(int i = 0; i < data.numCreatedTasks(); ++i){
            const tracked_task_data_t *task = &(data.task(i));
            if(ReflectiveEngine::get().predict<SEN_LASTCPU>(task) == core->position){
                auto taskTLC = baselineModel->predictTLC(task,ReflectiveEngine::currentWID(),core,freq);
                if(taskTLC != 0){
                    if(taskTLC <= cfs.equal_share)
                        cfs.residual_share += cfs.equal_share - taskTLC;
                    else
                        cfs.tlc_sum_residual += taskTLC;
                }
            }
        }
    }

    if(cfs.total_tlc_sum < 1)
        cfs.load = cfs.total_tlc_sum;
    else
        cfs.load = 1;

    //TODO incorporate aging models
    //cfs.aged_load = cfs.load + cfs->common.core->aging_info.rel_delay_degrad_penalty;
}

void LinuxCFSModel::updateTaskLoad(const core_info_t *core, const tracked_task_data_t *task, int freq)
{
    core_load_info &cfs = load_info[core->position];

    auto taskTLC = baselineModel->predictTLC(task,ReflectiveEngine::currentWID(),core,freq);

    if(taskTLC == 0){
        task_load_info[task] = 0;
        return;
    }

    double task_load = 0;

    if((cfs.total_tlc_sum < 1) || (taskTLC <= cfs.equal_share)){
        task_load = taskTLC;
    }
    else{
        assert_false(cfs.equal_share == 0);
        assert_false(cfs.tlc_sum_residual > cfs.total_tlc_sum);
        assert_false(cfs.residual_share >= 1);

        if(cfs.tlc_sum_residual == 0){
            pinfo("WARNING: Inconsistent load on core %d task %d/%d/%s: tlc=%f tlcSumRsd=%f rsdShare=%f eqShare=%f)\n",core->position,task->task_idx,task->this_task_pid,task->this_task_name,taskTLC,cfs.tlc_sum_residual,cfs.residual_share,cfs.equal_share);
            task_load = cfs.equal_share;
        }
        else{
            double residual = (taskTLC*cfs.residual_share)/cfs.tlc_sum_residual;

            task_load = cfs.equal_share + residual;

            cfs.tlc_sum_residual -= taskTLC;
            cfs.residual_share -= residual;

            assert_false(task_load > 1);
            assert_false(task_load < cfs.equal_share);
            assert_false(cfs.tlc_sum_residual > cfs.total_tlc_sum);
            assert_false(cfs.residual_share >= 1);

            if(task_load > taskTLC){
                //return to the pool the load I'm not using
                cfs.residual_share += task_load - taskTLC;
                task_load = taskTLC;
            }
        }
    }

    assert_false(task_load > 1.0);
    assert_false(task_load == 0);//A minimum TLC value load is assigned to it
    assert_false(task_load > taskTLC);

    task_load_info[task] = task_load;

    //pinfo("\tupdateTaskLoad(c=%d tsk=%d/%d/%s load=%f tlc=%f)\n",core->position,task->task_idx,task->this_task_pid,task->this_task_name,task_load,taskTLC);
}

void LinuxCFSModel::updateState(const core_info_t *core)
{
    //pinfo("Share::updateState(c=%d)\n",core->position);
    int freq = ReflectiveEngine::get().predict<SEN_FREQ_MHZ>(core);

    updateCoreLoad(core,freq);

    const PerformanceData &data = PerformanceData::localData();
    double taskLoadSum = 0; // For consistency checks
    for(int i = 0; i < data.numCreatedTasks(ReflectiveEngine::currentWID()); ++i){
        const tracked_task_data_t *task = &(data.task(i));
        if(ReflectiveEngine::get().predict<SEN_LASTCPU>(task) == core->position){
            updateTaskLoad(core,task,freq);
            taskLoadSum += task_load_info[task];
        }
    }

    //pinfo("\tupdateCoreLoad: core %d@%d load=%f\n",core->position,freq,load_info[core->position].load);

    // Check consistency
    if(std::fabs(taskLoadSum-load_info[core->position].load) > 0.5)
        pinfo("WARNING: Inconsistent load on core %d: load=%f task_load_sum=%f (%s:%d)",core->position,load_info[core->position].load,taskLoadSum,__FILE__,__LINE__);
    else if(load_info[core->position].residual_share > 0.5)
        pinfo("WARNING: Inconsistent load on core %d: load=%f rsd_share=%f taskLoadSum=%f (%s:%d)",core->position,load_info[core->position].load,load_info[core->position].residual_share,taskLoadSum,__FILE__,__LINE__);
    else if(load_info[core->position].tlc_sum_residual > 0.5)
        pinfo("WARNING: Inconsistent load on core %d: load=%f tlc_rsd=%f taskLoadSum=%f (%s:%d)",core->position,load_info[core->position].load,load_info[core->position].tlc_sum_residual,taskLoadSum,__FILE__,__LINE__);

    // Some precision error might happen (though we expect nothing larger then above)
    // so ajust here to keep consistent
    load_info[core->position].load = taskLoadSum;


    load_info[core->position].invalid = false;
}

