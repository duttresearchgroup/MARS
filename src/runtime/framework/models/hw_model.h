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
#include <runtime/framework/sensing_interface.h>
#include "bin_based_predictor.h"
#include "tlc.h"

class StaticHWModel : public SensingInterface {
  private:

    static constexpr int BIN_PRED_IPC_BUSY_IDX = 0;
    static constexpr int BIN_PRED_POWER_IDX = 1;

    const sys_info_t *_sys_info;

    BinBasedPred::Predictor _binPred;
    std::vector<double> _binPredBuffer;

    //map of core_arc -> freq -> per-core idle power
    std::unordered_map<int,double> _idlePower[SIZE_COREARCH];

    struct PredBuffer {
        uint64_t lastTS;
        bool valid;
        double predIPC;
        double predPow;
        double predTLC;
        PredBuffer():lastTS(0),valid(false),predIPC(0),predPow(0),predTLC(0){}
    };
    std::unordered_map<int,PredBuffer> _predBuffer[MAX_WINDOW_CNT][MAX_CREATED_TASKS][SIZE_COREARCH];

    struct HistoryBuffer {
        bool validSense;
        bool validSensePrev;
        double sensedIPC;
        double sensedIPCPrev;
        int sensedPowerLastFreq;
        int sensedTLCLastFreq;
        bool sensedPowerLastFreqValid;
        bool sensedTLCLastFreqValid;
        std::unordered_map<int,double> sensedPower;
        std::unordered_map<int,double> sensedPowerPrev;
        std::unordered_map<int,double> sensedTLC;
        std::unordered_map<int,double> sensedTLCPrev;
        HistoryBuffer()
            :validSense(false),validSensePrev(false),
            sensedIPC(0),sensedIPCPrev(0),
            sensedPowerLastFreq(0), sensedTLCLastFreq(0),
            sensedPowerLastFreqValid(false),
            sensedTLCLastFreqValid(false)
        {}
    };
    HistoryBuffer _history[MAX_WINDOW_CNT][MAX_CREATED_TASKS][SIZE_COREARCH];

  public:
    StaticHWModel(sys_info_t *sys_info)
      :_sys_info(sys_info),_binPred(sys_info)
    {
        _loadModels();
    }
    ~StaticHWModel(){}

    double predictIPC(const tracked_task_data_t *task, int wid, const core_info_t *tgtCore, int tgtFreqMhz)
    {
        return _predict(task,wid,tgtCore,tgtFreqMhz).predIPC;
    }

    double predictPower(const tracked_task_data_t *task, int wid, const core_info_t *tgtCore, int tgtFreqMhz)
    {
        return _predict(task,wid,tgtCore,tgtFreqMhz).predPow;
    }

    double predictTLC(const tracked_task_data_t *task, int wid, const core_info_t *tgtCore, int tgtFreqMhz)
    {
        return _predict(task,wid,tgtCore,tgtFreqMhz).predTLC;
    }

    double idlePower(const core_info_t *core, int atFreqMhz)
    {
        auto freqI = _idlePower[core->arch].find(atFreqMhz);
        assert_true(freqI != _idlePower[core->arch].end());
        return freqI->second;
    }

  private:

    void _feedback(const tracked_task_data_t *task, int wid)
    {
        uint64_t instr = sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,task,wid);

        if(instr == 0) return;

        double ipc = (double) instr / (double)sense<SEN_PERFCNT>(PERFCNT_BUSY_CY,task,wid);
        const core_info_t *core = &(_sys_info->core_list[sense<SEN_LASTCPU>(task,wid)]);
        int freq = sense<SEN_FREQ_MHZ>(core->freq,wid);

