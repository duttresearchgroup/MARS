#ifndef __arm_rt_helpers_h
#define __arm_rt_helpers_h

#include <linux/kernel.h>
#include <linux/sched.h>

#include <core/core.h>
#include <runtime/interfaces/common/sense_data_shared.h>

bool actuate_change_cpu(tracked_task_data_t *kern_tsk, int next_cpu);
bool actuate_change_cpu(uint32_t pid, int next_cpu);

#endif

