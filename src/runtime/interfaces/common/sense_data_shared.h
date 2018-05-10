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

#ifndef __arm_rt_sense_data_shared_h
#define __arm_rt_sense_data_shared_h

//////////////////////////////////////////
//Sensing data definition         ////////
//////////////////////////////////////////

//this header is self-contained and does not have kernel includes. It can be used in both kernel and user-level stuff

#include "sense_defs.h"
#include "sensing_window_defs.h"
#include "pal/pal_setup.h"

//TODO unsafe XD
#define SECRET_WORD 0xCAFEBABE

struct perf_window_data_struct {
	//updated before when the sensing window period is done
	//no synch and the user-level task should be done reading these guys before the next perio expires
	perf_data_cpu_t			cpus[MAX_NR_CPUS];
	perf_data_freq_domain_t	freq_domains[MAX_NR_CPUS];

	perf_data_task_t			tasks[MAX_CREATED_TASKS];
};
typedef struct perf_window_data_struct perf_window_data_t;

struct perf_window_struct {
	//sensing window data
	//updated before when the sensing window period is done
	//user interface accesses these for all counter data
	//no synch and the user-level task should be done reading these guys before the next period expires
	perf_window_data_t curr;
    perf_window_data_t aggr;//aggregate values for all previous samples for this window.

	uint64_t		   			curr_sample_time_ms;
	uint64_t		   			prev_sample_time_ms;

	uint32_t num_of_samples;

	int created_tasks_cnt;

	//this window id
	int wid;

	volatile bool ___updating;
	volatile bool ___reading;
};
typedef struct perf_window_struct perf_window_t;

//single struct containing all the sensed data
//there is only one instance of this
struct perf_data_struct {
	uint32_t __checksum0;

	//sensing window global sensed data
	//the tasks' sensed data for each window is inside each task hook data
	perf_window_t sensing_windows[MAX_WINDOW_CNT];
	int sensing_window_cnt;

	//Accumulated counters used to compute the windows above
	//Counters in __acc_cpus and __acc_tasks are continuously incremented every
	//context switch, while __acc_freq_domains is incremented every minimum_sensing_window().
    //Unsafe to read from user-level and private to the sensing module only.
	//Unsafe to read/write without the proper locks by anyone.
	//Notice __acc_cpus and __acc_tasks are double-buffered to avoid blocking
	//during context switches
    perf_data_cpu_t         __acc_cpus[MAX_NR_CPUS][2];
    perf_data_cpu_t         __acc_tasks[MAX_CREATED_TASKS][2];
    int                     __acc_tasks_last_cpu[MAX_CREATED_TASKS];
    perf_data_freq_domain_t __acc_freq_domains[MAX_NR_CPUS];

	// list of created tasks. Stores sensing info for each task.
	//As of now there is no dealloc because we keep al the sensed info even
	//after the task is done
	tracked_task_data_t created_tasks[MAX_CREATED_TASKS];
	//number of created tasks. Notice this may increment in the middle of a
	//sensing window, so sensing_windows[wid].created_tasks_cnt should be used
	//to get the number of tasks in the latest window
	int created_tasks_cnt;
	int __created_tasks_cnt_tmp;//used to handle race conditions during task creation. Do not read/write this

	uint32_t num_of_minimum_periods;//number of times the innermost sensing loop was executed
	uint32_t num_of_csw_periods[MAX_NR_CPUS];//number of time sensing was done during context switch on each cpu

	int number_of_cpus;
	uint32_t starttime_ms;
	uint32_t stoptime_ms;

	// perf counter mapping info
	int idx_to_perfcnt_map[MAX_PERFCNTS];
	int perfcnt_to_idx_map[SIZE_PERFCNT];
	int perfcnt_mapped_cnt;

	uint32_t __sysChecksum;
	uint32_t __checksum1;
};
typedef struct perf_data_struct perf_data_t;

/////////////////////////////////
// Checksum funcs

