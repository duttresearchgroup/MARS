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

#ifndef __core_base_base_h
#define __core_base_base_h

//Include this header to use base defs in both kernel,daemon, and applications
//This is the only header that is safe to include in both C/C++ code
//All other 'core' headers are C++ only

#include "portability.h"
#include "_exceptions.h"

CBEGIN

#include "_defs.h"
#include "_converters.h"
#include "_dvfs.h"
#include "_lists.h"
#include "_power.h"
#include "_scaling.h"
#include "_info_init.h"
#include "_fileio.h"

CEND

#endif
