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

#ifndef __arm_rt_actuator_interface_impl_h
#define __arm_rt_actuator_interface_impl_h

#include "types.h"

/*
 * These are the functions that need to be specialized to perform physical
 * actuations.
 *
 * We define them separately here (and not in actuation_interface.h) to break
 * the dependency between models required by the actuation_interface.h
 * reflective components and the ActuationInterfaceImpl required by some
 * models.
 *
 * Functions are protected to enforce care in it's use
 *
 */
class ActuationInterfaceImpl {

    friend class ReflectiveEngine;

  protected:

	struct Impl {
	    /*
	     * Sets a new actuation valute
	     */
	    template<ActuationType ACT_T,typename ResourceT>
	    static void actuate(const ResourceT *rsc, typename ActuationTypeInfo<ACT_T>::ValType val);

	    /*
	     * Gets the current actuation value.
	     */
	    template<ActuationType ACT_T,typename ResourceT>
	    static typename ActuationTypeInfo<ACT_T>::ValType actuationVal(const ResourceT *rsc);

	    /*
	     * Gets the actuation ranges
	     */
	    template<ActuationType ACT_T,typename ResourceT>
	    static const typename ActuationTypeInfo<ACT_T>::Ranges& actuationRanges(const ResourceT *rsc);

	    /*
	     * Tries to change the actuation ranges.
	     */
	    template<ActuationType ACT_T,typename ResourceT>
	    static void actuationRanges(const ResourceT *rsc, const typename ActuationTypeInfo<ACT_T>::Ranges &new_range);
	};
};




#endif
