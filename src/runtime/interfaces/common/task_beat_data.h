#ifndef __arm_rt_common_task_beat_data_h
#define __arm_rt_common_task_beat_data_h

#define MAX_BEAT_DOMAINS 1

typedef enum {
	BEAT_PERF,
	BEAT_IO
} task_beat_type;

struct task_beat_data_struct {
	task_beat_type	type;
	uint64_t		tgt_rate;//in beats per second
	uint64_t __curr_beat_cnt;
};
typedef struct task_beat_data_struct task_beat_data_t;

struct task_beat_struct {
	uint32_t __checksum0;

	task_beat_data_t beat_data[MAX_BEAT_DOMAINS];
	int num_beat_domains;

	uint32_t __checksum1;
};
typedef struct task_beat_struct task_beat_t;

static inline void set_task_beat_data_cksum(task_beat_t *data){
	data->__checksum0 = 0xDEADBEEF;
	data->__checksum1 = 0xBEEFDEAD;
}

static inline bool check_task_beat_data_cksum(task_beat_t *data){
	return (data->__checksum0 == 0xDEADBEEF) && (data->__checksum1 == 0xBEEFDEAD);
}


#endif

