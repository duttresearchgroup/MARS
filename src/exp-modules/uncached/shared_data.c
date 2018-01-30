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

#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include "user_if_shared.h"
#include "user_if.h"

void *cached_shared_data = 0;
void *uncached_coherent_shared_data=0;
dma_addr_t uncached_coherent_shared_data_handle;
void *uncached_noncoherent_shared_data=0;
dma_addr_t uncached_noncoherent_shared_data_handle;
unsigned int shared_data_page_cnt = 0;

static unsigned int _alloc_order;

static inline void _alloc_cached_data(void){
	cached_shared_data = (void*)__get_free_pages(GFP_KERNEL,_alloc_order);
	if(!cached_shared_data){
		pinfo("Failed to allocate cached data !!\n");
		cached_shared_data = 0;
		shared_data_page_cnt = 0;
	}
}

static inline void _alloc_uncached_data(void){
	uncached_coherent_shared_data = dma_alloc_coherent(NULL, shared_data_page_cnt*PAGE_SIZE, &uncached_coherent_shared_data_handle, GFP_KERNEL);

	if(!uncached_coherent_shared_data){
		pinfo("Failed to allocate uncached coherent data !!\n");
		uncached_coherent_shared_data = 0;
		shared_data_page_cnt = 0;
	}
}

static inline void _alloc_uncached_noncoherent_data(void){
	uncached_noncoherent_shared_data = dma_alloc_noncoherent(NULL, shared_data_page_cnt*PAGE_SIZE, &uncached_noncoherent_shared_data_handle, GFP_KERNEL);

	if(!uncached_noncoherent_shared_data){
		pinfo("Failed to allocate uncached noncoherent data !!\n");
		uncached_noncoherent_shared_data = 0;
		shared_data_page_cnt = 0;
	}
}

//allocs the memory to keep all the sensed data
//should be called first thing when the module is loaded
bool alloc_shared_data(void){
	_alloc_order = get_order(roundup_pow_of_two(SHARED_DATA_SIZE));
	shared_data_page_cnt = 1 << _alloc_order;

	_alloc_cached_data();
	if(cached_shared_data == 0) return false;

	_alloc_uncached_data();
	if(uncached_coherent_shared_data == 0) return false;

	//_alloc_uncached_noncoherent_data();
	//if(uncached_noncoherent_shared_data == 0) return false;

	return true;
}

//deallocs the memory to keep all the sensed data
//should be the last thing called when the module is unloaded
void dealloc_shared_data(void){
	if(cached_shared_data) free_pages((unsigned long)cached_shared_data,_alloc_order);

	if(uncached_coherent_shared_data)
		dma_free_coherent(NULL, shared_data_page_cnt*PAGE_SIZE, uncached_coherent_shared_data, uncached_coherent_shared_data_handle);

	if(uncached_noncoherent_shared_data)
		dma_free_noncoherent(NULL, shared_data_page_cnt*PAGE_SIZE, uncached_noncoherent_shared_data, uncached_noncoherent_shared_data_handle);

	cached_shared_data = 0;
	uncached_coherent_shared_data = 0;
	uncached_noncoherent_shared_data = 0;
	shared_data_page_cnt = 0;
}