static inline void set_perf_data_cksum(perf_data_t *data){
	data->__checksum0 = 0xDEADBEEF;
	data->__checksum1 = 0xBEEFDEAD;
}

static inline bool check_perf_data_cksum(perf_data_t *data){
	return (data->__checksum0 == 0xDEADBEEF) && (data->__checksum1 == 0xBEEFDEAD);
}


///////////////////////////////////////
// Initialization funcs

static inline void perf_data_init(perf_data_t *data, sys_info_t *info)
{
    data->starttime_ms = 0;
    data->created_tasks_cnt = 0;
    data->__created_tasks_cnt_tmp = 0;
    data->number_of_cpus = info->core_list_size;
    data->sensing_window_cnt = 0;
    data->__sysChecksum = sys_info_cksum(info);
    set_perf_data_cksum(data);
}

static inline void _perf_data_reset_perf_counters(perf_data_perf_counters_t *sen_data){
    int cnt;
    for(cnt = 0; cnt < MAX_PERFCNTS; ++cnt) sen_data->perfcnts[cnt] = 0;
    sen_data->nivcsw= 0;
    sen_data->nvcsw = 0;
    sen_data->time_busy_ms = 0;
    sen_data->time_total_ms = 0;
}

static inline void _perf_data_reset_task_counters(int cpu,perf_data_task_t *sen_data){
    int cnt;
    _perf_data_reset_perf_counters(&(sen_data->perfcnt));
    for(cnt = 0; cnt < MAX_BEAT_DOMAINS; ++cnt) sen_data->beats[cnt] = 0;
    sen_data->last_cpu_used = cpu;
}

static inline void _perf_data_reset_cpu_counters(perf_data_cpu_t *sen_data){
    int cnt;
    for(cnt = 0; cnt < MAX_BEAT_DOMAINS; ++cnt) sen_data->beats[cnt] = 0;
    _perf_data_reset_perf_counters(&(sen_data->perfcnt));
}
static inline void _perf_data_reset_freq_counters(perf_data_freq_domain_t *sen_data, uint64_t last_update_time){
    sen_data->avg_freq_mhz_acc = 0;
    sen_data->time_ms_acc = 0;
    sen_data->last_update_time_ms = last_update_time;
}

static inline void perf_data_reset_task(perf_data_t *data, int task_idx, int initial_cpu)
{
    int i;
    _perf_data_reset_cpu_counters(&(data->__acc_tasks[task_idx][0]));
    _perf_data_reset_cpu_counters(&(data->__acc_tasks[task_idx][1]));
    data->__acc_tasks_last_cpu[task_idx] = initial_cpu;
    for(i=0;i<data->sensing_window_cnt;++i){
        _perf_data_reset_task_counters(initial_cpu,&(data->sensing_windows[i].curr.tasks[task_idx]));
        _perf_data_reset_task_counters(initial_cpu,&(data->sensing_windows[i].aggr.tasks[task_idx]));
    }
}

static inline void perf_data_cleanup_counters(sys_info_t *sys, perf_data_t *data, uint64_t curr_time)
{
    int i,wid;

    for(wid=0;wid<data->sensing_window_cnt;++wid){
        data->sensing_windows[wid].curr_sample_time_ms = curr_time;
        data->sensing_windows[wid].prev_sample_time_ms = data->sensing_windows[wid].curr_sample_time_ms;
    }
    for(i = 0; i < sys->core_list_size; ++i){
        data->num_of_csw_periods[i] = 0;
        _perf_data_reset_cpu_counters(&(data->__acc_cpus[i][0]));
        _perf_data_reset_cpu_counters(&(data->__acc_cpus[i][1]));
        for(wid=0;wid<data->sensing_window_cnt;++wid){
            _perf_data_reset_cpu_counters(&(data->sensing_windows[wid].curr.cpus[i]));
            _perf_data_reset_cpu_counters(&(data->sensing_windows[wid].aggr.cpus[i]));
        }
    }

    for(wid=0;wid<data->sensing_window_cnt;++wid){
        data->sensing_windows[wid].num_of_samples = 0;
        data->sensing_windows[wid].created_tasks_cnt = 0;
        data->sensing_windows[wid].wid = wid;
        data->sensing_windows[wid].___reading = false;
        data->sensing_windows[wid].___updating = false;
    }

    data->num_of_minimum_periods = 0;

    for(i = 0; i < sys->freq_domain_list_size; ++i){
        _perf_data_reset_freq_counters(&(data->__acc_freq_domains[i]),curr_time);
        for(wid=0;wid<data->sensing_window_cnt;++wid){
            _perf_data_reset_freq_counters(&(data->sensing_windows[wid].curr.freq_domains[i]),curr_time);
            _perf_data_reset_freq_counters(&(data->sensing_windows[wid].aggr.freq_domains[i]),curr_time);
        }
    }
}

