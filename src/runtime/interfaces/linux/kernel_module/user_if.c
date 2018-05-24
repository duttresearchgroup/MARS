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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/mm.h>       /* mmap related stuff               */

#include "user_if.h"
#include "helpers.h"
#include "sense.h"
#include "sense_data.h"

#ifndef VM_RESERVED
#define VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

// Pointer to the debugfs file
struct dentry *debugfs_file = nullptr;

#define DAEMON_PIDS_CNT 4
static pid_t daemon_pids[DAEMON_PIDS_CNT] = {-1,-1,-1,-1};

static inline bool is_daemon_pid(pid_t pid){
	int i;
	for(i = 0; i < DAEMON_PIDS_CNT; ++i) if(daemon_pids[i]==pid) return true;
	return false;
}
static inline bool add_daemon_pid(pid_t pid){
	int i;
	for(i = 0; i < DAEMON_PIDS_CNT; ++i)
		if(daemon_pids[i]==-1){
			daemon_pids[i] = pid;
			return true;
		}
	return false;
}
static inline bool rm_daemon_pid(pid_t pid){
	int i;
	for(i = 0; i < DAEMON_PIDS_CNT; ++i)
		if(daemon_pids[i]==pid){
			daemon_pids[i] = -1;
			return true;
		}
	return false;
}

static void mmap_open(struct vm_area_struct *vma)
{
	//pinfo("mmap_open() virt %lx, phys %lx, for %d\n",
	//		vma->vm_start, vma->vm_pgoff << PAGE_SHIFT,
	//		current->pid);
}

static void mmap_close(struct vm_area_struct *vma)
{
	//pinfo("mmap_close() virt %lx, phys %lx, for %d\n",
	//		vma->vm_start, vma->vm_pgoff << PAGE_SHIFT,
	//		current->pid);
}

static struct vm_operations_struct mmap_vm_ops = {
	.open =  mmap_open,
	.close = mmap_close,
};

static inline int daemon_mmap(struct file *filp, struct vm_area_struct *vma){
	unsigned long remap_size;
	pinfo("daemon_mmap() sharing %u pages starting at %p, phy %x, with %d\n",
			vitsdata_page_cnt,vitsdata,virt_to_phys(vitsdata),
			current->pid);

	vma->vm_pgoff = page_to_pfn(virt_to_page(vitsdata));
	remap_size = vma->vm_end - vma->vm_start;

	if(remap_size > (vitsdata_page_cnt*PAGE_SIZE)){
		pinfo("%lu is not a valid mmap request size\n",remap_size);
		return -EAGAIN;
	}

    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, remap_size, vma->vm_page_prot))
		return -EAGAIN;

	vma->vm_ops = &mmap_vm_ops;
	vma->vm_flags |= VM_RESERVED;
	mmap_open(vma);
	return 0;
}

static int user_mmap(pid_t task_pid, struct file *filp, struct vm_area_struct *vma){
	unsigned long remap_size;

	//first get the task and allocate beat data
	private_hook_data_t *task = add_created_task(current);
	BUG_ON(current->pid != task_pid);

	if(task == nullptr){
		pinfo("user_mmap failed: task pid %d not captured by module!\n",task_pid);
		return -EAGAIN;
	}
	if(task->beats != nullptr){
		pinfo("user_mmap failed: task %s pid %d already has beats data!\n",task->hook_data->this_task_name,task_pid);
		return -EAGAIN;
	}

	alloc_task_beat_data(task);

	pinfo("user_mmap() sharing beats data page starting at %p, phy %x, with %d\n",
			task->beats,virt_to_phys(task->beats), task_pid);

	vma->vm_pgoff = page_to_pfn(virt_to_page(task->beats));
	remap_size = vma->vm_end - vma->vm_start;

	if(remap_size != PAGE_SIZE){
		pinfo("user_mmap failed: %lu is not a valid mmap request size\n",remap_size);
		return -EAGAIN;
	}

    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, remap_size, vma->vm_page_prot))
		return -EAGAIN;

	vma->vm_ops = &mmap_vm_ops;
	vma->vm_flags |= VM_RESERVED;
	mmap_open(vma);
	return 0;
}

