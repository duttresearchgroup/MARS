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

#ifndef __arm_rt_user_shared_if
#define __arm_rt_user_shared_if

//Interface shared between this module and daemons

#define MODULE_SYSFS_PATH "/sys/kernel/debug/vitamins"

typedef enum {
	//blocks until a specific window is ready. Window ID is given as argument. Returns the window ID
	//NOTE: currently all there is only one thread in the daemon and module handling windows and they
	//are synchronous and handled in order, so only IOCTLCMD_SENSE_WINDOW_WAIT_ANY is supported
	_IOCTLCMD_SENSE_WINDOW_WAIT=0,

	//blocks until any window is ready. Returns the window ID
	_IOCTLCMD_SENSE_WINDOW_WAIT_ANY,

	//creates a sensing window. Period is given as argument. Returns the window ID
	//mmap should be used the map the window sensing data to user pages
	_IOCTLCMD_SENSE_WINDOW_CREATE,

	//destroys all sensing windows. Should stop sensing first
	//NOTE: currently all windows are deallocated when the module exits and reconfiguring windows is not supported
	_IOCTLCMD_SENSE_WINDOW_DESTROY_ALL,

	//starts sensing
	_IOCTLCMD_SENSING_START,

	//stops sensing
	_IOCTLCMD_SENSING_STOP,

	//pass >0 to enable pertask sensed data
	_IOCTLCMD_ENABLE_PERTASK_SENSING,

	//enables/disable tracing of a specific perfcnt
	//effective only before sensing
	_IOCTLCMD_PERFCNT_ENABLE,
	_IOCTLCMD_PERFCNT_RESET,

	//tells the sensing module that the calling task beat info was updated
	_IOCTLCMD_TASKBEAT_UPDATED,

	//registers/unregisters the calling process as the daemon process
	//only the daemon process can execute certain operations
	//need to send the magic word to seccesfully register
	_IOCTLCMD_REGISTER_DAEMON,
	_IOCTLCMD_UNREGISTER_DAEMON,

	SIZE_IOCTLCMD
} user_if_ioctl_cmds_t;

#define IOCTLCMD_SENSE_WINDOW_WAIT_ANY	_IOR('v', _IOCTLCMD_SENSE_WINDOW_WAIT_ANY, uint32_t)
#define IOCTLCMD_SENSE_WINDOW_CREATE	_IOR('v', _IOCTLCMD_SENSE_WINDOW_CREATE, uint32_t)
#define IOCTLCMD_SENSING_START			_IOR('v', _IOCTLCMD_SENSING_START, uint32_t)
#define IOCTLCMD_SENSING_STOP			_IOR('v', _IOCTLCMD_SENSING_STOP, uint32_t)
#define IOCTLCMD_ENABLE_PERTASK_SENSING	_IOR('v', _IOCTLCMD_ENABLE_PERTASK_SENSING, uint32_t)
#define IOCTLCMD_PERFCNT_ENABLE			_IOR('v', _IOCTLCMD_PERFCNT_ENABLE, uint32_t)
#define IOCTLCMD_PERFCNT_RESET			_IOR('v', _IOCTLCMD_PERFCNT_RESET, uint32_t)
#define IOCTLCMD_TASKBEAT_UPDATED		_IOR('v', _IOCTLCMD_TASKBEAT_UPDATED, uint32_t)
#define IOCTLCMD_REGISTER_DAEMON		_IOR('v', _IOCTLCMD_REGISTER_DAEMON, uint32_t)
#define IOCTLCMD_UNREGISTER_DAEMON		_IOR('v', _IOCTLCMD_UNREGISTER_DAEMON, uint32_t)

#endif

