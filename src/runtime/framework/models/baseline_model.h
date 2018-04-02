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
 * BaselineModel<HWModel,SharingModel> predicts performance and power given
 * the current actuations of the system. The metrics predicted by the baseline
 * model are then used to implement the senseIf functions.
 *
 * HWModel takes as input the sensed data for a task and predicts the task's
 * performance and power when executed in a different core. Performance is
 * predicted as average instructions/cycle (IPC) and power as average power in
 * Watts. These are the predicted metrics for the timeslices the task is using
 * the cpu only. To get the overall task performance when the task has idle
 * period (e.g. IO-bound tasks) and/or is sharing a CPU with other tasks, the
 * SharingModel is used.
 *
 * The SharingModel models the concept of "thread load contribution" (TLC).
 * TLC is defined as the maximum load a task can impose on a computing resource.
 * The main goal of the SharingModel is to use the TLC to predict the actual
 * load the task will impose on computing resources when the computing resource
 * is shared between multiple tasks (e.g. various tasks sharing the same cpu).
 * The way TLC and load is predicted will depend how detailed the SharingModel is.
 *
 */


#ifndef __arm_rt_models_hwmodel_h
#define __arm_rt_models_hwmodel_h

#include <fstream>
#include <sstream>

#include "baseline_model_common.h"
#include "bin_based_predictor.h"
#include "sharing_model_cfs.h"


#include <runtime/common/rt_config_params.h>
#include <runtime/common/strings.h>
#include <external/minijson_reader/minijson_reader.hpp>

template<typename HWModel, typename SharingModel>
class BaselineModel;

template<typename SharingModel>
class BaselineModel<BinBasedPred::Predictor,SharingModel> : public BaselineModelCommon {
  private:
    static constexpr int BIN_PRED_IPC_BUSY_IDX = 0;
    static constexpr int BIN_PRED_POWER_IDX = 1;

    BinBasedPred::Predictor _hwModel;

    SharingModel _sharingModel;

    std::vector<double> _hwModelBuffer;

    inline std::string _predictorPathName(){
        std::stringstream ss;
        ss << rt_param_model_path() << "/bin_pred.json";
        return ss.str();
    }
    inline std::string _idlePowerPathName(){
        std::stringstream ss;
        ss << rt_param_model_path() << "/idle_power.json";
        return ss.str();
    }

    static inline core_arch_t archFromStr(const std::string &arch)
    {
        for(int i = 0; i < SIZE_COREARCH; ++i){
            std::string aux(archToString((core_arch_t)i));
            if(aux == arch) return (core_arch_t)i;
        }
        assert_false("Couldn't find arch!");
        return SIZE_COREARCH;
    }

    void _loadHWModel()
    {
        _hwModel.loadFromFile(_predictorPathName());

        //chacks if we have the right predictor
        auto predMetrics =  _hwModel.getFuncs().final_metric;
        if(predMetrics.size() != 2)
            arm_throw(ModelException,"Loaded predictor is invalid");
        if((predMetrics[BIN_PRED_IPC_BUSY_IDX].id != BinBasedPred::BinFuncID::ipcBusy)||
           (predMetrics[BIN_PRED_POWER_IDX].id != BinBasedPred::BinFuncID::power))
            arm_throw(ModelException,"Loaded predictor is invalid");

        //load idle power
        std::ifstream is(_idlePowerPathName());
        minijson::istream_context ctx(is);

        minijson::parse_object(ctx, [&](const char* name, minijson::value value)
        {
            if(streq(name, "idle_power"))
                minijson::parse_object(ctx, [&](const char* name, minijson::value value)
                {
                    core_arch_t arch = archFromStr(name);
                    minijson::parse_object(ctx, [&](const char* name, minijson::value value)
                    {
                        int freq = fromstr<int>(name);
                        _idlePower[arch][freq] = value.as_double();
                    });
                });
            else
                minijson::ignore(ctx);
        });

        is.close();
    }


  protected:

    virtual void _modelsPredict(PredBuffer &buffer,
            const tracked_task_data_t *task, int wid,
            const core_info_t *tgtCore, int tgtFreqMhz) override
    {
        _hwModel.predict(_hwModelBuffer,task,wid,tgtCore,tgtFreqMhz);
        buffer.predIPC = _hwModelBuffer[BIN_PRED_IPC_BUSY_IDX];
        buffer.predPow = _hwModelBuffer[BIN_PRED_POWER_IDX];
        buffer.predTLC = _sharingModel.predict_tlc(task,wid,buffer.predIPC*tgtFreqMhz*1e6);
    }

  public:
    BaselineModel(const sys_info_t *sys_info)
      :BaselineModelCommon(sys_info),_hwModel(sys_info),_sharingModel(sys_info,this)
    {
        _loadHWModel();
    }

    virtual ~BaselineModel() {}

    SharingModel& sharingModel() { return _sharingModel; }

    // Called before every window handler
    void reset()
    {
        _sharingModel.reset();
    }

};

typedef BaselineModel<BinBasedPred::Predictor,LinuxCFSModel> DefaultBaselineModel;

#endif

