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
 * HWModel takes as input the sensed data for a task and predicts the task's
 * performance and power when executed in a different core. Performance is
 * predicted as average instructions/cycle (IPC) and power as average power in
 * Watts. These are the predicted metrics for the timeslices the task is using
 * the cpu only. To get the overall task performance when the task has idle
 * period (e.g. IO-bound tasks) and/or is sharing a CPU with other tasks, the
 * dynamic prediction models must be used.
 */


#ifndef __arm_rt_models_hwmodel_h
#define __arm_rt_models_hwmodel_h

#include <unordered_map>
#include <cmath>

#include <base/base.h>
#include <runtime/interfaces/common/sense_defs.h>
#include <runtime/interfaces/sensing_module.h>
#include <runtime/common/rt_config_params.h>
#include "bin_based_predictor.h"

class StaticHWModel {
  private:

    static constexpr int PRED_IPC_BUSY_IDX = 0;
    static constexpr int PRED_POWER_IDX = 1;

    BinBasedPred::Predictor _predictor;

    //map of core_arc -> freq -> per-core idle power
    std::unordered_map<int,double> _idlePower[SIZE_COREARCH];

    struct PredBuffer {
        uint64_t lastTS;
        bool valid;
        std::vector<double> predData;
        PredBuffer():lastTS(0),valid(false){}
    };
    std::unordered_map<int,PredBuffer> _predBuffer[MAX_WINDOW_CNT][MAX_CREATED_TASKS][SIZE_COREARCH];

    struct HistoryBuffer {
        bool validSense;
        bool validSensePrev;
        double sensedIPC;
        double sensedIPCPrev;
        int sensedPowerLastFreq;
        std::unordered_map<int,double> sensedPower;
        std::unordered_map<int,double> sensedPowerPrev;
        HistoryBuffer()
            :validSense(false),validSensePrev(false),
            sensedIPC(0),sensedIPCPrev(0),sensedPowerLastFreq(0)
        {}
    };
    HistoryBuffer _history[MAX_WINDOW_CNT][MAX_CREATED_TASKS][SIZE_COREARCH];

  public:
    StaticHWModel(sys_info_t *sys_info)
      :_predictor(sys_info)
    {
        _loadModels();
    }
    ~StaticHWModel(){}

    double predictIPC(const tracked_task_data_t *task, int wid, const core_info_t *tgtCore, int tgtFreqMhz)
    {
        return _predict(task,wid,tgtCore,tgtFreqMhz).predData[PRED_IPC_BUSY_IDX];
    }

    double predictPower(const tracked_task_data_t *task, int wid, const core_info_t *tgtCore, int tgtFreqMhz)
    {
        return _predict(task,wid,tgtCore,tgtFreqMhz).predData[PRED_POWER_IDX];
    }

    double idlePower(const core_info_t *core, int atFreqMhz)
    {
        auto freqI = _idlePower[core->arch].find(atFreqMhz);
        assert_true(freqI != _idlePower[core->arch].end());
        return freqI->second;
    }

    void feedback(const tracked_task_data_t *task, int wid, const core_info_t *tgtCore, int tgtFreqMhz,
            double sensedIPC, double sensedPower){
        HistoryBuffer &corrBuffer = _history[wid][task->task_idx][tgtCore->arch];
        corrBuffer.sensedIPCPrev = corrBuffer.sensedIPC;
        corrBuffer.validSensePrev = corrBuffer.validSense;
        if(corrBuffer.validSensePrev)
            corrBuffer.sensedPowerPrev[corrBuffer.sensedPowerLastFreq] = corrBuffer.sensedPower[corrBuffer.sensedPowerLastFreq];
        corrBuffer.sensedIPC = sensedIPC;
        corrBuffer.sensedPower[tgtFreqMhz] = sensedPower;
        corrBuffer.sensedPowerLastFreq = tgtFreqMhz;
        corrBuffer.validSense = true;
    }

  private:

    inline PredBuffer& _predict(const tracked_task_data_t *task, int wid, const core_info_t *tgtCore, int tgtFreqMhz)
    {
        auto buffIter =  _predBuffer[wid][task->task_idx][tgtCore->arch].find(tgtFreqMhz);
        if(buffIter == _predBuffer[wid][task->task_idx][tgtCore->arch].end()){
            _predBuffer[wid][task->task_idx][tgtCore->arch][tgtFreqMhz] = PredBuffer();
            buffIter =  _predBuffer[wid][task->task_idx][tgtCore->arch].find(tgtFreqMhz);
        }
        PredBuffer &buffer = buffIter->second;
        uint64_t ts = SensingModule::get().data().currWindowTimeMS(wid);
        if(!buffer.valid || (buffer.lastTS != ts)){
            _predictor.predict(buffer.predData,task,wid,tgtCore,tgtFreqMhz);
            buffer.valid = true;
            buffer.lastTS = ts;

            HistoryBuffer &hist = _history[wid][task->task_idx][tgtCore->arch];

            // Apply correction
            // Weights how much we want to use history vs the predicted value
            if(hist.validSense && hist.validSensePrev){
                double bias = std::fabs(hist.sensedIPC-hist.sensedIPCPrev)/hist.sensedIPC;
                if(bias > 1) bias = 1;
                buffer.predData[PRED_IPC_BUSY_IDX] =
                        (buffer.predData[PRED_IPC_BUSY_IDX]*bias) + (hist.sensedIPC*(1-bias));
                //pinfo("\t\tcorrected ipc = %f\n",buffer.predData[PRED_IPC_BUSY_IDX]);
                //pinfo("\t\t\t sen=%f senPrev=%f\n",hist.sensedIPC,hist.sensedIPCPrev);

                auto senPowI = hist.sensedPower.find(tgtFreqMhz);
                auto senPowPrevI = hist.sensedPowerPrev.find(tgtFreqMhz);
                if((senPowI != hist.sensedPower.end()) && (senPowPrevI != hist.sensedPowerPrev.end())){
                    bias = std::fabs(senPowI->second - senPowPrevI->second) / senPowI->second;
                    if(bias > 1) bias = 1;
                    buffer.predData[PRED_POWER_IDX] =
                            (buffer.predData[PRED_POWER_IDX]*bias) + (senPowI->second*(1-bias));
                    //pinfo("\t\tcorrected power = %f\n",buffer.predData[PRED_POWER_IDX]);
                    //pinfo("\t\t\t sen=%f senPrev=%f\n",senPowI->second,senPowPrevI->second);
                }
            }
        }
        return buffer;
    }

    void _loadModels();

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
};

#endif

