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

