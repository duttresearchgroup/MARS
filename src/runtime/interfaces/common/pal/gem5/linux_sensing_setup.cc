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

#include <runtime/interfaces/linux/common/pal/sensing_setup.h>
#include <runtime/interfaces/sensing_module.h>
#include <runtime/framework/sensing_interface.h>

template<>
void pal_sensing_setup<SensingModule>(SensingModule *m){
    // no sensors implemented yet
}

template<>
void pal_sensing_teardown<SensingModule>(SensingModule *m){
    // no sensors implemented yet
}

/*
 * Sensing interface functions implementation
 */

template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
SensingInterfaceImpl::Impl::sense<SEN_POWER_W,power_domain_info_t>(const power_domain_info_t *rsc, int wid)
{
	return 0;
}

template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
SensingInterfaceImpl::Impl::senseAgg<SEN_POWER_W,power_domain_info_t>(const power_domain_info_t *rsc, int wid)
{
	return 0;
}

// We need this specialization only to support the implementation of
// BinFuncImpl<power>
// However this fuction should never be actually called
template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
SensingInterfaceImpl::Impl::sense<SEN_POWER_W,tracked_task_data_t>(const tracked_task_data_t *rsc, int wid)
{
    arm_throw(SensingException,"This function should never be called");
    return 0;
}

// We need this specialization only to allow the reflective code to compile
// However this fuction should never be actually called
template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
SensingInterfaceImpl::Impl::sense<SEN_POWER_W,freq_domain_info_t>(const freq_domain_info_t *rsc, int wid)
{
    arm_throw(SensingException,"This function should never be called");
    return 0;
}
template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
SensingInterfaceImpl::Impl::senseAgg<SEN_POWER_W,freq_domain_info_t>(const freq_domain_info_t *rsc, int wid)
{
    arm_throw(SensingException,"This function should never be called");
    return 0;
}

template <>
typename SensingTypeInfo<SEN_NV_GPU_PERFCNT>::ValType
SensingInterfaceImpl::Impl::sense<SEN_NV_GPU_PERFCNT, NullResource>(typename SensingTypeInfo<SEN_NV_GPU_PERFCNT>::ParamType p, const NullResource *rsc, int wid)
{
        arm_throw(SensingException,"sense<SEN_NV_GPU_PERFCNT, NullResource>: This function should never be called");
	    return 0;
}


template<>
typename SensingTypeInfo<SEN_NV_GPU_PERFCNT>::ValType
SensingInterfaceImpl::Impl::senseAgg<SEN_NV_GPU_PERFCNT, NullResource>(typename SensingTypeInfo<SEN_NV_GPU_PERFCNT>::ParamType p, const NullResource *rsc, int wid)
{
        arm_throw(SensingException,"senseAgg<SEN_NV_GPU_PERFCNT, NullResource>: This function should never be called");
	    return 0;
}

template <>
int
SensingInterfaceImpl::Impl::enableSensor<SEN_NV_GPU_PERFCNT, NullResource>(typename SensingTypeInfo<SEN_NV_GPU_PERFCNT>::ParamType p, const NullResource *rsc, bool enable)
{
        arm_throw(SensingException,"enableSensor<SEN_NV_GPU_PERFCNT, NullResource>: This function should never be called");
	    return 0;
}
