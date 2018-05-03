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

#ifndef __arm_rt_actuator_interface_h
#define __arm_rt_actuator_interface_h

#include <queue>
#include <map>

#include "types.h"
#include "sensing_interface.h"
#include "actuation_interface_impl.h"
#include <base/base.h>


class ActuationInterface : ActuationInterfaceImpl {

    friend class ReflectiveEngine;

  public:

	/*
	 * Sets a new actuation valute
	 */
	template<ActuationType ACT_T,typename ResourceT>
	static void actuate(const ResourceT *rsc, typename ActuationTypeInfo<ACT_T>::ValType val)
	{
	    if(ReflectiveEngine::isReflecting())
	        tryActuate<ACT_T>(rsc,val);
	    else{
	        if(ReflectiveEngine::enabled())
	            ReflectiveEngine::get().actuate<ACT_T>(rsc,val);
	        Impl::actuate<ACT_T>(rsc,val);
	    }
	}

	/*
	 * Gets the current actuation value.
	 */
	template<ActuationType ACT_T,typename ResourceT>
	static typename ActuationTypeInfo<ACT_T>::ValType actuationVal(const ResourceT *rsc)
	{
	    if(ReflectiveEngine::isReflecting())
            return tryActuationVal<ACT_T>(rsc);
        else
            return Impl::actuationVal<ACT_T>(rsc);
	}

    /*
     * Gets the actuation ranges
     */
    template<ActuationType ACT_T,typename ResourceT>
    static const typename ActuationTypeInfo<ACT_T>::Ranges& actuationRanges(const ResourceT *rsc)
    { return Impl::actuationRanges<ACT_T>(rsc); }

    /*
     * Tries to change the actuation ranges.
     */
    template<ActuationType ACT_T,typename ResourceT>
    static void actuationRanges(const ResourceT *rsc, const typename ActuationTypeInfo<ACT_T>::Ranges &new_range)
    { Impl::actuationRanges<ACT_T>(rsc,new_range); }

    /*
     * One-time functions that need be called before using the interface
     * and after we are done using it to contruct/destruct object that may
     * be necessary for implementing the interface.
     */
    static void construct(const sys_info_t &info);
    static void destruct();


	/*
	 * Updates the predictive models with a new actuation value
	 */
	template<ActuationType ACT_T,typename ResourceT>
	static void tryActuate(const ResourceT *rsc, typename ActuationTypeInfo<ACT_T>::ValType val)
	{
	    //pinfo("tryActuate\n");
	    ReflectiveEngine::get().tryActuate<ACT_T>(rsc,val);
	}

	/*
	 * Gets the current actuatuation values from the predictive models
	 */
	template<ActuationType ACT_T,typename ResourceT>
	static typename ActuationTypeInfo<ACT_T>::ValType tryActuationVal(const ResourceT *rsc)
	{
	    if(ReflectiveEngine::get().hasNewActuationVal<ACT_T>(rsc))
	        return ReflectiveEngine::get().newActuationVal<ACT_T>(rsc);
	    else
	        return Impl::actuationVal<ACT_T>(rsc);
	}
};




#endif /* ACTUATOR_H_ */
