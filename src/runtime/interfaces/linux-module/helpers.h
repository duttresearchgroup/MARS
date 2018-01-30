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

#ifndef __arm_rt_helpers_h
#define __arm_rt_helpers_h

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/semaphore.h>

////////////////////////////////////////////
// helper functions

int kern_cpu_get_freq_mhz(int core);
bool kern_cpu_set_freq_mhz(int core, int freq);
bool kern_cpu_freq_isuserspace(int core);


//helper circ buffer impl
//no waits on writes; fails if it is full
typedef int circbuf_data_t;
typedef struct {
	circbuf_data_t * const buffer;
    int head;
    int tail;
    const int maxLen;
    struct semaphore *rd_sem;
    spinlock_t *lock;
} circbuf_t;
#define CIRCBUF_DEF(x,y) \
	static circbuf_data_t x##_space[y]; \
	static struct semaphore x##_rd_sem = __SEMAPHORE_INITIALIZER(x##_rd_sem, 0); \
	static spinlock_t x##_lock = __SPIN_LOCK_UNLOCKED(x##_lock); \
	static circbuf_t x = { x##_space,0,0,y,&x##_rd_sem,&x##_lock}

int circbuf_push(circbuf_t *c, circbuf_data_t data);
int circbuf_pop(circbuf_t *c, circbuf_data_t *data);


#endif

