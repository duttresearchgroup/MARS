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

#ifndef __arm_rt_models_hwmodel_common_h
#define __arm_rt_models_hwmodel_common_h

#include <unordered_map>
#include <map>

#include <base/base.h>
#include <runtime/interfaces/common/sense_defs.h>
#include <runtime/framework/sensing_interface_impl.h>


class BaselineModelCommon : SensingInterfaceImpl {

  protected:

    const sys_info_t *_sys_info;

    //map of core_arc -> freq -> per-core idle power
    std::map<int,double> _idlePower[SIZE_COREARCH];

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

    inline double _nearestIdlePower(core_arch_t arch, int freq){
        assert_true(_idlePower[arch].size() > 0);
        if(freq <= _idlePower[arch].begin()->first) return _idlePower[arch].begin()->second;
        if(freq >= _idlePower[arch].rbegin()->first) return _idlePower[arch].rbegin()->second;
        auto upper = _idlePower[arch].lower_bound(freq);
        auto lower = upper;--lower;
        if((upper->first - freq) < (freq - lower->first))
            return upper->second;
        else
            return lower->second;
    }

  public:
    BaselineModelCommon(const sys_info_t *sys_info)
      :_sys_info(sys_info)
    {
    }

    virtual ~BaselineModelCommon(){}

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
        return _nearestIdlePower(core->arch,atFreqMhz);
    }

  private:

    void _feedback(const tracked_task_data_t *task, int wid);

    PredBuffer& _predict(const tracked_task_data_t *task, int wid, const core_info_t *tgtCore, int tgtFreqMhz);

  protected:
    virtual void _modelsPredict(PredBuffer &buffer, const tracked_task_data_t *task, int wid, const core_info_t *tgtCore, int tgtFreqMhz) = 0;

};

#endif

