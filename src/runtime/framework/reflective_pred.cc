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

///////////////////////////////////////////////////
// Reflective::predict* functions implementation
///////////////////////////////////////////////////

#include <cmath>

#include "reflective.h"
#include "actuation_interface.h"
#include <runtime/interfaces/sensing_module.h>

///////////////////////////////////////////////////////////////////
// Specialization declarations.
// Needed so we dont care about the order implementations appear

template<> typename SensingTypeInfo<SEN_PERFCNT>::ValType ReflectiveEngine::predict<SEN_PERFCNT,tracked_task_data_t>(typename SensingTypeInfo<SEN_PERFCNT>::ParamType p, const tracked_task_data_t *rsc);
template<> typename SensingTypeInfo<SEN_BEATS>::ValType ReflectiveEngine::predict<SEN_BEATS,tracked_task_data_t>(typename SensingTypeInfo<SEN_BEATS>::ParamType p, const tracked_task_data_t *rsc);
template<> typename SensingTypeInfo<SEN_BEATS_TGT>::ValType ReflectiveEngine::predict<SEN_BEATS_TGT,tracked_task_data_t>(typename SensingTypeInfo<SEN_BEATS_TGT>::ParamType p, const tracked_task_data_t *rsc);
template<> typename SensingTypeInfo<SEN_TOTALTIME_S>::ValType ReflectiveEngine::predict<SEN_TOTALTIME_S,tracked_task_data_t>(const tracked_task_data_t *rsc);
template<> typename SensingTypeInfo<SEN_BUSYTIME_S>::ValType ReflectiveEngine::predict<SEN_BUSYTIME_S,tracked_task_data_t>(const tracked_task_data_t *rsc);
template<> typename SensingTypeInfo<SEN_NIVCSW>::ValType ReflectiveEngine::predict<SEN_NIVCSW,tracked_task_data_t>(const tracked_task_data_t *rsc);
template<> typename SensingTypeInfo<SEN_NVCSW>::ValType ReflectiveEngine::predict<SEN_NVCSW,tracked_task_data_t>(const tracked_task_data_t *rsc);
template<> typename SensingTypeInfo<SEN_FREQ_MHZ>::ValType ReflectiveEngine::predict<SEN_FREQ_MHZ,tracked_task_data_t>(const tracked_task_data_t *rsc);
template<> typename SensingTypeInfo<SEN_LASTCPU>::ValType ReflectiveEngine::predict<SEN_LASTCPU,tracked_task_data_t>(const tracked_task_data_t *rsc);
template<> typename SensingTypeInfo<SEN_POWER_W>::ValType ReflectiveEngine::predict<SEN_POWER_W,tracked_task_data_t>(const tracked_task_data_t *rsc);
template<> typename SensingTypeInfo<SEN_TEMP_C>::ValType ReflectiveEngine::predict<SEN_TEMP_C,tracked_task_data_t>(const tracked_task_data_t *rsc);
template<> typename SensingTypeInfo<SEN_PERFCNT>::ValType ReflectiveEngine::predict<SEN_PERFCNT,core_info_t>(typename SensingTypeInfo<SEN_PERFCNT>::ParamType p, const core_info_t *rsc);
template<> typename SensingTypeInfo<SEN_BEATS>::ValType ReflectiveEngine::predict<SEN_BEATS,core_info_t>(typename SensingTypeInfo<SEN_BEATS>::ParamType p, const core_info_t *rsc);
template<> typename SensingTypeInfo<SEN_TOTALTIME_S>::ValType ReflectiveEngine::predict<SEN_TOTALTIME_S,core_info_t>(const core_info_t *rsc);
template<> typename SensingTypeInfo<SEN_BUSYTIME_S>::ValType ReflectiveEngine::predict<SEN_BUSYTIME_S,core_info_t>(const core_info_t *rsc);
template<> typename SensingTypeInfo<SEN_NIVCSW>::ValType ReflectiveEngine::predict<SEN_NIVCSW,core_info_t>(const core_info_t *rsc);
template<> typename SensingTypeInfo<SEN_NVCSW>::ValType ReflectiveEngine::predict<SEN_NVCSW,core_info_t>(const core_info_t *rsc);
template<> typename SensingTypeInfo<SEN_FREQ_MHZ>::ValType ReflectiveEngine::predict<SEN_FREQ_MHZ,core_info_t>(const core_info_t *rsc);
template<> typename SensingTypeInfo<SEN_LASTCPU>::ValType ReflectiveEngine::predict<SEN_LASTCPU,core_info_t>(const core_info_t *rsc);
template<> typename SensingTypeInfo<SEN_POWER_W>::ValType ReflectiveEngine::predict<SEN_POWER_W,core_info_t>(const core_info_t *rsc);
template<> typename SensingTypeInfo<SEN_TEMP_C>::ValType ReflectiveEngine::predict<SEN_TEMP_C,core_info_t>(const core_info_t *rsc);
template<> typename SensingTypeInfo<SEN_PERFCNT>::ValType ReflectiveEngine::predict<SEN_PERFCNT,freq_domain_info_t>(typename SensingTypeInfo<SEN_PERFCNT>::ParamType p, const freq_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_BEATS>::ValType ReflectiveEngine::predict<SEN_BEATS,freq_domain_info_t>(typename SensingTypeInfo<SEN_BEATS>::ParamType p, const freq_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_TOTALTIME_S>::ValType ReflectiveEngine::predict<SEN_TOTALTIME_S,freq_domain_info_t>(const freq_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_BUSYTIME_S>::ValType ReflectiveEngine::predict<SEN_BUSYTIME_S,freq_domain_info_t>(const freq_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_NIVCSW>::ValType ReflectiveEngine::predict<SEN_NIVCSW,freq_domain_info_t>(const freq_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_NVCSW>::ValType ReflectiveEngine::predict<SEN_NVCSW,freq_domain_info_t>(const freq_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_FREQ_MHZ>::ValType ReflectiveEngine::predict<SEN_FREQ_MHZ,freq_domain_info_t>(const freq_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_LASTCPU>::ValType ReflectiveEngine::predict<SEN_LASTCPU,freq_domain_info_t>(const freq_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_POWER_W>::ValType ReflectiveEngine::predict<SEN_POWER_W,freq_domain_info_t>(const freq_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_TEMP_C>::ValType ReflectiveEngine::predict<SEN_TEMP_C,freq_domain_info_t>(const freq_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_PERFCNT>::ValType ReflectiveEngine::predict<SEN_PERFCNT,power_domain_info_t>(typename SensingTypeInfo<SEN_PERFCNT>::ParamType p, const power_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_BEATS>::ValType ReflectiveEngine::predict<SEN_BEATS,power_domain_info_t>(typename SensingTypeInfo<SEN_BEATS>::ParamType p, const power_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_TOTALTIME_S>::ValType ReflectiveEngine::predict<SEN_TOTALTIME_S,power_domain_info_t>(const power_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_BUSYTIME_S>::ValType ReflectiveEngine::predict<SEN_BUSYTIME_S,power_domain_info_t>(const power_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_NIVCSW>::ValType ReflectiveEngine::predict<SEN_NIVCSW,power_domain_info_t>(const power_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_NVCSW>::ValType ReflectiveEngine::predict<SEN_NVCSW,power_domain_info_t>(const power_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_FREQ_MHZ>::ValType ReflectiveEngine::predict<SEN_FREQ_MHZ,power_domain_info_t>(const power_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_LASTCPU>::ValType ReflectiveEngine::predict<SEN_LASTCPU,power_domain_info_t>(const power_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_POWER_W>::ValType ReflectiveEngine::predict<SEN_POWER_W,power_domain_info_t>(const power_domain_info_t *rsc);
template<> typename SensingTypeInfo<SEN_TEMP_C>::ValType ReflectiveEngine::predict<SEN_TEMP_C,power_domain_info_t>(const power_domain_info_t *rsc);

///////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////
// For resource type 'tracked_task_data_t'
/////////////////////////////////////////////////

template<>
typename SensingTypeInfo<SEN_PERFCNT>::ValType
ReflectiveEngine::predict<SEN_PERFCNT,tracked_task_data_t>(typename SensingTypeInfo<SEN_PERFCNT>::ParamType p, const tracked_task_data_t *rsc)
{
    SensingTypeInfo<SEN_PERFCNT>::ValType result = 0;

    int cpu = predict<SEN_LASTCPU>(rsc);
    assert_true((cpu >= 0) && (cpu < _sys_info->core_list_size));
    const core_info_t *currCore = &(_sys_info->core_list[cpu]);
    int currentFreq = predict<SEN_FREQ_MHZ>(currCore);

    switch (p) {
        case PERFCNT_BUSY_CY:
            result = predict<SEN_BUSYTIME_S>(rsc) * currentFreq*1e6;
            break;
        case PERFCNT_INSTR_EXE:
            result = predict<SEN_BUSYTIME_S>(rsc) * currentFreq*1e6 * _baselineModel.predictIPC(rsc,currentWID(),currCore,currentFreq);
            break;
        default:
            arm_throw(SensingInterfaceException, "Prediction of perfcnt %s is not supported",perfcnt_str(p));
            break;
    }
    return result;
}

template<>
typename SensingTypeInfo<SEN_BEATS>::ValType
ReflectiveEngine::predict<SEN_BEATS,tracked_task_data_t>(typename SensingTypeInfo<SEN_BEATS>::ParamType p, const tracked_task_data_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_BEATS_TGT>::ValType
ReflectiveEngine::predict<SEN_BEATS_TGT,tracked_task_data_t>(typename SensingTypeInfo<SEN_BEATS_TGT>::ParamType p, const tracked_task_data_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_TOTALTIME_S>::ValType
ReflectiveEngine::predict<SEN_TOTALTIME_S,tracked_task_data_t>(const tracked_task_data_t *rsc)
{
    // TOTALTIME is the window length so it shouldn't change
    return SensingInterface::Impl::sense<SEN_TOTALTIME_S>(rsc,currentWID());
}

template<>
typename SensingTypeInfo<SEN_BUSYTIME_S>::ValType
ReflectiveEngine::predict<SEN_BUSYTIME_S,tracked_task_data_t>(const tracked_task_data_t *rsc)
{
    // Usess the sharing model the predict load and calculate busy time
    int cpu = predict<SEN_LASTCPU>(rsc);
    assert_true((cpu >= 0) && (cpu < _sys_info->core_list_size));
    const core_info_t *core = &(_sys_info->core_list[cpu]);
    return _baselineModel.sharingModel().predict_load(core,rsc) * predict<SEN_TOTALTIME_S>(rsc);
}

template<>
typename SensingTypeInfo<SEN_NIVCSW>::ValType
ReflectiveEngine::predict<SEN_NIVCSW,tracked_task_data_t>(const tracked_task_data_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_NVCSW>::ValType
ReflectiveEngine::predict<SEN_NVCSW,tracked_task_data_t>(const tracked_task_data_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_FREQ_MHZ>::ValType
ReflectiveEngine::predict<SEN_FREQ_MHZ,tracked_task_data_t>(const tracked_task_data_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_LASTCPU>::ValType
ReflectiveEngine::predict<SEN_LASTCPU,tracked_task_data_t>(const tracked_task_data_t *rsc)
{
    if(hasNewActuationVal<ACT_TASK_MAP>(rsc))
        return newActuationVal<ACT_TASK_MAP>(rsc)->position;
    else
        return SensingInterface::Impl::sense<SEN_LASTCPU>(rsc,currentWID());
}

template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
ReflectiveEngine::predict<SEN_POWER_W,tracked_task_data_t>(const tracked_task_data_t *rsc)
{
    int cpu = predict<SEN_LASTCPU>(rsc);
    assert_true((cpu >= 0) && (cpu < _sys_info->core_list_size));
    const core_info_t *core = &(_sys_info->core_list[cpu]);
    return _baselineModel.predictPower(rsc,currentWID(),core, predict<SEN_FREQ_MHZ>(core->freq));
}

template<>
typename SensingTypeInfo<SEN_TEMP_C>::ValType
ReflectiveEngine::predict<SEN_TEMP_C,tracked_task_data_t>(const tracked_task_data_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}


/////////////////////////////////////////////////
// For resource type 'core_info_t'
/////////////////////////////////////////////////

template<>
typename SensingTypeInfo<SEN_PERFCNT>::ValType
ReflectiveEngine::predict<SEN_PERFCNT,core_info_t>(typename SensingTypeInfo<SEN_PERFCNT>::ParamType p, const core_info_t *rsc)
{
    SensingTypeInfo<SEN_PERFCNT>::ValType acc = 0;
    const PerformanceData &data = PerformanceData::localData();
    for(int i = 0; i < data.numCreatedTasks(ReflectiveEngine::currentWID()); ++i){
        const tracked_task_data_t *task = &(data.task(i));
        if(predict<SEN_LASTCPU>(task) == rsc->position)
            acc += predict<SEN_PERFCNT>(p,task);
    }
    return acc;
}

template<>
typename SensingTypeInfo<SEN_BEATS>::ValType
ReflectiveEngine::predict<SEN_BEATS,core_info_t>(typename SensingTypeInfo<SEN_BEATS>::ParamType p, const core_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_TOTALTIME_S>::ValType
ReflectiveEngine::predict<SEN_TOTALTIME_S,core_info_t>(const core_info_t *rsc)
{
    // TOTALTIME is the window length so it shouldn't change
    return SensingInterface::Impl::sense<SEN_TOTALTIME_S>(rsc,currentWID());
}

template<>
typename SensingTypeInfo<SEN_BUSYTIME_S>::ValType
ReflectiveEngine::predict<SEN_BUSYTIME_S,core_info_t>(const core_info_t *rsc)
{
    return _baselineModel.sharingModel().predict_load(rsc) * predict<SEN_TOTALTIME_S>(rsc);
}

template<>
typename SensingTypeInfo<SEN_NIVCSW>::ValType
ReflectiveEngine::predict<SEN_NIVCSW,core_info_t>(const core_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_NVCSW>::ValType
ReflectiveEngine::predict<SEN_NVCSW,core_info_t>(const core_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_FREQ_MHZ>::ValType
ReflectiveEngine::predict<SEN_FREQ_MHZ,core_info_t>(const core_info_t *rsc)
{
    return predict<SEN_FREQ_MHZ>(rsc->freq);
}

template<>
typename SensingTypeInfo<SEN_LASTCPU>::ValType
ReflectiveEngine::predict<SEN_LASTCPU,core_info_t>(const core_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
ReflectiveEngine::predict<SEN_POWER_W,core_info_t>(const core_info_t *rsc)
{
    // Gets the average power consumed by all tasks
    // and adds the idle power (if any)
    typename SensingTypeInfo<SEN_POWER_W>::ValType tasksPower = 0;
    int cnt = 0;
    const PerformanceData &data = PerformanceData::localData();
    for(int i = 0; i < data.numCreatedTasks(ReflectiveEngine::currentWID()); ++i){
        const tracked_task_data_t *task = &(data.task(i));
        if(predict<SEN_LASTCPU>(task) == rsc->position){
            tasksPower += predict<SEN_POWER_W>(task);
            ++cnt;
        }
    }
    if(tasksPower > 0)
        tasksPower /= cnt;

    double load = _baselineModel.sharingModel().predict_load(rsc);

    double idlePower = _baselineModel.idlePower(rsc,predict<SEN_FREQ_MHZ>(rsc));

    double totalPower = (tasksPower*load) + (idlePower*(1-load));

    //pinfo("\tcore %d@%d power=%f\n",rsc->position,(int)predict<SEN_FREQ_MHZ>(rsc),totalPower);

    return totalPower;
}

template<>
typename SensingTypeInfo<SEN_TEMP_C>::ValType
ReflectiveEngine::predict<SEN_TEMP_C,core_info_t>(const core_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}


/////////////////////////////////////////////////
// For resource type 'freq_domain_info_t'
/////////////////////////////////////////////////

template<>
typename SensingTypeInfo<SEN_PERFCNT>::ValType
ReflectiveEngine::predict<SEN_PERFCNT,freq_domain_info_t>(typename SensingTypeInfo<SEN_PERFCNT>::ParamType p, const freq_domain_info_t *rsc)
{
    //pinfo("predict<SEN_PERFCNT,freq>(p=%s, fd=%d)\n",perfcnt_str(p),rsc->domain_id);
    SensingTypeInfo<SEN_PERFCNT>::ValType acc = 0;
    for(int i = 0; i < _sys_info->core_list_size; ++i){
        core_info_t *c = &(_sys_info->core_list[i]);
        if(c->freq == rsc)
            acc += predict<SEN_PERFCNT>(p,c);
    }
    return acc;
}

template<>
typename SensingTypeInfo<SEN_BEATS>::ValType
ReflectiveEngine::predict<SEN_BEATS,freq_domain_info_t>(typename SensingTypeInfo<SEN_BEATS>::ParamType p, const freq_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_TOTALTIME_S>::ValType
ReflectiveEngine::predict<SEN_TOTALTIME_S,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    // TOTALTIME is the window length so it should be the same for all cores in the system
    //pinfo("predict<SEN_TOTALTIME,freq>(fd=%d)\n",rsc->domain_id);
    return predict<SEN_TOTALTIME_S>(rsc->__vitaminslist_head_cores);
}

template<>
typename SensingTypeInfo<SEN_BUSYTIME_S>::ValType
ReflectiveEngine::predict<SEN_BUSYTIME_S,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_NIVCSW>::ValType
ReflectiveEngine::predict<SEN_NIVCSW,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_NVCSW>::ValType
ReflectiveEngine::predict<SEN_NVCSW,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_FREQ_MHZ>::ValType
ReflectiveEngine::predict<SEN_FREQ_MHZ,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    if(hasNewActuationVal<ACT_FREQ_MHZ>(rsc))
        return newActuationVal<ACT_FREQ_MHZ>(rsc);
    else
        return SensingInterface::Impl::sense<SEN_FREQ_MHZ>(rsc,currentWID());
}

template<>
typename SensingTypeInfo<SEN_LASTCPU>::ValType
ReflectiveEngine::predict<SEN_LASTCPU,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
ReflectiveEngine::predict<SEN_POWER_W,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    //pinfo("predict<SEN_POWER,freq>(fd=%d)\n",rsc->domain_id);
    typename SensingTypeInfo<SEN_POWER_W>::ValType acc = 0;
    for(int i = 0; i < _sys_info->power_domain_list_size; ++i){
        const power_domain_info_t *pd = &(_sys_info->power_domain_list[i]);
        if(pd->freq_domain == rsc)
            acc += predict<SEN_POWER_W>(pd);
    }
    return acc;
}

template<>
typename SensingTypeInfo<SEN_TEMP_C>::ValType
ReflectiveEngine::predict<SEN_TEMP_C,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}


/////////////////////////////////////////////////
// For resource type 'power_domain_info_t'
/////////////////////////////////////////////////

template<>
typename SensingTypeInfo<SEN_PERFCNT>::ValType
ReflectiveEngine::predict<SEN_PERFCNT,power_domain_info_t>(typename SensingTypeInfo<SEN_PERFCNT>::ParamType p, const power_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_BEATS>::ValType
ReflectiveEngine::predict<SEN_BEATS,power_domain_info_t>(typename SensingTypeInfo<SEN_BEATS>::ParamType p, const power_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_TOTALTIME_S>::ValType
ReflectiveEngine::predict<SEN_TOTALTIME_S,power_domain_info_t>(const power_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_BUSYTIME_S>::ValType
ReflectiveEngine::predict<SEN_BUSYTIME_S,power_domain_info_t>(const power_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_NIVCSW>::ValType
ReflectiveEngine::predict<SEN_NIVCSW,power_domain_info_t>(const power_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_NVCSW>::ValType
ReflectiveEngine::predict<SEN_NVCSW,power_domain_info_t>(const power_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_FREQ_MHZ>::ValType
ReflectiveEngine::predict<SEN_FREQ_MHZ,power_domain_info_t>(const power_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_LASTCPU>::ValType
ReflectiveEngine::predict<SEN_LASTCPU,power_domain_info_t>(const power_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}

template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
ReflectiveEngine::predict<SEN_POWER_W,power_domain_info_t>(const power_domain_info_t *rsc)
{
    typename SensingTypeInfo<SEN_POWER_W>::ValType acc = 0;
    for(int i = 0; i < _sys_info->core_list_size; ++i){
        const core_info_t *core = &(_sys_info->core_list[i]);
        if(core->power == rsc)
            acc += predict<SEN_POWER_W>(core);
    }
    return acc;
}

template<>
typename SensingTypeInfo<SEN_TEMP_C>::ValType
ReflectiveEngine::predict<SEN_TEMP_C,power_domain_info_t>(const power_domain_info_t *rsc)
{
    arm_throw(SensingInterfaceException,"%s not implemented",__PRETTY_FUNCTION__);
}


/////////////////////////////////////////////////
// For resource type 'NullResource'
/////////////////////////////////////////////////

template<>
typename SensingTypeInfo<SEN_DUMMY>::ValType
ReflectiveEngine::predict<SEN_DUMMY,NullResource>(const NullResource *rsc)
{
    // Dummy is the sum of ACT_DUMMY1 and ACT_DUMMY2
    int dummy1 = 0;
    int dummy2 = 0;

    if(hasNewActuationVal<ACT_DUMMY1>(rsc))
        dummy1 += newActuationVal<ACT_DUMMY1>(rsc);
    else
        dummy1 += ActuationInterface::actuationVal<ACT_DUMMY1>(rsc);

    if(hasNewActuationVal<ACT_DUMMY2>(rsc))
        dummy2 += newActuationVal<ACT_DUMMY2>(rsc);
    else
        dummy2 += ActuationInterface::actuationVal<ACT_DUMMY2>(rsc);

    //pinfo("\t\t***PRED_DUMMY: SEN_DUMMY is %d (%d(%d) + %d(%d))\n",dummy1+dummy2,dummy1,hasNewActuationVal<ACT_DUMMY1>(rsc),dummy2,hasNewActuationVal<ACT_DUMMY2>(rsc));

    return dummy1+dummy2;
}
