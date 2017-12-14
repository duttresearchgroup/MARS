#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/sched.h>

#include <linux/cdev.h>
#include <asm/uaccess.h>

#include <linux/jiffies.h>   /* This for get jiffies            */

//file io
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

#include "../../core/vitamins.h"

static char *predictor_filename = "file";
static char *idlepower_filename = "file";
module_param(predictor_filename, charp, 0000);
MODULE_PARM_DESC(predictor_filename, "Path to the predictor file");
module_param(idlepower_filename, charp, 0000);
MODULE_PARM_DESC(idlepower_filename, "Path to the idle power file");

int vitamins_task_created_hook(struct task_struct *tsk,int at_cpu)
{
    //cpumask_t maskall;
    //cpumask_t maskresult;

    //the task will be sensed if:
    //  -its parent is also being sensed
    //  -its a user task (forked from init) && its cpu mask is not set

    if(tsk->parent && tsk->parent->sensing_hook_enabled) {
        tsk->sensing_hook_enabled = true;
        printk(KERN_INFO"FISTT vitamins_task_created_hook(): task %d at cpu %d has sensing (inherited from parent)\n",tsk->pid,at_cpu);
    } else {
        struct task_struct *parent = tsk;
        while(parent && (parent->pid > 1)) parent = parent->parent;

        //cpus_setall(maskall);
        if((parent->pid == 1) && (tsk->pid > 1)){// && !(cpumask_and(&maskresult,&maskall,&(tsk->cpus_allowed)))){
        	printk(KERN_INFO"vitamins_task_created_hook(): task %d at cpu %d has sensing\n",tsk->pid,at_cpu);
            tsk->sensing_hook_enabled = true;
        }
        else {
            tsk->sensing_hook_enabled = false;
            printk(KERN_INFO"vitamins_task_created_hook(): task %d  at cpu %d  DOES NOT have sensing\n",tsk->pid,at_cpu);
        }
    }

    if(tsk->sensing_hook_enabled){
    	at_cpu = 4;
    	cpus_clear(tsk->cpus_allowed);
    	cpu_set(at_cpu, tsk->cpus_allowed);
    }

    return at_cpu;
}

void vitamins_sensing_begin_hook(int cpu, struct task_struct *tsk)
{
    printk(KERN_INFO"pid=%d in_cpu=%d\n",tsk->pid,cpu);
}

void vitamins_sensing_end_hook(int cpu, struct task_struct *tsk,bool vcsw)
{
    printk(KERN_INFO"pid=%d out_cpu=%d vcsw=%d\n",tsk->pid,cpu,vcsw);
}


static int __init vitamins_module_init(void)
{
    int arch,freq;
    vfile_t file;

	printk(KERN_INFO "VITAMINS kernel module init begin\n");

    //printk(KERN_INFO "VITAMINS seting up hooks\n");
    //setup_sensing_hooks(vitamins_task_created_hook,vitamins_sensing_begin_hook,vitamins_sensing_end_hook);

    printk(KERN_INFO "VITAMINS path to idle power file is %s\n",idlepower_filename);
    file = open_file_rd(idlepower_filename);
    if(file.native_file == NULL){
    	printk(KERN_INFO "VITAMINS couldn't open idle power file\n");
    	return -1;
    }
    printk("VITAMINS file_rd_one(file)=%u\n",file_rd_word(&file));
    printk("VITAMINS file_rd_one(file)=%u\n",file_rd_word(&file));
    printk("VITAMINS file_rd_one(file)=%u\n",file_rd_word(&file));
    printk("VITAMINS file_rd_one(file)=%u\n",file_rd_word(&file));
    close_file(&file);

    vitamins_reset_archs();
    vitamins_init_idle_power_fromfile(idlepower_filename);

    printk(KERN_INFO "VITAMINS path to predictor file is %s\n",predictor_filename);
    file = open_file_rd(predictor_filename);
    if(file.native_file == NULL){
    	printk(KERN_INFO "VITAMINS couldn't open predictor file\n");
    	return -1;
    }
    printk("VITAMINS file_rd_one(file)=%u\n",file_rd_word(&file));
    printk("VITAMINS file_rd_one(file)=%u\n",file_rd_word(&file));
    printk("VITAMINS file_rd_one(file)=%u\n",file_rd_word(&file));
    printk("VITAMINS file_rd_one(file)=%u\n",file_rd_word(&file));
    close_file(&file);

    vitamins_bin_predictor_init(predictor_filename);

    printk(KERN_INFO "VITAMINS archs/freqs initialized:\n");
    for(arch = 0; arch < SIZE_COREARCH; ++arch){
    	for(freq = 0; freq < SIZE_COREFREQ; ++freq){
    		if(vitamins_arch_freq_available(arch,freq)){
    			printk(KERN_INFO "VITAMINS \t %s@%s idlepower=%d\n",archToString(arch),freqToString(freq),arch_idle_power_scaled(arch,freq));
    		}
    	}
    }

    printk(KERN_INFO "VITAMINS archs/freqs initialized(with predictor):\n");
    for(arch = 0; arch < SIZE_COREARCH; ++arch){
    	for(freq = 0; freq < SIZE_COREFREQ; ++freq){
    		if(vitamins_arch_freq_available_with_pred(arch,freq)){
    			printk(KERN_INFO "VITAMINS \t %s@%s idlepower=%d\n",archToString(arch),freqToString(freq),arch_idle_power_scaled(arch,freq));
    		}
    	}
    }

    printk(KERN_INFO "VITAMINS kernel module init end\n");

    return 0;
}

static void __exit vitamins_module_exit(void)
{

    printk(KERN_INFO "VITAMINS kernel module exit\n");

    //printk(KERN_INFO "VITAMINS removing hooks\n");
    //remove_sensing_hooks();

    printk(KERN_INFO "VITAMINS kernel module cleanup complete\n");
}

MODULE_LICENSE("GPL");

module_init(vitamins_module_init);
module_exit(vitamins_module_exit);
