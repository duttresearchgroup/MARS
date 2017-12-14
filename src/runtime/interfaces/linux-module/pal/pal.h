#ifndef __arm_rt_pal_h
#define __arm_rt_pal_h

#include "../../common/pal/pal_setup.h"

/////////////////////////////////////////
//platform specific functions

//start/stop sensing power on the spaeicifed domain
//end stops sensing and returns power in uW
void power_sense_start(power_domain_info_t *domain);
uint32_t power_sense_end(power_domain_info_t *domain);

//start/stop perf sense on the current cpu. Despite taking the cpu as argument, the value should always be == the current cpu
void start_perf_sense(int cpu);
void stop_perf_sense(int cpu);

//Call once during plat setup to set which counters to be enabled when start_perf_sense is called.
//This should be called before enabling the hooks.
//Only the vit_available_perfcnt func should call this
void plat_enable_perfcnt(perfcnt_t perfcnt);
void plat_reset_perfcnts(void);//undo any call to enable

//returns the current num of enabled perf. counts and the maximum
int plat_enabled_perfcnts(void);
int plat_max_enabled_perfcnts(void);

//read counters from the current cpu. Despite taking the cpu as argument, the value should always be == the current cpu
uint32_t read_perfcnt(int cpu,perfcnt_t perfcnt);



#endif

