#ifndef __arm_rt_sense_h
#define __arm_rt_sense_h

#include "../linux-module/core.h"
#include "../common/perfcnts.h"
#include "../linux-module/sense_data.h"

//Sensing functions definitions

bool sense_create_mechanisms(sys_info_t *sys);
void sense_init(sys_info_t *sys);
void sense_begin(sys_info_t *sys);
void sense_stop(sys_info_t *sys);
void sense_cleanup(sys_info_t *sys);
void sense_destroy_mechanisms(sys_info_t *sys);

//used to check the current frequency (could also be used to set a frequency)
void minimum_sensing_window(sys_info_t *sys);
//sums-up the sensed information of currently active tasks; sets task_list_size
void sense_window(sys_info_t *sys, int wid);

//sensing options
void set_per_task_sensing(bool val);
void set_pin_task_to_cpu(int cpu);


bool trace_perf_counter(perfcnt_t pc);
bool trace_perf_counter_reset(void);

private_hook_data_t* add_created_task(struct task_struct *tsk);

//copy task beat info from the internal struct accecible by the task to the sensed data struct
bool update_task_beat_info(pid_t pid);

#endif

