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

#ifndef __arm_rt_framework_sensing_interface_h
#define __arm_rt_framework_sensing_interface_h

#include <type_traits>

#include "types.h"
#include <core/core.h>


class SensingInterface {

  public:

	/*
	 * Returns the sensed value for given window
	 */
	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(const ResourceT *rsc, int wid);

	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType sense(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc, int wid);


	/*
	 * Returns aggregated sensed value for the
	 * current and previous instances of the given window
	 */
	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType senseAgg(const ResourceT *rsc, int wid);

	template<SensingType SEN_T,typename ResourceT>
	static typename SensingTypeInfo<SEN_T>::ValType senseAgg(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc, int wid);
};

#endif /* ACTUATOR_H_ */
