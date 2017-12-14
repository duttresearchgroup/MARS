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
