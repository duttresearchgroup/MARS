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

#include <type_traits>

#include <runtime/framework/sensing_interface.h>
#include <runtime/interfaces/performance_data.h>
#include <runtime/interfaces/sensing_module.h>

/*
 * Implementation of the sensing interface for performance sensing
 */


/*
 * SEN_PERFCNT for cores and tasks
 */
//cores
template<>
typename SensingTypeInfo<SEN_PERFCNT>::ValType
SensingInterface::sense<SEN_PERFCNT,core_info_t>(SensingTypeInfo<SEN_PERFCNT>::ParamType p,const core_info_t *rsc, int wid)
{
    static_assert(std::is_same<decltype(p), perfcnt_t>::value, "p must be perfcnt_t");
    const PerformanceData &data = SensingModule::get().data();
    assert_true(data.perfCntAvailable(p));
    return data.getPerfcntVal(data.swCurrData(wid).cpus[rsc->position].perfcnt,p);
}
template<>
typename SensingTypeInfo<SEN_PERFCNT>::ValType
SensingInterface::senseAgg<SEN_PERFCNT,core_info_t>(SensingTypeInfo<SEN_PERFCNT>::ParamType p,const core_info_t *rsc, int wid)
{
    static_assert(std::is_same<decltype(p), perfcnt_t>::value, "p must be perfcnt_t");
    const PerformanceData &data = SensingModule::get().data();
    assert_true(data.perfCntAvailable(p));
    return data.getPerfcntVal(data.swAggrData(wid).cpus[rsc->position].perfcnt,p);
}

//tasks
template<>
typename SensingTypeInfo<SEN_PERFCNT>::ValType
SensingInterface::sense<SEN_PERFCNT,tracked_task_data_t>(SensingTypeInfo<SEN_PERFCNT>::ParamType p,const tracked_task_data_t *rsc, int wid)
{
    static_assert(std::is_same<decltype(p), perfcnt_t>::value, "p must be perfcnt_t");
    const PerformanceData &data = SensingModule::get().data();
    assert_true(data.perfCntAvailable(p));
    return data.getPerfcntVal(data.swCurrData(wid).tasks[rsc->task_idx].perfcnt,p);
}
template<>
typename SensingTypeInfo<SEN_PERFCNT>::ValType
SensingInterface::senseAgg<SEN_PERFCNT,tracked_task_data_t>(SensingTypeInfo<SEN_PERFCNT>::ParamType p,const tracked_task_data_t *rsc, int wid)
{
    static_assert(std::is_same<decltype(p), perfcnt_t>::value, "p must be perfcnt_t");
    const PerformanceData &data = SensingModule::get().data();
    assert_true(data.perfCntAvailable(p));
    return data.getPerfcntVal(data.swAggrData(wid).tasks[rsc->task_idx].perfcnt,p);
}


/*
 * SEN_TOTALTIME_S for cores and tasks
 */
//cores
template<>
typename SensingTypeInfo<SEN_TOTALTIME_S>::ValType
SensingInterface::sense<SEN_TOTALTIME_S,core_info_t>(const core_info_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swCurrData(wid).cpus[rsc->position].perfcnt.time_total_ms / 1000.0;
}
template<>
typename SensingTypeInfo<SEN_TOTALTIME_S>::ValType
SensingInterface::senseAgg<SEN_TOTALTIME_S,core_info_t>(const core_info_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swAggrData(wid).cpus[rsc->position].perfcnt.time_total_ms / 1000.0;
}

//tasks
template<>
typename SensingTypeInfo<SEN_TOTALTIME_S>::ValType
SensingInterface::sense<SEN_TOTALTIME_S,tracked_task_data_t>(const tracked_task_data_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swCurrData(wid).tasks[rsc->task_idx].perfcnt.time_total_ms / 1000.0;
}
template<>
typename SensingTypeInfo<SEN_TOTALTIME_S>::ValType
SensingInterface::senseAgg<SEN_TOTALTIME_S,tracked_task_data_t>(const tracked_task_data_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swAggrData(wid).tasks[rsc->task_idx].perfcnt.time_total_ms / 1000.0;
}


/*
 * SEN_BUSYTIME_S for cores and tasks
 */
//cores
template<>
typename SensingTypeInfo<SEN_BUSYTIME_S>::ValType
SensingInterface::sense<SEN_BUSYTIME_S,core_info_t>(const core_info_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swCurrData(wid).cpus[rsc->position].perfcnt.time_busy_ms / 1000.0;
}
template<>
typename SensingTypeInfo<SEN_BUSYTIME_S>::ValType
SensingInterface::senseAgg<SEN_BUSYTIME_S,core_info_t>(const core_info_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swAggrData(wid).cpus[rsc->position].perfcnt.time_busy_ms / 1000.0;
}

//tasks
template<>
typename SensingTypeInfo<SEN_BUSYTIME_S>::ValType
SensingInterface::sense<SEN_BUSYTIME_S,tracked_task_data_t>(const tracked_task_data_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swCurrData(wid).tasks[rsc->task_idx].perfcnt.time_busy_ms / 1000.0;
}
template<>
typename SensingTypeInfo<SEN_BUSYTIME_S>::ValType
SensingInterface::senseAgg<SEN_BUSYTIME_S,tracked_task_data_t>(const tracked_task_data_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swAggrData(wid).tasks[rsc->task_idx].perfcnt.time_busy_ms / 1000.0;
}



/*
 * SEN_NIVCSW for cores and tasks
 */
