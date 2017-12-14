#ifndef __arm_rt_sensing_window_h
#define __arm_rt_sensing_window_h

#include "core.h"
#include "../common/sensing_window_defs.h"

bool create_queues(void);
void destroy_queues(void);

extern int sensing_window_cnt;
int create_sensing_window(int period_ms);

void start_sensing_windows(void);
void stop_sensing_windows(void);

bool sensing_running(void);

#endif

