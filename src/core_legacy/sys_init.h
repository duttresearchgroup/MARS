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

#ifndef __core_sys_init_h
#define __core_sys_init_h

#include "base/base.h"
#include "dvfs.h"
#include "power.h"

void vitamins_task_init(model_task_t *task, int id);
void vitamins_sys_task_init(model_systask_t *task, model_core_t *core);

void vitamins_freq_domain_init(model_freq_domain_t *freq_domain, freq_domain_info_t *freq_domain_info, core_freq_t initial_freq);

void vitamins_power_domain_init(model_power_domain_t *power_domain, power_domain_info_t *power_domain_info);

void vitamins_core_init(model_core_t *core, core_info_t *core_info);

#endif