//cores
template<>
typename SensingTypeInfo<SEN_NIVCSW>::ValType
SensingInterface::sense<SEN_NIVCSW,core_info_t>(const core_info_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return data.swCurrData(wid).cpus[rsc->position].perfcnt.nivcsw;
}
template<>
typename SensingTypeInfo<SEN_NIVCSW>::ValType
SensingInterface::senseAgg<SEN_NIVCSW,core_info_t>(const core_info_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swAggrData(wid).cpus[rsc->position].perfcnt.nivcsw;
}

//tasks
template<>
typename SensingTypeInfo<SEN_NIVCSW>::ValType
SensingInterface::sense<SEN_NIVCSW,tracked_task_data_t>(const tracked_task_data_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swCurrData(wid).tasks[rsc->task_idx].perfcnt.nivcsw;
}
template<>
typename SensingTypeInfo<SEN_NIVCSW>::ValType
SensingInterface::senseAgg<SEN_NIVCSW,tracked_task_data_t>(const tracked_task_data_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swAggrData(wid).tasks[rsc->task_idx].perfcnt.nivcsw;
}

/*
 * SEN_NVCSW for cores and tasks
 */
//cores
template<>
typename SensingTypeInfo<SEN_NVCSW>::ValType
SensingInterface::sense<SEN_NVCSW,core_info_t>(const core_info_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return data.swCurrData(wid).cpus[rsc->position].perfcnt.nvcsw;
}
template<>
typename SensingTypeInfo<SEN_NVCSW>::ValType
SensingInterface::senseAgg<SEN_NVCSW,core_info_t>(const core_info_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swAggrData(wid).cpus[rsc->position].perfcnt.nvcsw;
}

//tasks
template<>
typename SensingTypeInfo<SEN_NVCSW>::ValType
SensingInterface::sense<SEN_NVCSW,tracked_task_data_t>(const tracked_task_data_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swCurrData(wid).tasks[rsc->task_idx].perfcnt.nvcsw;
}
template<>
typename SensingTypeInfo<SEN_NVCSW>::ValType
SensingInterface::senseAgg<SEN_NVCSW,tracked_task_data_t>(const tracked_task_data_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double)data.swAggrData(wid).tasks[rsc->task_idx].perfcnt.nvcsw;
}


/*
 * SEN_BEATS for cores and tasks
 */
//cores
template<>
typename SensingTypeInfo<SEN_BEATS>::ValType
SensingInterface::sense<SEN_BEATS,core_info_t>(SensingTypeInfo<SEN_BEATS>::ParamType p, const core_info_t *rsc, int wid)
{
    static_assert(std::is_same<decltype(p), unsigned int>::value, "p must be unsigned int");
    const PerformanceData &data = SensingModule::get().data();
    return data.swCurrData(wid).cpus[rsc->position].beats[p];
}
template<>
typename SensingTypeInfo<SEN_BEATS>::ValType
SensingInterface::senseAgg<SEN_BEATS,core_info_t>(SensingTypeInfo<SEN_BEATS>::ParamType p, const core_info_t *rsc, int wid)
{
    static_assert(std::is_same<decltype(p), unsigned int>::value, "p must be unsigned int");
    const PerformanceData &data = SensingModule::get().data();
    return data.swAggrData(wid).cpus[rsc->position].beats[p];
}

//tasks
template<>
typename SensingTypeInfo<SEN_BEATS>::ValType
SensingInterface::sense<SEN_BEATS,tracked_task_data_t>(SensingTypeInfo<SEN_BEATS>::ParamType p, const tracked_task_data_t *rsc, int wid)
{
    static_assert(std::is_same<decltype(p), unsigned int>::value, "p must be unsigned int");
    const PerformanceData &data = SensingModule::get().data();
    return data.swCurrData(wid).tasks[rsc->task_idx].beats[p];
}
template<>
typename SensingTypeInfo<SEN_BEATS>::ValType
SensingInterface::senseAgg<SEN_BEATS,tracked_task_data_t>(SensingTypeInfo<SEN_BEATS>::ParamType p, const tracked_task_data_t *rsc, int wid)
{
    static_assert(std::is_same<decltype(p), unsigned int>::value, "p must be unsigned int");
    const PerformanceData &data = SensingModule::get().data();
    return data.swAggrData(wid).tasks[rsc->task_idx].beats[p];
}


/*
 * SEN_FREQ_MHZ for frequency domains
 */
//cores
template<>
typename SensingTypeInfo<SEN_FREQ_MHZ>::ValType
SensingInterface::sense<SEN_FREQ_MHZ,freq_domain_info_t>(const freq_domain_info_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double) data.swCurrData(wid).freq_domains[rsc->domain_id].avg_freq_mhz_acc /
           (double) data.swCurrData(wid).freq_domains[rsc->domain_id].time_ms_acc;
}
template<>
typename SensingTypeInfo<SEN_FREQ_MHZ>::ValType
SensingInterface::senseAgg<SEN_FREQ_MHZ,freq_domain_info_t>(const freq_domain_info_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return (double) data.swAggrData(wid).freq_domains[rsc->domain_id].avg_freq_mhz_acc /
           (double) data.swAggrData(wid).freq_domains[rsc->domain_id].time_ms_acc;
}


/*
 * SEN_LASTCPU for tasks
 */
template<>
typename SensingTypeInfo<SEN_LASTCPU>::ValType
SensingInterface::sense<SEN_LASTCPU,tracked_task_data_t>(const tracked_task_data_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return data.swCurrData(wid).tasks[rsc->task_idx].last_cpu_used;
}
template<>
typename SensingTypeInfo<SEN_LASTCPU>::ValType
SensingInterface::senseAgg<SEN_LASTCPU,tracked_task_data_t>(const tracked_task_data_t *rsc, int wid)
{
    const PerformanceData &data = SensingModule::get().data();
    return data.swAggrData(wid).tasks[rsc->task_idx].last_cpu_used;
}

