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

#ifndef __core_optimal_h
#define __core_optimal_h

#include "base/base.h"

void vitamins_optimal_map(model_sys_t *sys);
void vitamins_optimal_shared_map(model_sys_t *sys);
void vitamins_optimalIPS_map(model_sys_t *sys);
void vitamins_optimalIPS_shared_map(model_sys_t *sys);
void vitamins_optimal_shared_freq_map(model_sys_t *sys);
void vitamins_optimal_sparta_map(model_sys_t *sys);
void vitamins_optimal_sparta_shared_map(model_sys_t *sys);
void vitamins_optimal_energy_given_ips(model_sys_t *sys, uint32_t ips_tgt);


#endif
