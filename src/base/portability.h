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

#ifndef __core_base_portability_h
#define __core_base_portability_h

//kernel-level C / daemon-level C++ / app-level
//C++ portability definitions

#ifndef __KERNEL__
    #include <stdint.h>
	#include <limits.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <assert.h>
	#ifdef HASDEBUG
		//we always flush since when running as daemon stdout is sent to the ring buffer
		#define pdebug(...) do{ printf("VDEMON_D " __VA_ARGS__); fflush(stdout); }while(0)
    #else
        #define pdebug(...) do{}while(0)
    #endif
	//we always flush since when running as daemon stdout is sent to the ring buffer
	#define pinfo(...) do{ printf("VDEMON_I " __VA_ARGS__); fflush(stdout); }while(0)
#endif

#ifndef __cplusplus
    #ifndef __KERNEL__
        enum {
            false = 0,
            true = 1
        };
        typedef _Bool bool;
    #endif

    #define CBEGIN
    #define CEND
	#define nullptr 0
#else
    #define CBEGIN extern "C" {
    #define CEND }
#endif

#ifdef __KERNEL__
    #include <linux/types.h>
    #include <linux/slab.h>
	#include <linux/fs.h>
    #ifdef HASDEBUG
        #define pdebug(...) printk(KERN_INFO"VSENSE_D " __VA_ARGS__)
    #else
        #define pdebug(...) do{}while(0)
    #endif
	#define pinfo(...) printk(KERN_INFO"VSENSE_I " __VA_ARGS__)
#endif


#endif
