/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef __arm_rt_user_if_ioctl
#define __arm_rt_user_if_ioctl

int ioctlcmd_sense_window_wait_any(void);
int ioctlcmd_sense_window_create(int period_ms);
int ioctlcmd_sensing_start(void);
int ioctlcmd_sensing_stop(void);
int ioctlcmd_enable_pertask_sensing(int enable);
int ioctlcmd_perfcnt_enable(int perfcnt);
int ioctlcmd_perfcnt_reset(void);
int ioctlcmd_task_beat_updated(pid_t task_pid);


#else
#error "This guy should be included at user_if.c and user_if_ioctl.c only"
#endif




