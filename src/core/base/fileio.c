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

#include "base.h"


#ifndef __KERNEL__

typedef FILE* native_file_t;

static inline native_file_t native_open_file_rd(const char* filepath)
{
    return fopen(filepath, "r");
}
static inline native_file_t native_open_file_wr(const char* filepath)
{
    return fopen(filepath, "w");
}
static inline void native_close_file(native_file_t file)
{
    fclose(file);
}
static inline void native_file_rd(native_file_t a_file, char *ptr, int count)
{
    int ret = fread(ptr,1,count,a_file);
    BUG_ON(ret!=count);
}
static inline void native_file_wr(native_file_t a_file, const char *ptr, int count)
{
    int ret =  fwrite(ptr,1,count,a_file);
    BUG_ON(ret!=count);
}
#else

#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

typedef struct file* native_file_t;

static inline native_file_t native_open_file_rd(const char* filepath){
	struct file* fd;
    mm_segment_t oldfs;

    oldfs = get_fs();
    set_fs(get_ds());

    fd = filp_open(filepath, O_RDONLY, 0);

    if(IS_ERR(fd)) {
    	set_fs(oldfs);
    	return nullptr;
    }
    set_fs(oldfs);
    return fd;
}
static inline native_file_t native_open_file_wr(const char* filepath){
	printk(KERN_INFO"Writing files from the kernel is not supported\n");
	return NULL;
}
static inline void native_close_file(native_file_t file){
    mm_segment_t oldfs;
    BUG_ON(file==NULL);
    oldfs = get_fs();
    set_fs(get_ds());
    filp_close(file,NULL);
    set_fs(oldfs);
}

static inline int _native_file_rd(native_file_t file, unsigned char* data, unsigned int size) {
    mm_segment_t oldfs;
    int ret;
    loff_t pos;

    oldfs = get_fs();
    set_fs(get_ds());

    pos = file->f_pos;
    ret = vfs_read(file, data, size, &pos);
    file->f_pos = pos;
    //printk(KERN_INFO"VITAMINS read %u bytes. data = %u, ret=%d, off=%llu\n",size,*data,ret,pos);
    set_fs(oldfs);

    return ret;
}
static inline void native_file_rd(native_file_t a_file, char *ptr, int count){
	BUG_ON(a_file==NULL);
	_native_file_rd(a_file,ptr,count);
}
static inline void native_file_wr(native_file_t a_file, const char *ptr, int count){
	printk(KERN_INFO"Writing files from the kernel is not supported\n");
}

#endif


static inline bool is_big_endian(uint32_t ck) { char *aux = (char *)&ck;  return (aux[0] == 1) && (aux[3] == 4); }
static inline uint32_t set_big_endian(void) { return 0x01020304; }

static bool machine_big_endian;

vfile_t open_file_rd(const char* filepath)
{
    vfile_t f;
    f.native_file = native_open_file_rd(filepath);
    if(f.native_file != NULL){
    	uint32_t ck;
    	native_file_rd((native_file_t)f.native_file,(char*)&ck,sizeof(uint32_t));
    	f.big_endian = is_big_endian(ck);

    	ck = set_big_endian();
    	machine_big_endian = is_big_endian(ck);
    }
    return f;
}
vfile_t open_file_wr(const char* filepath)
{
    vfile_t f;
    f.native_file = native_open_file_wr(filepath);
    if(f.native_file != NULL){
    	uint32_t ck = set_big_endian();
    	machine_big_endian = is_big_endian(ck);
    	native_file_wr((native_file_t)f.native_file,(char*)&ck,sizeof(uint32_t));
    	f.big_endian = is_big_endian(ck);
    }
    return f;
}
void close_file(vfile_t *file)
{
    BUG_ON(file==nullptr);
    BUG_ON(file->native_file==nullptr);
    native_close_file((native_file_t)file->native_file);

}
void file_rd(vfile_t *a_file, char *ptr, int count)
{
    BUG_ON(a_file==nullptr);
	native_file_rd((native_file_t)a_file->native_file,ptr,count);
}
void file_wr(vfile_t *a_file, const char *ptr, int count)
{
    BUG_ON(a_file==nullptr);
	native_file_wr((native_file_t)a_file->native_file,ptr,count);
}

static inline uint32_t reverse_uint32 (uint32_t val) {
    uint32_t result = 0;
	char *val_c = (char *)&val;
	char *result_c = (char *)&result;
	result_c[0] = val_c[3];
	result_c[1] = val_c[2];
	result_c[2] = val_c[1];
	result_c[3] = val_c[0];
    return result;
}

uint32_t file_rd_word(vfile_t *a_file)
{
    uint32_t data;
    file_rd(a_file,(char*)(&data),sizeof(uint32_t));
    if(machine_big_endian != a_file->big_endian)
    	return reverse_uint32(data);
    else
    	return data;
}
void file_wr_word(vfile_t *a_file, uint32_t data)
{
    BUG_ON(machine_big_endian != a_file->big_endian);
	file_wr(a_file,(char*)(&data),sizeof(uint32_t));
}
