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

#ifndef __core_vanilla_h
#define __core_vanilla_h

#include "base/base.h"

void vitamins_vanilla_map(model_sys_t *sys);
void vitamins_vanilla_shared_map(model_sys_t *sys);
void vitamins_vanilla_shared_agingaware_map(model_sys_t *sys);

void vitamins_vanilla_load_balance(model_task_t **task_list, int task_list_size, model_core_t *core_list, bool agingaware);

#endif
