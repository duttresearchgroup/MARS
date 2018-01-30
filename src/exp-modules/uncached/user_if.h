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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/fs.h>

#ifdef HASDEBUG
	#define pdebug(...) printk(KERN_INFO"VSENSE_D " __VA_ARGS__)
#else
	#define pdebug(...) do{}while(0)
#endif
#define pinfo(...) printk(KERN_INFO"UNCACHED " __VA_ARGS__)

//Sets up a debugfs file so user processes can interact with this guy

#include "user_if_shared.h"

bool create_user_if(void);
void destroy_user_if(void);

#endif

