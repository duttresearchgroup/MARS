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

struct sensing_window_data_struct {
	//updated before when the sensing window period is done
	//no synch and the user-level task should be done reading these guys before the next perio expires
	sensed_data_cpu_t			cpus[MAX_NR_CPUS];
	sensed_data_power_domain_t	power_domains[MAX_NR_CPUS];
	sensed_data_freq_domain_t	freq_domains[MAX_NR_CPUS];

	sensed_data_task_t			tasks[MAX_CREATED_TASKS];
};
typedef struct sensing_window_data_struct sensing_window_data_t;

struct sensing_window_struct {
	//continuously incremented during the sensing period
	//unsafe to read from user-level
	//changed by the sensing module only
	sensing_window_data_t _acc;

	//sensing window data
	//updated before when the sensing window period is done
	//user interface accesses these for all counter data
	//no synch and the user-level task should be done reading these guys before the next period expires
	sensing_window_data_t curr;
    sensing_window_data_t aggr;//aggregate values for all previous samples for this window.

	uint64_t		   			curr_sample_time_ms;
	uint64_t		   			prev_sample_time_ms;

	uint32_t num_of_samples;

	//this window id
	int wid;
};
typedef struct sensing_window_struct sensing_window_t;

//single struct containing all the sensed data
//there is only one instance of this
struct sensed_data_struct {
	uint32_t __checksum0;

	//sensing window global sensed data
	//the tasks' sensed data for each window is inside each task hook data
	sensing_window_t sensing_windows[MAX_WINDOW_CNT];

	// list of created tasks. Stores sensing info for each task
	//as of now there is no dealloc because we keep al the sensed info even
	//after the task is done
	tracked_task_data_t created_tasks[MAX_CREATED_TASKS];
	int created_tasks_cnt;
	int __created_tasks_cnt_tmp;//used to handle race conditions during task creation. Do not read/write this

	uint32_t num_of_minimum_periods;//number of times the innermost sensing loop was executed
	uint32_t num_of_csw_periods[MAX_NR_CPUS];//number of time sensing was done during context switch on each cpu

	int number_of_cpus;
	uint32_t starttime_ms;
	uint32_t stoptime_ms;

	// history data for controller
	int power_err_prev_epoch[MAX_NR_CPUS];
	int power_set_prev_epoch[MAX_NR_CPUS];
	int ips_err_prev_epoch[MAX_NR_CPUS];
	int ips_set_prev_epoch[MAX_NR_CPUS];

	// perf counter mapping info
	int idx_to_perfcnt_map[MAX_PERFCNTS];
	int perfcnt_to_idx_map[SIZE_PERFCNT];
	int perfcnt_mapped_cnt;

	uint32_t __sysChecksum;
	uint32_t __checksum1;
};
typedef struct sensed_data_struct sensed_data_t;

static inline void set_sensed_data_cksum(sensed_data_t *data){
	data->__checksum0 = 0xDEADBEEF;
	data->__checksum1 = 0xBEEFDEAD;
}

static inline bool check_sensed_data_cksum(sensed_data_t *data){
	return (data->__checksum0 == 0xDEADBEEF) && (data->__checksum1 == 0xBEEFDEAD);
}

#endif

