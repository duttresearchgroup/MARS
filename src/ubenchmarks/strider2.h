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

//Stuff copied from module source code

#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdexcept>
#include <system_error>
#include <sys/time.h>

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


struct ModuleIF
{
	int _module_file_if;
	void* cached_mem_ptr;
	void* uncached_cohr_mem_ptr;
	//void* uncached_noncohr_mem_ptr;

	ModuleIF()
	:_module_file_if(0), cached_mem_ptr(nullptr),uncached_cohr_mem_ptr(nullptr)//,uncached_noncohr_mem_ptr(nullptr)
{
	_module_file_if = open(MODULE_SYSFS_PATH, O_RDWR);
    if(_module_file_if < 0)
    	printf("Vitamins module %s not inserted errno=%d\n",MODULE_SYSFS_PATH,errno);

    if(ioctl(_module_file_if, IOCTLCMD_CACHEDALLOC,0) !=0)
    	printf("IOCTLCMD_CACHEDALLOC failed errno=%d\n",errno);
    cached_mem_ptr = mmap(NULL, SHARED_DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, _module_file_if, 0);
    if(cached_mem_ptr == MAP_FAILED)
    	printf("mmap error\n");

    if(ioctl(_module_file_if, IOCTLCMD_UNCACHEDALLOC_COHERENT,0) !=0)
    	printf("IOCTLCMD_UNCACHEDALLOC_COHERENT failed errno=%d\n",errno);
    uncached_cohr_mem_ptr = mmap(NULL, SHARED_DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, _module_file_if, 0);
    if(uncached_cohr_mem_ptr == MAP_FAILED)
    	printf("mmap error\n");

    //if(ioctl(_module_file_if, IOCTLCMD_UNCACHEDALLOC_NONCOHERENT,0) !=0)
    // 	printf("IOCTLCMD_UNCACHEDALLOC_NONCOHERENT failed errno=%d\n",errno);
    //uncached_noncohr_mem_ptr = mmap(NULL, SHARED_DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, _module_file_if, 0);
    //if(uncached_noncohr_mem_ptr == MAP_FAILED)
    // 	printf("mmap error\n");
}

	~ModuleIF()
{
	if(munmap(cached_mem_ptr,SHARED_DATA_SIZE) < 0)
    	printf("munmap failed with errno=%d!\n",errno);

	if(munmap(uncached_cohr_mem_ptr,SHARED_DATA_SIZE) < 0)
    	printf("munmap failed with errno=%d!\n",errno);

	//if(munmap(uncached_noncohr_mem_ptr,SHARED_DATA_SIZE) < 0)
    //	printf("munmap failed with errno=%d!\n",errno);

    if(close(_module_file_if) < 0)
    	printf("close failed with errno=%d!\n",errno);
}

};


#endif
