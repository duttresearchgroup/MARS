#ifndef __arm_rt_sense_data_h
#define __arm_rt_sense_data_h

//////////////////////////////////////////
//Sensing data definition         ////////
//////////////////////////////////////////

#include<linux/sched.h>

#include "../linux-module/core.h"
#include "../common/sense_data_shared.h"
#include "../common/task_beat_data.h"

//pointer to the sensed data struct
extern sensed_data_t *vitsdata;
extern unsigned int vitsdata_page_cnt;

//allocs the memory to keep all the sensed data
//should be called first thing when the module is loaded
//updates the global vitsdata pointer
sensed_data_t* alloc_sensed_data(sys_info_t *info);

//deallocs the memory to keep all the sensed data
//should be the last thing called when the module is unloaded
void dealloc_sensed_data(void);


//wrapper for the task hook data with the kernel-only stuff
struct private_hook_data_struct {
	tracked_task_data_t *hook_data;
    spinlock_t                  sen_data_lock;
    //kernel task info
    struct task_struct *this_task;
    //link for the hash that maps pid to this object

    //task beat data
    //allocated only when the task registers itself as having beat data
    //MUST be nullptr otherwise
    task_beat_t* beats;

    define_list_addable(struct private_hook_data_struct,hashmap);
};
typedef struct private_hook_data_struct private_hook_data_t;

void alloc_task_beat_data(private_hook_data_t *task);
void dealloc_task_beat_data(private_hook_data_t *task);

//this mirrors vitsdata->created_tasks
extern private_hook_data_t priv_hook_created_tasks[MAX_CREATED_TASKS];

// maps a pid of an added tasks to a vitamins_hook_data_t*
#define hook_data_hashmap_struct_size 1024
define_vitamins_list(extern private_hook_data_t, hook_hashmap[hook_data_hashmap_struct_size]);
extern rwlock_t hook_hashmap_lock[hook_data_hashmap_struct_size];
//
static inline private_hook_data_t* __hook_hashmap_get(int idx,pid_t pid){
	private_hook_data_t *curr = nullptr;
    for_each_in_list(hook_hashmap[idx],curr,hashmap){
    	if(curr->hook_data->this_task_pid == pid) return curr;
    }
    return nullptr;
}
static inline private_hook_data_t* hook_hashmap_get_pid(pid_t pid){
	private_hook_data_t *result;
	int idx = pid % hook_data_hashmap_struct_size;
	read_lock(&(hook_hashmap_lock[idx]));
		result = __hook_hashmap_get(idx,pid);
	read_unlock(&(hook_hashmap_lock[idx]));
    return result;
}
static inline private_hook_data_t* hook_hashmap_get(struct task_struct *tsk){ return hook_hashmap_get_pid(tsk->pid); }
//
static inline void hook_hashmap_add(struct task_struct *tsk, private_hook_data_t *hookdata){
	int idx = tsk->pid % hook_data_hashmap_struct_size;
	BUG_ON(hook_hashmap_get(tsk)!=nullptr);
	write_lock(&(hook_hashmap_lock[idx]));
		add_to_list(hook_hashmap[idx],hookdata,hashmap);
	write_unlock(&(hook_hashmap_lock[idx]));
}

#endif

