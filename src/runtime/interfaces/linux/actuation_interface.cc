/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * Copyright (C) 2018 Bryan Donyanavard <bdonyana@uci.edu>
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

#include <unordered_map>

#include <runtime/framework/types.h>
#include <runtime/framework/actuation_interface.h>
#include <runtime/interfaces/sensing_module.h>

#include "cpufreq.h"
#include "idledomain.h"

class LinuxFrequencyActuator {

    // Should we get this from cpufreq ???
    static const int FREQ_STEPS_MHZ = 100;

private:
    // Interface to cpufreq module
    CpuFreq _cpufreq;

    // Maximum/minimum/current frequencies per domain
    // This "buffers" the values read from the files
    // and are only checked once at the beggining and if
    // we change them. If someone other program is concurrently
    // messing up with cpufreq, then this will become inconsistent
    ActuationTypeInfo<ACT_FREQ_MHZ>::Ranges *_ranges;
    int *_freq_curr_mhz;//this is only buffered after we change the freq the first time
    bool *_freq_curr_valid;
    bool _manual_mode;//if the governor is currently set to user-space or some other governor

    void _reset_ranges()
    {
        for(int i = 0; i < _cpufreq.sys_info().freq_domain_list_size; ++i){
            _ranges[i].max = _cpufreq.scaling_max_freq(&_cpufreq.sys_info().freq_domain_list[i]);
            _ranges[i].min = _cpufreq.scaling_min_freq(&_cpufreq.sys_info().freq_domain_list[i]);
            _ranges[i].steps = FREQ_STEPS_MHZ;
            _freq_curr_valid[i] = false;
        }
    }

public:
    LinuxFrequencyActuator(const sys_info_t &_info)
        :_cpufreq(_info),
         _ranges(new ActuationTypeInfo<ACT_FREQ_MHZ>::Ranges[_info.freq_domain_list_size]),
         _freq_curr_mhz(new int[_info.freq_domain_list_size]),
         _freq_curr_valid(new bool[_info.freq_domain_list_size]),
         _manual_mode(false)
    {
        _reset_ranges();
    }

    ~LinuxFrequencyActuator()
    {
        delete[] _freq_curr_mhz;
        delete[] _freq_curr_valid;
        delete[] _ranges;
    }

    void doSysActuation(const freq_domain_info_t *rsc, int val_mhz){
        if(!_manual_mode){
            _cpufreq.scaling_governor("userspace");
            _manual_mode = true;
            _reset_ranges();
        }

        _cpufreq.scaling_setspeed(rsc,val_mhz);
        _freq_curr_mhz[rsc->domain_id] = _cpufreq.scaling_cur_freq(rsc);
        _freq_curr_valid[rsc->domain_id] = true;
        if((_freq_curr_mhz[rsc->domain_id] > (val_mhz+3)) || (_freq_curr_mhz[rsc->domain_id] < (val_mhz-3)))
            pinfo("WARNING: tried to set domain %d freq to %d mhz, actual set value was %d mhz\n",
                    rsc->domain_id,val_mhz,_freq_curr_mhz[rsc->domain_id]);
    }
    int getSysActuation(const freq_domain_info_t *rsc){
        if(_freq_curr_valid[rsc->domain_id])
            return _freq_curr_mhz[rsc->domain_id];
        else
            return _cpufreq.scaling_cur_freq(rsc);
    }

    void setGovernor(const std::string &arg)
    {
        _cpufreq.scaling_governor(arg);
        _manual_mode = false;
        _reset_ranges();
    }

    std::string getGovernor(const freq_domain_info_t *rsc)
    {
        return _cpufreq.scaling_governor(rsc);
    }

    const ActuationTypeInfo<ACT_FREQ_MHZ>::Ranges& ranges(const freq_domain_info_t* rsc) { return _ranges[rsc->domain_id];}

    void ranges(const freq_domain_info_t* rsc, const ActuationTypeInfo<ACT_FREQ_MHZ>::Ranges &newRange)
    {
        _cpufreq.scaling_max_freq(rsc,newRange.max);
        _ranges[rsc->domain_id].max = _cpufreq.scaling_max_freq(rsc);
        if((_ranges[rsc->domain_id].max > (newRange.max+5)) || (_ranges[rsc->domain_id].max < (newRange.max-5)))
            pinfo("WARNING: tried to set domain %d max freq to %d mhz, actual set value was %d mhz\n",
                    rsc->domain_id,newRange.max,_ranges[rsc->domain_id].max);

        _cpufreq.scaling_min_freq(rsc,newRange.min);
        _ranges[rsc->domain_id].min = _cpufreq.scaling_min_freq(rsc);
        if((_ranges[rsc->domain_id].min > (newRange.min+5)) || (_ranges[rsc->domain_id].min < (newRange.min-5)))
            pinfo("WARNING: tried to set domain %d min freq to %d mhz, actual set value was %d mhz\n",
                    rsc->domain_id,newRange.min,_ranges[rsc->domain_id].min);

        _ranges[rsc->domain_id].steps = newRange.steps;
    }
};