/////////////////////////////
// perf conter maping funcs

static inline void perf_data_reset_mapped_perfcnt(perf_data_t *data)
{
    int i;
    for(i = 0; i < MAX_PERFCNTS;++i) data->idx_to_perfcnt_map[i] = -1;
    for(i = 0; i < SIZE_PERFCNT;++i) data->perfcnt_to_idx_map[i] = -1;
    data->perfcnt_mapped_cnt = 0;
}
static inline bool perf_data_map_perfcnt(perf_data_t *data, perfcnt_t perfcnt)
{
    if(data->perfcnt_mapped_cnt >= MAX_PERFCNTS){
        pinfo("Cannot use more then %d pmmu counters!",MAX_PERFCNTS);
        return false;
    }
    data->idx_to_perfcnt_map[data->perfcnt_mapped_cnt] = perfcnt;
    data->perfcnt_to_idx_map[perfcnt] = data->perfcnt_mapped_cnt;
    data->perfcnt_mapped_cnt += 1;
    return true;
}

//////////////////////////////////////////////////////////
// Functions for copying accumulated values per window

// Performs tgt->cnt 'op' acc->cnt, where 'op' is +=,-=,=, for all counters in
// the structs given by 'acc' and 'tgt'
// Note:  tgt->perfcnt.time_total_ms is not changed
#define _perf_data_op(op,tgt,acc) \
    do{\
        int cnt;\
        for(cnt = 0; cnt < MAX_PERFCNTS; ++cnt) (tgt)->perfcnt.perfcnts[cnt] op (acc)->perfcnt.perfcnts[cnt];\
        (tgt)->perfcnt.nvcsw op (acc)->perfcnt.nvcsw;\
        (tgt)->perfcnt.nivcsw op (acc)->perfcnt.nivcsw;\
        (tgt)->perfcnt.time_busy_ms op (acc)->perfcnt.time_busy_ms;\
        for(cnt = 0; cnt < MAX_BEAT_DOMAINS; ++cnt) (tgt)->beats[cnt] op (acc)->beats[cnt];\
    } while(0)

#define perf_data_set(tgt,acc) _perf_data_op(=,tgt,acc)
#define perf_data_inc(tgt,acc) _perf_data_op(+=,tgt,acc)
#define perf_data_dec(tgt,acc) _perf_data_op(-=,tgt,acc)

typedef void (*perf_data_commit_window_acc_lock_f)(int cpuOrTaskIdx, int acc_idx, unsigned long *flags);
typedef void (*perf_data_commit_window_acc_unlock_f)(int cpuOrTaskIdx, int acc_idx, unsigned long *flags);

