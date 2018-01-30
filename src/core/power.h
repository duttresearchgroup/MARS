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

#ifndef __core_power_domain_h
#define __core_power_domain_h

#include "base/base.h"

//static domain info
struct model_power_domain_struct {
	power_domain_info_t *info;

    power_domain_sensed_data_t sensed_data;

    pred_checker_power_domain_t pred_checker;
};
typedef struct model_power_domain_struct model_power_domain_t;

#endif
