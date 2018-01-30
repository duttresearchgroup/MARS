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

