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

/*
 * Models the concept of "thread load contribution" (TLC). The is defined as
 * the maximum load a task can impose on a cpu. For tasks that did not share
 * the cpu with other tasks, the TLC is simply the cpu utilization. For tasks
 * that did share the cpu, the TLC is estimated using context switch counters
 * (see the RunDMC paper for details).
 * The TLC scales according the cpu performance
 */

#ifndef __arm_rt_models_tlc_h
#define __arm_rt_models_tlc_h

#include <runtime/interfaces/common/sense_defs.h>
#include <runtime/framework/sensing_interface.h>

class TLCModel : public SensingInterface {
    //any value above this is considered full load
    static constexpr double FULL_LOAD = 0.98;

    // Returns true if the task is not sharing the current core with other
    // tasks
    static bool noSharing(const tracked_task_data_t *task, int wid)
    {
        const PerformanceData &data = SensingModule::get().data();
        for(int i = 0; i < data.numCreatedTasks(); ++i){
            const tracked_task_data_t *other = &(data.task(i));
            if((other != task) &&
               (sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,other,wid)>0) &&
               (sense<SEN_LASTCPU>(other,wid) == sense<SEN_LASTCPU>(task,wid)))
                return false;
        }
        return true;
    }

    // Estimate the task's TLC for the last window
    static double estimate_tlc(const sys_info_t *info,
            const tracked_task_data_t *task, int wid){
        double tlc = 0;
        double procTimeShare = sense<SEN_BUSYTIME_S>(task,wid) / sense<SEN_TOTALTIME_S>(&(info->core_list[sense<SEN_LASTCPU>(task,wid)]),wid);
        if(noSharing(task,wid))
            tlc = procTimeShare;
        else {
            //estimate based on context switch counters
            tlc = sense<SEN_NIVCSW>(task,wid) / (double)(sense<SEN_NIVCSW>(task,wid) + sense<SEN_NVCSW>(task,wid));

            //it cannot be smaller than the amount of cpu it has used
            if(tlc < procTimeShare)
                tlc = procTimeShare;
        }
        //ajust in case of extreme error; minimum allowable is 0.1%
        if(tlc > 1)
            tlc = 1;
        if(tlc <  0.001)
            tlc = 0.001;

        return tlc;
    }

  public:

    // Predicts the task TLC for the predicted performance given by the
    // predicted IPS when the task executes
    static double predict_tlc(const sys_info_t *info,
            const tracked_task_data_t *task, int wid,
            double predIPS)
    {
        // The current TLC
        double refTLC = estimate_tlc(info,task,wid);

        // if the demand is 1.0 sharp, we can skip some computation
        if(refTLC >= FULL_LOAD) return refTLC;

        // Obtain the current IPS
        double refIPS = sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,task,wid) / sense<SEN_BUSYTIME_S>(task,wid);

        // use the demand to create a virtual active and sleep part
        // scale the active part according to performance ratio between
        // refIPS and predIPS
        // recalculate the utilization
        double activePart = (refTLC * refIPS) / predIPS;
        double sleepPart = 1-refTLC;

        return activePart / (activePart+sleepPart);
    }



};

#endif