static int debugfs_mmap(struct file *filp, struct vm_area_struct *vma){
	//if the calling process is the daemon then we share the pages that have
	//all the sensing info
	//otherwise we share a page that the process can use to write heartbeat info
	pid_t current_pid = current->pid;
	if(is_daemon_pid(current_pid)){
		pinfo("debugfs_mmap() for registered daemon pid %d\n",current_pid);
		return daemon_mmap(filp,vma);
	}
	else{
		pinfo("debugfs_mmap() for user pid %d\n",current_pid);
		return user_mmap(current_pid,filp,vma);
	}
}

#include "../kernel_module/user_if_ioctl.h"

//return true if the given pid can execute the ictl cmd
static inline bool ioctl_security(pid_t pid, unsigned int cmd){
	switch (cmd) {
		//cmds that can be executed by anyone
		case IOCTLCMD_TASKBEAT_UPDATED: return true;
		case IOCTLCMD_REGISTER_DAEMON: return true;
		//every other cmd can only be exeuted by the daemon pid or its direct children
		default: return is_daemon_pid(pid);
	}
}

static inline long ioctl_register_daemon(pid_t pid, unsigned long arg){
	if(arg == SECRET_WORD){
		if(add_daemon_pid(pid)){
			pinfo("Proc. pid %d registered as deamon\n",pid);
			return 0;
		}
		else{
			pinfo("Failed to register pid %d as deamon. Max registered pids reached\n",pid);
			return -1;
		}
	}
	else{
		pinfo("Failed to register pid %d as deamon. Wrong secret word\n",pid);
		return -1;
	}
}
static inline long ioctl_unregister_daemon(pid_t pid){
	if(rm_daemon_pid(pid)){
		pinfo("Unregisterd daemon proc. pid %d\n",pid);
		return 0;
	}
	else{
		pinfo("Failed to unregister daemon proc pid %d. Not found !\n",pid);
		return -1;
	}
}


static long debugfs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	pid_t current_pid = current->pid;
	if(!ioctl_security(current_pid,cmd)){
		pinfo("Process %d, cannot perform ioctl cmd %u\n",current_pid,cmd);
		return -1;
	}
	switch (cmd) {
		case IOCTLCMD_SENSE_WINDOW_WAIT_ANY:
			return ioctlcmd_sense_window_wait_any();
		case IOCTLCMD_SENSE_WINDOW_CREATE:
			return ioctlcmd_sense_window_create(arg);
		case IOCTLCMD_SENSING_START:
			return ioctlcmd_sensing_start();
		case IOCTLCMD_SENSING_STOP:
			return ioctlcmd_sensing_stop();
		case IOCTLCMD_ENABLE_PERTASK_SENSING:
			return ioctlcmd_enable_pertask_sensing(arg);
		case IOCTLCMD_PERFCNT_ENABLE:
			return ioctlcmd_perfcnt_enable(arg);
		case IOCTLCMD_PERFCNT_RESET:
			return ioctlcmd_perfcnt_reset();
		case IOCTLCMD_TASKBEAT_UPDATED:
			return ioctlcmd_task_beat_updated(current_pid);
		case IOCTLCMD_REGISTER_DAEMON:
			return ioctl_register_daemon(current_pid,arg);
		case IOCTLCMD_UNREGISTER_DAEMON:
			return ioctl_unregister_daemon(current_pid);
		default:
			pinfo("debugfs_ioctl() unknown cmd\n");
			return -1;
	}
    return -1;
}

/*
 * Open the file; in fact, there's nothing to do here.
 */
static int debugfs_open (struct inode *inode, struct file *filp)
{
	//pinfo("debugfs_open()\n");
	return 0;
}

/*
 * Closing is just as simpler.
 */
static int debugfs_release(struct inode *inode, struct file *filp)
{
	//pinfo("debugfs_release()\n");
	return 0;
}

static const struct file_operations debugfs_fops = {
	.owner   = THIS_MODULE,
	.open    = debugfs_open,
	.release = debugfs_release,
    .mmap = debugfs_mmap,
    .unlocked_ioctl = debugfs_ioctl,
    .compat_ioctl = debugfs_ioctl,
};

bool create_user_if()
{
	debugfs_file = debugfs_create_file("vitamins", 0666, NULL, NULL, &debugfs_fops);
    if(debugfs_file) return true;
    else return false;
}

void destroy_user_if()
{
    debugfs_remove(debugfs_file);
}


