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
 * Models TLC and cpu load when tasks that share cpus are scheduled using
 * Linux's CFS policy. The TLC is defined as the maximum load a task can impose
 * on a cpu. For tasks that did not share the cpu with other tasks, the TLC is
 * simply the cpu utilization. For tasks that did share the cpu, the TLC is
 * estimated using context switch counters (see the RunDMC paper for details).
 * The TLC scales according the cpu performance
 */

#ifndef __arm_rt_models_tlc_h
#define __arm_rt_models_tlc_h

#include <runtime/framework/sensing_interface_impl.h>
#include "baseline_model_common.h"

class LinuxCFSModel : SensingInterfaceImpl {
    //any value above this is considered full load
    static constexpr double FULL_LOAD = 0.98;
    static constexpr double MIN_LOAD = 0.001;

    // Returns true if the task is not sharing the current core with other
    // tasks
    bool noSharing(const tracked_task_data_t *task, int wid);

    // Estimate the task's TLC for the last window
    double estimate_tlc(const tracked_task_data_t *task, int wid);

    const sys_info_t *sys_info;

    struct core_load_info {
        bool invalid;

        // Considers only tasks that executed
        int task_cnt;

        double load;

        double equal_share;
        double residual_share;
        double app_tlc_sum;
        double total_tlc_sum;
        double tlc_sum_residual;

        double aged_load;//for aging stuff
    };

    bool *needs_update;

    core_load_info *load_info;

    std::unordered_map<const tracked_task_data_t*,double> task_load_info;

    BaselineModelCommon *baselineModel;

    void updateCoreLoad(const core_info_t *core, int freq);
    void updateTaskLoad(const core_info_t *core, const tracked_task_data_t *task, int freq);

    void updateState(const core_info_t *core);

  public:

    LinuxCFSModel(const sys_info_t *si, BaselineModelCommon *bm)
      :sys_info(si), needs_update(nullptr), load_info(nullptr),  baselineModel(bm)
    {
        assert_true(si != nullptr);
        assert_true(si->core_list_size > 0);
        assert_true(si->core_list[0].position == 0);
        assert_true(si->core_list[si->core_list_size-1].position == (si->core_list_size-1));
        assert_true(bm != nullptr);

        load_info = new core_load_info[si->core_list_size];
        needs_update = new bool[si->core_list_size];

        reset();
    }
    ~LinuxCFSModel()
    {
        delete[] load_info;
        delete[] needs_update;
    }

    // Predicts the task TLC for the predicted performance given by the
    // predicted IPS when the task executes
    double predict_tlc(const tracked_task_data_t *task, int wid, double predIPS);


    double predict_load(const core_info_t *core, const tracked_task_data_t *task)
    {
        //pinfo("Share::predict_load(c=%d tsk=%d/%d/%s)\n",core->position,task->task_idx,task->this_task_pid,task->this_task_name);
        if(needs_update[core->position]){
            updateState(core);
            needs_update[core->position] = false;
        }
        auto iter = task_load_info.find(task);
        assert_true(iter != task_load_info.end());
        return iter->second;
    }

    double predict_load(const core_info_t *core)
    {
        //pinfo("Share::predict_load(c=%d)\n",core->position);
        if(needs_update[core->position]){
            updateState(core);
            needs_update[core->position] = false;
        }
        assert_true(load_info[core->position].invalid == false);
        return load_info[core->position].load;
    }

    // MUST be called everytime an actuation may affect this core
    void actuationNotify(const core_info_t *core)
    {
        //if(!needs_update[core->position]){
        //    pinfo("Share::actuationNotify(c=%d)\n",core->position);
            needs_update[core->position] = true;
        //}
    }

    // Called before every window handler
    void reset()
    {
        //pinfo("Share::reset()\n");
        for(int i = 0; i < sys_info->core_list_size; ++i){
            load_info[i].invalid = true;
            needs_update[i] = false;
        }
    }
};

#endif

