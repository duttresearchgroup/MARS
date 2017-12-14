#ifndef __arm_rt_sense_data_h
#define __arm_rt_sense_data_h

#include <linux/dma-mapping.h>

//pointer to the sensed data struct
extern void *cached_shared_data;
extern void *uncached_coherent_shared_data;
extern dma_addr_t uncached_coherent_shared_data_handle;
extern void *uncached_noncoherent_shared_data;
extern dma_addr_t uncached_noncoherent_shared_data_handle;
extern unsigned int shared_data_page_cnt;

bool alloc_shared_data(void);
void dealloc_shared_data(void);


#endif

