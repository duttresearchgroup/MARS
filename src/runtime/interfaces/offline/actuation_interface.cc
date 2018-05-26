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
#include <type_traits>

#include <runtime/framework/types.h>
#include <runtime/framework/actuation_interface.h>
#include <runtime/interfaces/sensing_module.h>
#include <runtime/interfaces/offline/sensing_module.h>
#include <runtime/interfaces/offline/trace_simulator.h>

static_assert(std::is_same<SensingModule, OfflineSensingModule>::value, "Invalid stuff");

void ActuationInterface::construct(SensingModule *sm)
{
    assert_true(sm == OfflineSensingModule::localModule());
}

void ActuationInterface::destruct()
{

}


////////////////////////////////////////////////
// Actuation interface for ACT_TASK_MAP

template<>
void
ActuationInterfaceImpl::Impl::actuate<ACT_TASK_MAP,tracked_task_data_t>(
        const tracked_task_data_t *rsc,
        typename ActuationTypeInfo<ACT_TASK_MAP>::ValType val)
{
    OfflineSensingModule::localModule()->sim()->setTaskCore(rsc,val);
}

template<>
typename ActuationTypeInfo<ACT_TASK_MAP>::ValType
ActuationInterfaceImpl::Impl::actuationVal<ACT_TASK_MAP,tracked_task_data_t>(const tracked_task_data_t *rsc)
{
    return OfflineSensingModule::localModule()->sim()->getTaskCore(rsc);
}

template<>
const typename ActuationTypeInfo<ACT_TASK_MAP>::Ranges&
ActuationInterfaceImpl::Impl::actuationRanges<ACT_TASK_MAP,tracked_task_data_t>(const tracked_task_data_t *rsc)
{
    arm_throw(SensingException,"Function not implemented");
}


////////////////////////////////////////////////
// Actuation interface for ACT_FREQ_MHZ

template<>
void
ActuationInterfaceImpl::Impl::actuate<ACT_FREQ_MHZ,freq_domain_info_t>(
        const freq_domain_info_t *rsc,
        typename ActuationTypeInfo<ACT_FREQ_MHZ>::ValType val)
{
    OfflineSensingModule::localModule()->sim()->setDomainFreq(rsc,val);
}

template<>
typename ActuationTypeInfo<ACT_FREQ_MHZ>::ValType
ActuationInterfaceImpl::Impl::actuationVal<ACT_FREQ_MHZ,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    return OfflineSensingModule::localModule()->sim()->getDomainFreq(rsc);
}

template<>
const typename ActuationTypeInfo<ACT_FREQ_MHZ>::Ranges&
ActuationInterfaceImpl::Impl::actuationRanges<ACT_FREQ_MHZ,freq_domain_info_t>(const freq_domain_info_t *rsc)
{
    return OfflineSensingModule::localModule()->sim()->getDomainRanges(rsc);
}

template<>
void
ActuationInterfaceImpl::Impl::actuationRanges<ACT_FREQ_MHZ,freq_domain_info_t>(
        const freq_domain_info_t *rsc,
        const typename ActuationTypeInfo<ACT_FREQ_MHZ>::Ranges &newRange)
{
    arm_throw(SensingException,"Function not implemented");
}