class LinuxIdleDomainActuator {

private:
    IdleDomain **_idleDomain;
    ActuationTypeInfo<ACT_ACTIVE_CORES>::Ranges *_ranges;


public:

    const sys_info_t &info;

    LinuxIdleDomainActuator(const sys_info_t &_info)
        :_idleDomain(nullptr),_ranges(nullptr),info(_info)
    {
        _idleDomain = new IdleDomain*[info.freq_domain_list_size];
        _ranges = new ActuationTypeInfo<ACT_ACTIVE_CORES>::Ranges[info.freq_domain_list_size];
        for(int i = 0; i < _info.freq_domain_list_size; ++i){
            assert_true(info.freq_domain_list[i].domain_id == i);
            _idleDomain[i] = new IdleDomain(&info,info.freq_domain_list[i]);
            _ranges[i].min = 1;
            _ranges[i].max = info.freq_domain_list[i].core_cnt;
        }
    }
    ~LinuxIdleDomainActuator()
    {
        for(int i = 0; i < info.freq_domain_list_size; ++i){
            delete _idleDomain[i];
        }
        delete[] _idleDomain;
        delete[] _ranges;
    }

    void doSysActuation(const freq_domain_info_t *rsc, int cores){
        _idleDomain[rsc->domain_id]->idleCores(cores,PerformanceData::localData());
    }
    int getSysActuation(const freq_domain_info_t *rsc){
        return _idleDomain[rsc->domain_id]->idleCores();
    }

    const ActuationTypeInfo<ACT_ACTIVE_CORES>::Ranges& range(const freq_domain_info_t *rsc){
        return _ranges[rsc->domain_id];
    }


};


class LinuxTaskMapActuator {

private:

    const sys_info_t &_info;
    ActuationTypeInfo<ACT_TASK_MAP>::Ranges _ranges;
    std::unordered_map<const tracked_task_data_t*,core_info_t*> _tasks;

public:

    LinuxTaskMapActuator(const sys_info_t &info)
        :_info(info)
    {
        _ranges.min = 0;
        _ranges.max = info.core_list_size - 1;
    }

    void doSysActuation(const tracked_task_data_t *task, core_info_t *core){
        cpu_set_t set;
        CPU_ZERO( &set );
        CPU_SET(core->position, &set );
        sched_setaffinity(task->this_task_pid, sizeof( cpu_set_t ), &set);
        _tasks[task] = core;
    }
    core_info_t* getSysActuation(const tracked_task_data_t *task){
        auto i = _tasks.find(task);
        if(i == _tasks.end()){
            pinfo("WARNING: ACT_TASK_MAP for task %d never set. Using sense<SEN_LASTCPU>\n",(int)task->this_task_pid);
            int cpu = SensingInterface::sense<SEN_LASTCPU>(task,0);
            assert_true((cpu >= 0) && (cpu < _info.core_list_size));
            return &(_info.core_list[cpu]);
        }
        else
            return i->second;
    }

    const ActuationTypeInfo<ACT_TASK_MAP>::Ranges& range(const tracked_task_data_t *rsc){
        return _ranges;
    }
};

static LinuxFrequencyActuator *freqAct = nullptr;
static LinuxIdleDomainActuator *idleDmAct = nullptr;
static LinuxTaskMapActuator *tMapAct = nullptr;

void ActuationInterface::construct(const sys_info_t &info)
{
    assert_true(freqAct == nullptr);
    assert_true(idleDmAct == nullptr);
    assert_true(tMapAct == nullptr);
#ifdef LINUX_HAS_CPUFREQ
    freqAct = new LinuxFrequencyActuator(info);
#endif
    idleDmAct = new LinuxIdleDomainActuator(info);
    tMapAct = new LinuxTaskMapActuator(info);
}

void ActuationInterface::destruct()
{
#ifdef LINUX_HAS_CPUFREQ
	assert_true(freqAct != nullptr);
	delete freqAct;
#endif
    assert_true(idleDmAct != nullptr);
    assert_true(tMapAct != nullptr);
    delete idleDmAct;
    delete tMapAct;
}

////////////////////////////////////////////////
// Actuation interface for ACT_FREQ_MHZ

template<>
void
ActuationInterfaceImpl::Impl::actuate<ACT_FREQ_MHZ,freq_domain_info_t>(
        const freq_domain_info_t *rsc,
        typename ActuationTypeInfo<ACT_FREQ_MHZ>::ValType val)
{
    assert_true(freqAct != nullptr);
    freqAct->doSysActuation(rsc,val);
}

