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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <trace/events/sched.h>

#include <linux/cdev.h>
#include <asm/uaccess.h>

#include <linux/jiffies.h>   /* This for get jiffies            */

#include "../../core/vitamins.h"

struct vitamins_hook_data_struct {
    struct task_struct *this_task;
    pid_t this_task_pid;
    char this_task_comm[TASK_COMM_LEN];
    //Default link for link for intrusive lists
    define_list_addable_default(struct vitamins_hook_data_struct);
    define_list_addable(struct vitamins_hook_data_struct,hashmap);
};
typedef struct vitamins_hook_data_struct vitamins_hook_data_t;

define_vitamins_list(static vitamins_hook_data_t, created_tasks);

#define vitamins_hook_data_hashmap_struct_size 1024
define_vitamins_list(vitamins_hook_data_t, hook_hashmap[vitamins_hook_data_hashmap_struct_size]);


static inline vitamins_hook_data_t* hook_hashmap_get(struct task_struct *tsk){
    vitamins_hook_data_t *curr = nullptr;
    for_each_in_list(hook_hashmap[tsk->pid % vitamins_hook_data_hashmap_struct_size],curr,hashmap){
    	if(curr->this_task == tsk) return curr;
    }
    return curr;
}
static inline void hook_hashmap_add(struct task_struct *tsk, vitamins_hook_data_t *hookdata){
	BUG_ON(hook_hashmap_get(tsk)!=nullptr);
	add_to_list(hook_hashmap[tsk->pid % vitamins_hook_data_hashmap_struct_size],hookdata,hashmap);
}


static void vitamins_new_task_created(int at_cpu, struct task_struct *tsk){
    //allocates the hook data
    vitamins_hook_data_t *hooks = kmalloc(sizeof(vitamins_hook_data_t),GFP_ATOMIC);

    clear_object_default(hooks);
    clear_object(hooks,hashmap);
    add_to_list_default(created_tasks,hooks);

    hooks->this_task = tsk;
    hooks->this_task_pid = tsk->pid;
    memcpy(hooks->this_task_comm,tsk->comm,TASK_COMM_LEN);
    hook_hashmap_add(tsk,hooks);

    //make sure the thread is always pinned to one CPU. This is only changed by the optimizer
    cpus_clear(tsk->cpus_allowed);
    cpu_set(at_cpu, tsk->cpus_allowed);

}

static inline int is_userspace(struct task_struct *tsk){
	//has parent and parent pid is > 1 and name does not begin with kthread
	return tsk->parent && (tsk->parent->pid > 1) && !((tsk->comm[0] == 'k') && (tsk->comm[6] == 'd'));
}

void vitamins_sensing_begin_probe(int cpu, struct task_struct *tsk)
{
    if(hook_hashmap_get(tsk))
    	printk(KERN_INFO"probe pid=%d in_cpu=%d\n",tsk->pid,cpu);
}

void vitamins_sensing_end_probe(int cpu, struct task_struct *tsk)
{
	if(hook_hashmap_get(tsk))
		printk(KERN_INFO"probe pid=%d out_cpu=%d vcsw=%d\n",tsk->pid,cpu,tsk->state && !(preempt_count() & PREEMPT_ACTIVE));
}


void probe_sched_process_fork(void *nope, struct task_struct *parent, struct task_struct *p){
	printk(KERN_INFO"probe_sched_process_fork() %d %s\n",p->pid,p->comm);

	if(is_userspace(p)){
		printk(KERN_INFO"vitamins_task_created_hook(): task %d %s IS USER SPACE\n",p->pid,p->comm);
		vitamins_new_task_created(2,p);
	}
	else{
		printk(KERN_INFO"vitamins_task_created_hook(): task %d %s IS KERN SPACE\n",p->pid,p->comm);
	}
}

void probe_context_switch(void *nope, struct task_struct *prev,struct task_struct *next){
	int cpu = task_thread_info(next)->cpu;
	vitamins_sensing_end_probe(cpu,prev);
	vitamins_sensing_begin_probe(cpu,next);
}

static int __init vitamins_module_init(void)
{
	int i;
	printk(KERN_INFO "VITAMINS kernel module init begin\n");

	for(i = 0; i < vitamins_hook_data_hashmap_struct_size; ++i){
		clear_list(hook_hashmap[i]);
	}

	register_trace_sched_switch(probe_context_switch,0);
	register_trace_sched_process_fork(probe_sched_process_fork,0);

    printk(KERN_INFO "VITAMINS kernel module init end\n");

    return 0;
}


void vitamins_sense_cleanup(void)
{
    int i;
	vitamins_hook_data_t *curr,*next;

	for(i = 0; i < vitamins_hook_data_hashmap_struct_size; ++i){
		//for_each_in_list(hook_hashmap[i],curr,hashmap){
		//	printk(KERN_INFO "VITAMINS hashmap[%d] had task %d %s\n",i,curr->this_task_pid,curr->this_task_comm);
		//}
		clear_list(hook_hashmap[i]);
	}

    curr = vitamins_list_head(created_tasks);
    next = curr;
    while(next != nullptr){
        curr = next;
        next = curr->_listnext_main;
        kfree(curr);
    }

    clear_list(created_tasks);
}


static void __exit vitamins_module_exit(void)
{

    printk(KERN_INFO "VITAMINS kernel module exit\n");

    unregister_trace_sched_switch(probe_context_switch,0);
    unregister_trace_sched_process_fork(probe_sched_process_fork,0);
	tracepoint_synchronize_unregister();

	vitamins_sense_cleanup();

    printk(KERN_INFO "VITAMINS kernel module cleanup complete\n");
}

MODULE_LICENSE("GPL");

module_init(vitamins_module_init);
module_exit(vitamins_module_exit);
