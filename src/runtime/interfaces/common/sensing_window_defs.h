#ifndef __arm_rt_sensing_window_defs_h
#define __arm_rt_sensing_window_defs_h


//Minimum lenght of a sensing window
//This is also the period used internally in the sensing module to sample
//sensors/counters that are not sampled at task context switch granularity
#define MINIMUM_WINDOW_LENGHT_MS 5
#define MINIMUM_WINDOW_LENGHT_JIFFIES ((MINIMUM_WINDOW_LENGHT_MS*CONFIG_HZ)/1000)

#define MAX_WINDOW_CNT 4

//SPECIAL codes that may be returned instead of a window id
#define WINDOW_ID_MASK 0xFAB00000

//Returned when the sensing module is exiting
#define WINDOW_EXIT ((int)(WINDOW_ID_MASK | 1))
//Invalid period when creating a window
#define WINDOW_INVALID_PERIOD ((int)(WINDOW_ID_MASK | 2))
//Max number of windows were created
#define WINDOW_MAX_NWINDOW ((int)(WINDOW_ID_MASK | 3))
//WIndow with the same period exists
#define WINDOW_EXISTS ((int)(WINDOW_ID_MASK | 4))

#endif
