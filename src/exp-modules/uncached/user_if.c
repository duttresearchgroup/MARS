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

#include <linux/stddef.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>

#include "user_if.h"
#include "shared_data.h"

#ifndef VM_RESERVED
#define VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

// Pointer to the debugfs file
struct dentry *debugfs_file = 0;

static void mmap_open(struct vm_area_struct *vma)
{

}

static void mmap_close(struct vm_area_struct *vma)
{

}

static struct vm_operations_struct mmap_vm_ops = {
	.open =  mmap_open,
	.close = mmap_close,
};

static unsigned int last_cmd = IOCTLCMD_CACHEDALLOC;

static int debugfs_mmap_cached(struct file *filp, struct vm_area_struct *vma){
	unsigned long remap_size;
	pinfo("debugfs_mmap_cached() sharing %u pages starting at %p, phy %x\n",
			shared_data_page_cnt,cached_shared_data,virt_to_phys(cached_shared_data));

	vma->vm_pgoff = page_to_pfn(virt_to_page(cached_shared_data));
	remap_size = vma->vm_end - vma->vm_start;

	if(remap_size > (shared_data_page_cnt*PAGE_SIZE)){
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

static int debugfs_mmap_uncached(struct file *filp, struct vm_area_struct *vma){
	pinfo("debugfs_mmap_uncached() sharing %u pages starting at %p, phy %x\n",
				shared_data_page_cnt,uncached_coherent_shared_data,virt_to_phys(uncached_coherent_shared_data));
	 return dma_common_mmap(
			 NULL,
			 vma,
			 uncached_coherent_shared_data,
			 uncached_coherent_shared_data_handle,
			 vma->vm_end - vma->vm_start);
}

static int debugfs_mmap_uncached_noncoherent(struct file *filp, struct vm_area_struct *vma){
	pinfo("debugfs_mmap_uncached_noncoherent() sharing %u pages starting at %p, phy %x\n",
				shared_data_page_cnt,uncached_noncoherent_shared_data,virt_to_phys(uncached_noncoherent_shared_data));
	 return dma_common_mmap(
			 NULL,
			 vma,
			 uncached_noncoherent_shared_data,
			 uncached_noncoherent_shared_data_handle,
			 vma->vm_end - vma->vm_start);
}

static int debugfs_mmap(struct file *filp, struct vm_area_struct *vma){
	if(last_cmd == IOCTLCMD_CACHEDALLOC)
		return debugfs_mmap_cached(filp,vma);
	else if(last_cmd == IOCTLCMD_UNCACHEDALLOC_COHERENT)
		return debugfs_mmap_uncached(filp,vma);
	else if(last_cmd == IOCTLCMD_UNCACHEDALLOC_NONCOHERENT)
		return debugfs_mmap_uncached_noncoherent(filp,vma);
	else{
		pinfo("Fuck\n");
		return -1;
	}
	return 0;
}

static long debugfs_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	switch (cmd) {
		case IOCTLCMD_CACHEDALLOC:
			pinfo("IOCTLCMD_CACHEDALLOC\n");
			last_cmd = IOCTLCMD_CACHEDALLOC;
			return 0;
		case IOCTLCMD_UNCACHEDALLOC_COHERENT:
			pinfo("IOCTLCMD_UNCACHEDALLOC_COHERENT\n");
			last_cmd = IOCTLCMD_UNCACHEDALLOC_COHERENT;
			return 0;
		case IOCTLCMD_UNCACHEDALLOC_NONCOHERENT:
			pinfo("IOCTLCMD_UNCACHEDALLOC_NONCOHERENT is not working !\n");
			//last_cmd = IOCTLCMD_UNCACHEDALLOC_NONCOHERENT;
			return 0;
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
	if(!alloc_shared_data()) return false;

	debugfs_file = debugfs_create_file("uncached", 0666, NULL, NULL, &debugfs_fops);
	if(!debugfs_file) return false;

    return true;
}

void destroy_user_if()
{
    debugfs_remove(debugfs_file);
    dealloc_shared_data();
}


