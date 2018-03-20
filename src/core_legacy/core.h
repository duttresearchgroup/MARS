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

#ifndef __core_core_h
#define __core_core_h

#warning core_legacy files are deprecated. Avoid them !

//Single header that just includes all other headers

#include <base/base.h>

#include "bin_predictor.h"
#include "cfs.h"
#include "dvfs.h"
#include "power.h"
#include "map.h"
#include "power_model.h"
#include "helpers.h"
#include "sys_init.h"

#include "map_initial.h"
#include "map_sparta.h"
#include "map_gts.h"
#include "map_mts.h"
#include "map_optimal.h"
#include "map_vanilla.h"
#include "map_sasolver.h"
#include "map_roundrobin.h"

#include "idlepower.h"
#include "random.h"



#endif
