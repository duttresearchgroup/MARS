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

