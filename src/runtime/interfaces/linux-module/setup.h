#ifndef __arm_rt_setup_h

#define __arm_rt_setup_h

#include <linux/kernel.h>

#include "../linux-module/core.h"

//setups cores and initialize the idle power consumption data structures
void init_system_info(void);
sys_info_t* system_info(void);


#endif

