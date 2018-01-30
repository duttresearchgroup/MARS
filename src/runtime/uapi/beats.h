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

#ifndef __arm_rt_uapi_beats_h
#define __arm_rt_uapi_beats_h

#include <sys/mman.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include <sys/time.h>

#include "../../core/base/portability.h"
#include "../interfaces/common/task_beat_data.h"
#include "../interfaces/common/user_if_shared.h"

typedef struct {
	task_beat_t *_data;
	void* _module_shared_mem_raw_ptr;
	int _module_file_if;
	pthread_mutex_t _mutex;
	long long _start_time;
} task_beat_info_t;


//let's go for a header only implementation

static inline long long _task_beat_current_timestamp_ms() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

static inline task_beat_info_t* task_beat_register_task()
{
	task_beat_info_t* info = (task_beat_info_t*)malloc(sizeof(task_beat_info_t));

	if(!info) return nullptr;

	info->_module_file_if = open(MODULE_SYSFS_PATH, O_RDWR);
	if(info->_module_file_if < 0) {
		free(info);
		return nullptr;
	}

	info->_module_shared_mem_raw_ptr = mmap(NULL, sizeof(task_beat_t), PROT_READ | PROT_WRITE, MAP_SHARED, info->_module_file_if, 0);
	if(info->_module_shared_mem_raw_ptr == MAP_FAILED){
		free(info);
		return nullptr;
	}

	info->_data = (task_beat_t*)(info->_module_shared_mem_raw_ptr);

	info->_start_time = _task_beat_current_timestamp_ms();

	pthread_mutex_init(&(info->_mutex),0);

	return info;
}


static inline task_beat_data_t* task_beat_create_domain(
		task_beat_info_t* info,//the struct returned when the task is registered
		task_beat_type type,//beat type
		uint64_t tgt_rate//the tgt beat rate in beats/us
		)
{
	if(info == nullptr) return nullptr;

	if(info->_data->num_beat_domains >= MAX_BEAT_DOMAINS) return nullptr;

	task_beat_data_t* data = &(info->_data->beat_data[info->_data->num_beat_domains]);
	data->__curr_beat_cnt = 0;
	data->tgt_rate = tgt_rate;
	data->type = type;
	info->_data->num_beat_domains += 1;

	//synch
	if(ioctl(info->_module_file_if, IOCTLCMD_TASKBEAT_UPDATED,0) !=0)
		return nullptr;

	return data;
}

static inline bool task_beat_update_domain(
		task_beat_info_t* info,//the struct returned when the task is registered
		task_beat_data_t *data,//the struct returned whe the domain is updated
		task_beat_type type,//beat type
		uint64_t tgt_rate//the tgt beat rate in beats/us
		)
{
	data->tgt_rate = tgt_rate;
	data->type = type;
	//synch
	if(ioctl(info->_module_file_if, IOCTLCMD_TASKBEAT_UPDATED,0) !=0)
		return false;
	return true;
}

static inline void task_beat(task_beat_data_t *data){
	data->__curr_beat_cnt += 1;
}
static inline void task_beat_ts(task_beat_info_t* info, task_beat_data_t *data)//thread-safe
{
	pthread_mutex_lock(&(info->_mutex));
	data->__curr_beat_cnt += 1;
	pthread_mutex_unlock(&(info->_mutex));
}
static inline uint64_t task_beat_read_total(task_beat_data_t *data){
	return data->__curr_beat_cnt;
}

static inline double task_beat_read_total_rate(task_beat_info_t* info, task_beat_data_t *data)//thread-safe
{
	return (double)data->__curr_beat_cnt /(((double)_task_beat_current_timestamp_ms() - (double)info->_start_time)/1000);
}

//return in beats/s
static inline double task_beat_read_tgt_rate(task_beat_info_t* info, task_beat_data_t *data)//thread-safe
{
	return (double)data->tgt_rate;
}

//TODO c++ interface here


#endif

