#include "../linux-module/sense_data.h"


#include <linux/slab.h>

#include "../common/sense_defs.h"
#include "../common/task_beat_data.h"
#include "../linux-module/core.h"

perf_data_t *vitsdata = nullptr;
unsigned int vitsdata_page_cnt = 0;

static unsigned int _alloc_order;
static inline void _alloc_sensed_data(void){
	_alloc_order = get_order(roundup_pow_of_two(sizeof(struct perf_data_struct)));
	vitsdata_page_cnt = 1 << _alloc_order;

	vitsdata = (struct perf_data_struct*)__get_free_pages(GFP_KERNEL,_alloc_order);
	//if(vitsdata){
	//	pinfo("Allocated %u bytes of sensed data using %u pages (%lu bytes)\n",sizeof(struct sensed_data_struct),vitsdata_page_cnt,vitsdata_page_cnt*PAGE_SIZE);
	//}
	//else {
	if(!vitsdata){
		pinfo("Failed to allocate sensed data !!\n");
		vitsdata = nullptr;
		vitsdata_page_cnt = 0;
	}
}

//allocs the memory to keep all the sensed data
//should be called first thing when the module is loaded
perf_data_t* alloc_sensed_data(sys_info_t *info){
	_alloc_sensed_data();
	if(vitsdata == nullptr) return nullptr;

	vitsdata->starttime_ms = 0;
	vitsdata->created_tasks_cnt = 0;
	vitsdata->__created_tasks_cnt_tmp = 0;
	vitsdata->number_of_cpus = NR_CPUS;

	BUG_ON(MAX_NR_CPUS < NR_CPUS);

	vitsdata->__sysChecksum = sys_info_cksum(info);
	set_perf_data_cksum(vitsdata);

	return vitsdata;
}

//deallocs the memory to keep all the sensed data
//should be the last thing called when the module is unloaded
void dealloc_sensed_data(){
	if(vitsdata) free_pages((unsigned long)vitsdata,_alloc_order);
	vitsdata = nullptr;
	vitsdata_page_cnt = 0;
}


private_hook_data_t priv_hook_created_tasks[MAX_CREATED_TASKS];
define_vitamins_list(private_hook_data_t, hook_hashmap[hook_data_hashmap_struct_size]);
rwlock_t hook_hashmap_lock[hook_data_hashmap_struct_size];

static unsigned int _beats_alloc_order;
void alloc_task_beat_data(private_hook_data_t *task){
	unsigned int page_cnt;
	task_beat_t* data;

	BUG_ON(task->beats != nullptr);

	//because this will be share, allocate all within a page
	//number of pages allocated must be == 1
	_beats_alloc_order = get_order(roundup_pow_of_two(sizeof(struct task_beat_struct)));
	page_cnt = 1 << _beats_alloc_order;

	BUG_ON(page_cnt != 1);

	data = (task_beat_t*)__get_free_pages(GFP_KERNEL,_alloc_order);
	if(!data){
		pinfo("Failed to alloc beat data for task %d %s!!\n",task->hook_data->this_task_pid,task->hook_data->this_task_name);
		task->beats = nullptr;
	}
	else{
		data->num_beat_domains = 0;
		set_task_beat_data_cksum(data);
		task->beats = data;
	}
}
void dealloc_task_beat_data(private_hook_data_t *task){
	if(task->beats != nullptr){
		free_pages((unsigned long)task->beats,_beats_alloc_order);
		task->beats = nullptr;
	}
}
