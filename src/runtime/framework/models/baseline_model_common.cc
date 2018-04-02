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
#include "baseline_model_common.h"

void BaselineModelCommon::_feedback(const tracked_task_data_t *task, int wid)
{
    uint64_t instr = SensingInterfaceImpl::Impl::sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,task,wid);

    if(instr == 0) return;

    double ipc = (double) instr / (double)SensingInterfaceImpl::Impl::sense<SEN_PERFCNT>(PERFCNT_BUSY_CY,task,wid);
    const core_info_t *core = &(_sys_info->core_list[SensingInterfaceImpl::Impl::sense<SEN_LASTCPU>(task,wid)]);
    int freq = SensingInterfaceImpl::Impl::sense<SEN_FREQ_MHZ>(core->freq,wid);

    uint64_t coreIntr = SensingInterfaceImpl::Impl::sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,core,wid);
    uint64_t domainInstr = 0;
    for(int i = 0; i < _sys_info->core_list_size; ++i) {
        const core_info_t *_core = &(_sys_info->core_list[i]);
        if(_core->power == core->power)
            domainInstr += SensingInterfaceImpl::Impl::sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,_core,wid);
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
        corrBuffer.sensedTLC[freq] = SensingInterfaceImpl::Impl::sense<SEN_BUSYTIME_S>(core,wid) / SensingInterfaceImpl::Impl::sense<SEN_TOTALTIME_S>(core,wid);
        corrBuffer.sensedTLCLastFreq = freq;
        corrBuffer.sensedTLCLastFreqValid = true;
        //pinfo("\t@%d tlc = %f\n",freq,corrBuffer.sensedTLC[freq]);
    }

    //mostly the single task in this power domain. We can extract task power
    double domainRatio = instr / (double)domainInstr;
    //pinfo("pd %d ratio = %f\n",core->power->domain_id, domainRatio);
    if(domainRatio > 0.9){
        //Assumes other cores are idle and deducts the idle power
        double power = SensingInterfaceImpl::Impl::sense<SEN_POWER_W>(core->power,wid);
        power -= idlePower(core,freq) * (core->power->core_cnt-1);
        corrBuffer.sensedPower[freq] = power;
        corrBuffer.sensedPowerLastFreq = freq;
        corrBuffer.sensedPowerLastFreqValid = true;
        //pinfo("\t@%d pow = %f\n",freq,corrBuffer.sensedPower[freq]);
    }

    corrBuffer.validSense = true;
}

BaselineModelCommon::PredBuffer& BaselineModelCommon::_predict(const tracked_task_data_t *task, int wid, const core_info_t *tgtCore, int tgtFreqMhz)
{
    auto buffIter = _predBuffer[wid][task->task_idx][tgtCore->arch].find(tgtFreqMhz);
    if(buffIter == _predBuffer[wid][task->task_idx][tgtCore->arch].end()){
        _predBuffer[wid][task->task_idx][tgtCore->arch][tgtFreqMhz] = PredBuffer();
        buffIter =  _predBuffer[wid][task->task_idx][tgtCore->arch].find(tgtFreqMhz);
    }
    PredBuffer &buffer = buffIter->second;
    uint64_t ts = SensingModule::get().data().currWindowTimeMS(wid);
    if(!buffer.valid || (buffer.lastTS != ts)){
        if((SensingInterfaceImpl::Impl::sense<SEN_BUSYTIME_S>(task,wid) == 0) ||
           (SensingInterfaceImpl::Impl::sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,task,wid) == 0)){
            //Tasks had not executed in the last window
            buffer.predIPC = 0;
            buffer.predTLC = 0;
            buffer.predPow = 0;
            buffer.valid = true;
            buffer.lastTS = ts;
        }
        else {
            // OK, predict

            // But first feedback current measurements first for error correction
            _feedback(task,wid);

            //predict
            _modelsPredict(buffer,task,wid,tgtCore,tgtFreqMhz);
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
    }
    return buffer;
}

