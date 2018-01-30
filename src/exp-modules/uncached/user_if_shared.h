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

#define MODULE_SYSFS_PATH "/sys/kernel/debug/uncached"

#define SHARED_DATA_SIZE (sizeof(char)*1024*1024*2)

typedef enum {
	_IOCTLCMD_CACHEDALLOC=0,
	_IOCTLCMD_UNCACHEDALLOC_COHERENT,
	_IOCTLCMD_UNCACHEDALLOC_NONCOHERENT,
	SIZE_IOCTLCMD
} user_if_ioctl_cmds_t;

#define IOCTLCMD_CACHEDALLOC				_IOR('v', _IOCTLCMD_CACHEDALLOC, uint32_t)
#define IOCTLCMD_UNCACHEDALLOC_COHERENT		_IOR('v', _IOCTLCMD_UNCACHEDALLOC_COHERENT, uint32_t)
#define IOCTLCMD_UNCACHEDALLOC_NONCOHERENT	_IOR('v', _IOCTLCMD_UNCACHEDALLOC_NONCOHERENT, uint32_t)

#endif

