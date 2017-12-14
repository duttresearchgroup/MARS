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