        uint64_t coreIntr = sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,core,wid);
        uint64_t domainInstr = 0;
        for(int i = 0; i < _sys_info->core_list_size; ++i) {
            const core_info_t *_core = &(_sys_info->core_list[i]);
            if(_core->power == core->power)
                domainInstr += sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,_core,wid);
        }

        HistoryBuffer &corrBuffer = _history[wid][task->task_idx][core->arch];
        corrBuffer.sensedIPCPrev = corrBuffer.sensedIPC;
        corrBuffer.validSensePrev = corrBuffer.validSense;
        if(corrBuffer.sensedPowerLastFreqValid)
            corrBuffer.sensedPowerPrev[corrBuffer.sensedPowerLastFreq] = corrBuffer.sensedPower[corrBuffer.sensedPowerLastFreq];
        if(corrBuffer.sensedTLCLastFreqValid)
            corrBuffer.sensedTLCPrev[corrBuffer.sensedTLCLastFreq] = corrBuffer.sensedTLC[corrBuffer.sensedTLCLastFreq];

        corrBuffer.sensedIPC = ipc;

        //mostly the single task in this core. We can extract TLC
        //pinfo("core %d ratio = %f\n",core->position,instr / (double)coreIntr);
        if((instr / (double)coreIntr) > 0.95){
            corrBuffer.sensedTLC[freq] = sense<SEN_BUSYTIME_S>(core,wid) / sense<SEN_TOTALTIME_S>(core,wid);
            corrBuffer.sensedTLCLastFreq = freq;
            corrBuffer.sensedTLCLastFreqValid = true;
            //pinfo("\t@%d tlc = %f\n",freq,corrBuffer.sensedTLC[freq]);
        }

        //mostly the single task in this power domain. We can extract task power
        double domainRatio = instr / (double)domainInstr;
        //pinfo("pd %d ratio = %f\n",core->power->domain_id, domainRatio);
        if(domainRatio > 0.9){
            //Assumes other cores are idle and deducts the idle power
            double power = sense<SEN_POWER_W>(core->power,wid);
            power -= idlePower(core,freq) * (core->power->core_cnt-1);
            corrBuffer.sensedPower[freq] = power;
            corrBuffer.sensedPowerLastFreq = freq;
            corrBuffer.sensedPowerLastFreqValid = true;
            //pinfo("\t@%d pow = %f\n",freq,corrBuffer.sensedPower[freq]);
        }

        corrBuffer.validSense = true;
    }


    inline PredBuffer& _predict(const tracked_task_data_t *task, int wid, const core_info_t *tgtCore, int tgtFreqMhz)
    {
        auto buffIter = _predBuffer[wid][task->task_idx][tgtCore->arch].find(tgtFreqMhz);
        if(buffIter == _predBuffer[wid][task->task_idx][tgtCore->arch].end()){
            _predBuffer[wid][task->task_idx][tgtCore->arch][tgtFreqMhz] = PredBuffer();
            buffIter =  _predBuffer[wid][task->task_idx][tgtCore->arch].find(tgtFreqMhz);
        }
        PredBuffer &buffer = buffIter->second;
        uint64_t ts = SensingModule::get().data().currWindowTimeMS(wid);
        if(!buffer.valid || (buffer.lastTS != ts)){

            //feedback current measurements first
            _feedback(task,wid);

            _binPred.predict(_binPredBuffer,task,wid,tgtCore,tgtFreqMhz);
            buffer.predIPC = _binPredBuffer[BIN_PRED_IPC_BUSY_IDX];
            buffer.predPow = _binPredBuffer[BIN_PRED_POWER_IDX];
            buffer.predTLC = TLCModel::predict_tlc(_sys_info,task,wid,buffer.predIPC*tgtFreqMhz*1e6);
            buffer.valid = true;
            buffer.lastTS = ts;

            HistoryBuffer &hist = _history[wid][task->task_idx][tgtCore->arch];

            // Apply correction
            // Weights how much we want to use history vs the predicted value
            if(hist.validSense && hist.validSensePrev){
                double bias = std::fabs(hist.sensedIPC-hist.sensedIPCPrev)/hist.sensedIPC;
                if(bias > 1) bias = 1;
                buffer.predIPC = (buffer.predIPC*bias) + (hist.sensedIPC*(1-bias));
                //pinfo("\t\tcorrected ipc = %f\n",buffer.predData[PRED_IPC_BUSY_IDX]);
                //pinfo("\t\t\t sen=%f senPrev=%f\n",hist.sensedIPC,hist.sensedIPCPrev);

                auto senPowI = hist.sensedPower.find(tgtFreqMhz);
                auto senPowPrevI = hist.sensedPowerPrev.find(tgtFreqMhz);
                if((senPowI != hist.sensedPower.end()) && (senPowPrevI != hist.sensedPowerPrev.end())){
                    bias = std::fabs(senPowI->second - senPowPrevI->second) / senPowI->second;
                    if(bias > 1) bias = 1;
                    buffer.predPow = (buffer.predPow*bias) + (senPowI->second*(1-bias));
                    //pinfo("\t\tcorrected power = %f\n",buffer.predData[PRED_POWER_IDX]);
                    //pinfo("\t\t\t sen=%f senPrev=%f\n",senPowI->second,senPowPrevI->second);
                }

                auto senTLCI = hist.sensedTLC.find(tgtFreqMhz);
                auto senTLCPrevI = hist.sensedTLCPrev.find(tgtFreqMhz);
                if((senTLCI != hist.sensedTLC.end()) && (senTLCPrevI != hist.sensedTLCPrev.end())){
                    bias = std::fabs(senTLCI->second - senTLCPrevI->second) / senTLCI->second;
                    if(bias > 1) bias = 1;
                    buffer.predTLC = (buffer.predTLC*bias) + (senTLCI->second*(1-bias));
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

