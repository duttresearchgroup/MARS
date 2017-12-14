#ifndef __arm_rt_user_if_ioctl
#define __arm_rt_user_if_ioctl

int ioctlcmd_sense_window_wait_any(void);
int ioctlcmd_sense_window_create(int period_ms);
int ioctlcmd_sensing_start(void);
int ioctlcmd_sensing_stop(void);
int ioctlcmd_enable_pertask_sensing(int enable);
int ioctlcmd_enable_pintask(int cpu);
int ioctlcmd_perfcnt_enable(int perfcnt);
int ioctlcmd_perfcnt_reset(void);
int ioctlcmd_task_beat_updated(pid_t task_pid);


#else
#error "This guy should be included at user_if.c and user_if_ioctl.c only"
#endif




