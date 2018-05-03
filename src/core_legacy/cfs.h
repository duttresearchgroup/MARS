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

#ifndef __core_cfs_h
#define __core_cfs_h

#include "base/base.h"

void vitamins_load_tracker_set(load_tracker_type_t t);
load_tracker_type_t vitamins_load_tracker_get(void);


uint32_t vitamins_predict_tlc(model_task_t* task,
                      uint32_t srcCoreIPS, uint32_t srcCoreUtil,
                      uint32_t tgtCoreIPS);

uint32_t vitamins_estimate_tlc(model_task_t* task);

//MUST be called whenever change the core mapping
void vitamins_load_tracker_map_changed(model_core_t *core);
uint32_t vitamins_load_tracker_task_load(model_task_t *task);
uint32_t vitamins_load_tracker_task_load_with_overhead(model_task_t *task);

static inline cfs_load_estimator_t* core_ptr_cfs(model_core_t *core){
    return &(core->load_tracking.cfs);
}
#define core_cfs(core) core_ptr_cfs(&(core))

#endif
