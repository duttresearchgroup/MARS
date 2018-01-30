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

#ifndef __arm_rt_user_if
#define __arm_rt_user_if

//Sets up a debugfs file so user processes can interact with this guy

#include "../common/user_if_shared.h"
#include "../linux-module/core.h"

bool create_user_if(void);
void destroy_user_if(void);

//called by the sensing part to notify users that a sensing window is done
void sensing_window_ready(int wid);

#endif