template<>
typename ActuationTypeInfo<ACT_FREQ_MHZ>::ValType
ActuationInterfaceImpl::Impl::actuationVal<ACT_FREQ_MHZ,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    assert_true(freqAct != nullptr);
    return freqAct->getSysActuation(rsc);
}

template<>
const typename ActuationTypeInfo<ACT_FREQ_MHZ>::Ranges&
ActuationInterfaceImpl::Impl::actuationRanges<ACT_FREQ_MHZ,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    assert_true(freqAct != nullptr);
    return freqAct->ranges(rsc);
}

template<>
void
ActuationInterfaceImpl::Impl::actuationRanges<ACT_FREQ_MHZ,freq_domain_info_t>(
        const freq_domain_info_t *rsc,
        const typename ActuationTypeInfo<ACT_FREQ_MHZ>::Ranges &newRange)
{
    assert_true(freqAct != nullptr);
    freqAct->ranges(rsc,newRange);
}


////////////////////////////////////////////////
// Actuation interface for ACT_FREQ_GOV

template<>
void
ActuationInterfaceImpl::Impl::actuate<ACT_FREQ_GOV,freq_domain_info_t>(
        const freq_domain_info_t *rsc,
        typename ActuationTypeInfo<ACT_FREQ_GOV>::ValType val)
{
    assert_true(freqAct != nullptr);
    freqAct->setGovernor(val);
}

template<>
typename ActuationTypeInfo<ACT_FREQ_GOV>::ValType
ActuationInterfaceImpl::Impl::actuationVal<ACT_FREQ_GOV,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    assert_true(freqAct != nullptr);
    return freqAct->getGovernor(rsc);
}

template<>
const typename ActuationTypeInfo<ACT_FREQ_GOV>::Ranges&
ActuationInterfaceImpl::Impl::actuationRanges<ACT_FREQ_GOV,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    assert_true(freqAct != nullptr);
    return freqAct->ranges(rsc);
}

template<>
void
ActuationInterfaceImpl::Impl::actuationRanges<ACT_FREQ_GOV,freq_domain_info_t>(
        const freq_domain_info_t *rsc,
        const typename ActuationTypeInfo<ACT_FREQ_GOV>::Ranges &newRange)
{
    assert_true(freqAct != nullptr);
    freqAct->ranges(rsc,newRange);
}

////////////////////////////////////////////////
// Actuation interface for ACT_ACTIVE_CORES

template<>
void
ActuationInterfaceImpl::Impl::actuate<ACT_ACTIVE_CORES,freq_domain_info_t>(
        const freq_domain_info_t *rsc,
        typename ActuationTypeInfo<ACT_ACTIVE_CORES>::ValType val)
{
    assert_true(idleDmAct != nullptr);
    idleDmAct->doSysActuation(rsc,val);
}

template<>
typename ActuationTypeInfo<ACT_ACTIVE_CORES>::ValType
ActuationInterfaceImpl::Impl::actuationVal<ACT_ACTIVE_CORES,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    assert_true(idleDmAct != nullptr);
    return idleDmAct->getSysActuation(rsc);
}

template<>
const typename ActuationTypeInfo<ACT_ACTIVE_CORES>::Ranges&
ActuationInterfaceImpl::Impl::actuationRanges<ACT_ACTIVE_CORES,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    assert_true(idleDmAct != nullptr);
    return idleDmAct->range(rsc);
}

// These functions doesn't make sense for this case so are left undefined
//
// template<ActuationType ACT_T,typename ResourceT>
// static void actuationRanges(ResourceT *rsc, const typename ActuationTypeInfo<ACT_T>::Ranges &new_range);

////////////////////////////////////////////////
// Actuation interface for ACT_TASK_MAP

template<>
void
ActuationInterfaceImpl::Impl::actuate<ACT_TASK_MAP,tracked_task_data_t>(
        const tracked_task_data_t *rsc,
        typename ActuationTypeInfo<ACT_TASK_MAP>::ValType val)
{
    assert_true(tMapAct != nullptr);
    tMapAct->doSysActuation(rsc,val);
}

template<>
typename ActuationTypeInfo<ACT_TASK_MAP>::ValType
ActuationInterfaceImpl::Impl::actuationVal<ACT_TASK_MAP,tracked_task_data_t>(const tracked_task_data_t *rsc)
{
    assert_true(tMapAct != nullptr);
    return tMapAct->getSysActuation(rsc);
}

template<>
const typename ActuationTypeInfo<ACT_TASK_MAP>::Ranges&
ActuationInterfaceImpl::Impl::actuationRanges<ACT_TASK_MAP,tracked_task_data_t>(const tracked_task_data_t *rsc)
{
    assert_true(tMapAct != nullptr);
    return tMapAct->range(rsc);
}


// These functions doesn't make sense for this case so are left undefined
//
// template<ActuationType ACT_T,typename ResourceT>
// static void actuationRanges(ResourceT *rsc, const typename ActuationTypeInfo<ACT_T>::Ranges &new_range);