static inline void perf_data_commit_cpu_window(sys_info_t *sys,
        perf_data_t *data, int wid, uint64_t curr_time_ms,
        perf_data_commit_window_acc_lock_f acc_lock,
        perf_data_commit_window_acc_unlock_f acc_unlock)
{
    int i;
    unsigned long flags;
    perf_data_cpu_t data_cnt;

    uint64_t time_elapsed_ms = curr_time_ms - data->sensing_windows[wid].curr_sample_time_ms;

    data->sensing_windows[wid].prev_sample_time_ms = data->sensing_windows[wid].curr_sample_time_ms;
    data->sensing_windows[wid].curr_sample_time_ms = curr_time_ms;

    for(i = 0; i < sys->core_list_size; ++i){
        perf_data_cpu_t *last_total = &(data->sensing_windows[wid].aggr.cpus[i]);
        perf_data_cpu_t *curr_epoch = &(data->sensing_windows[wid].curr.cpus[i]);

        //Combine data from both buffers
        (acc_lock)(i,0,&flags);
        perf_data_set(&data_cnt,&(data->__acc_cpus[i][0]));
        (acc_unlock)(i,0,&flags);

        (acc_lock)(i,1,&flags);
        perf_data_inc(&data_cnt,&(data->__acc_cpus[i][1]));
        (acc_unlock)(i,1,&flags);

        //data from current epoch
        perf_data_set(curr_epoch,&data_cnt);
        perf_data_dec(curr_epoch,last_total);

        //total acc data
        perf_data_set(last_total,&data_cnt);

        //perfcnt.time_total_ms is not updated within data_cnt, so handled separately
        curr_epoch->perfcnt.time_total_ms = time_elapsed_ms;
        last_total->perfcnt.time_total_ms += time_elapsed_ms;
    }

    //freq sense
    for(i = 0; i < sys->freq_domain_list_size; ++i){

        perf_data_freq_domain_t *last_total = &(data->sensing_windows[wid].aggr.freq_domains[i]);
        perf_data_freq_domain_t *curr_epoch = &(data->sensing_windows[wid].curr.freq_domains[i]);
        perf_data_freq_domain_t *data_cnt = &(data->__acc_freq_domains[i]);

        curr_epoch->avg_freq_mhz_acc = data_cnt->avg_freq_mhz_acc - last_total->avg_freq_mhz_acc;
        curr_epoch->time_ms_acc = data_cnt->time_ms_acc -  last_total->time_ms_acc;

        *last_total = *data_cnt;
    }
}

static inline void perf_data_commit_tasks_window(sys_info_t *sys,
        perf_data_t *data, int wid, uint64_t curr_time_ms,
        perf_data_commit_window_acc_lock_f acc_lock,
        perf_data_commit_window_acc_lock_f acc_unlock)
{
    int p;
    unsigned long flags;
    perf_data_cpu_t data_cnt;

    uint64_t time_elapsed_ms = curr_time_ms - data->sensing_windows[wid].prev_sample_time_ms;

    data->sensing_windows[wid].created_tasks_cnt = data->created_tasks_cnt;
    for(p = 0; p < data->sensing_windows[wid].created_tasks_cnt; ++p){
        perf_data_task_t *last_total = &(data->sensing_windows[wid].aggr.tasks[p]);
        perf_data_task_t *curr_epoch = &(data->sensing_windows[wid].curr.tasks[p]);

        //Combine data from both buffers
        (acc_lock)(p,0,&flags);
        perf_data_set(&data_cnt,&(data->__acc_tasks[p][0]));
        (acc_unlock)(p,0,&flags);

        (acc_lock)(p,1,&flags);
        perf_data_inc(&data_cnt,&(data->__acc_tasks[p][1]));
        (acc_unlock)(p,1,&flags);

        //data from current epoch
        perf_data_set(curr_epoch,&data_cnt);
        perf_data_dec(curr_epoch,last_total);

        //total acc data
        perf_data_set(last_total,&data_cnt);

        //perfcnt.time_total_ms is not updated within data_cnt, so handled separately
        curr_epoch->perfcnt.time_total_ms = time_elapsed_ms;
        last_total->perfcnt.time_total_ms += time_elapsed_ms;
        curr_epoch->last_cpu_used = data->__acc_tasks_last_cpu[p];
        last_total->last_cpu_used = data->__acc_tasks_last_cpu[p];
    }
}

#endif

