/*******************************************************************************
 * Copyright (C) 2019 Saehanseul Yi <saehansy@uci.edu>
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "nvidia_counters.h"
#include "cupti_wrapper.h"

#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h> /* For O_* constants */

#include <dlfcn.h>

// TODO: make semaphore and shm application specific
static constexpr const char *SEM_METRICS_NAME		= "/sem-metrics";
static constexpr const char *SEM_RESULT_NAME		= "/sem-results";
static constexpr const char *SHARED_MEM_NAME		= "/shm-mars-hookcuda";

const char *cur_kernel_name = NULL;
void *cur_kernel_ptr = NULL;
struct shm_hookcuda *shared_mem_ptr;
sem_t *metrics_sem, *results_sem;
int fd_shm;

typedef void *(*dlsym_ptr)(void*, const char*);
typedef void *(*dlopen_ptr)(const char*, int);
typedef int (*cuLaunchKernel_ptr) (void *, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, void *, void**, void**); 

dlsym_ptr orig_dlsym = NULL;
dlopen_ptr orig_dlopen = NULL;
void* dlopen_handle = NULL;
cuLaunchKernel_ptr orig_cuLaunchKernel = NULL;

extern "C" {
    extern void *_dl_sym (void *, const char *, void *);

    void *dlsym (void *handle, const char *symbol) 
    {
	if (orig_dlsym == NULL)
	    orig_dlsym = reinterpret_cast<dlsym_ptr>(_dl_sym(RTLD_NEXT, "dlsym", reinterpret_cast<void *>(dlsym)));

	if (!strcmp(symbol, "cuLaunchKernel"))
	    return reinterpret_cast<void *>(cuLaunchKernel);
	else
	    return orig_dlsym(handle, symbol);
    }

    void *dlopen (const char *filename, int flags) 
    {
	if (orig_dlopen == NULL)
	    orig_dlopen = reinterpret_cast<dlopen_ptr>(_dl_sym(RTLD_NEXT, "dlopen", reinterpret_cast<void *>(dlopen)));
	dlopen_handle = orig_dlopen(filename, flags);
	return dlopen_handle;
    }

    CUresult cuLaunchKernel (CUfunction f, unsigned int  gridDimX, unsigned int  gridDimY, unsigned int  gridDimZ, 
	    unsigned int  blockDimX, unsigned int  blockDimY, unsigned int  blockDimZ, unsigned int  sharedMemBytes, 
	    CUstream hStream, void** kernelParams, void** extra ) 
    {
	CUresult res;
	if (orig_cuLaunchKernel == NULL)
	    orig_cuLaunchKernel = reinterpret_cast<cuLaunchKernel_ptr>(orig_dlsym(dlopen_handle, "cuLaunchKernel"));

	res = (CUresult)orig_cuLaunchKernel(f, gridDimX, gridDimY, gridDimZ, blockDimX, blockDimY, blockDimZ, 
		sharedMemBytes, hStream, kernelParams, extra);
	// avoid calling expensive CUDA APIs
	// but subject to changes in cuda toolkit
	cur_kernel_name = *(const char**)((uintptr_t)f + 8);
	cur_kernel_ptr = (void*)f;

	return res;
    }
}   // extern "C"

__attribute__((constructor))
static void ctor(void) 
{
    cupti_wrapper_arg_t arg;

    if ((fd_shm = shm_open (SHARED_MEM_NAME, O_RDWR | O_CREAT, 0660)) == -1)
	printf ("error: shm_open\n");

    if (ftruncate (fd_shm, sizeof (struct shm_hookcuda)) == -1)
	printf ("error: ftruncate\n");

    if ((shared_mem_ptr = (struct shm_hookcuda*)mmap (NULL, sizeof (struct shm_hookcuda), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
	printf ("error: mmap\n");

    if ((metrics_sem = sem_open (SEM_METRICS_NAME, 0, 0, 0)) == SEM_FAILED)
	printf ("error: sem_open\n");

    if ((results_sem = sem_open (SEM_RESULT_NAME, 0, 0, 0)) == SEM_FAILED)
	printf ("error: sem_open\n");

    sem_wait (metrics_sem);

    arg.verbose = shared_mem_ptr->verbose;
    arg.exact_kernel_duration = shared_mem_ptr->exact_kernel_duration;

    if (shared_mem_ptr->exact_kernel_duration == EVENT_COLLECTION_MODE_KERNEL)
	arg.sampling_mode = CUPTI_EVENT_COLLECTION_MODE_KERNEL;
    else if (shared_mem_ptr->exact_kernel_duration == EVENT_COLLECTION_MODE_CONTINUOUS)
	arg.sampling_mode = CUPTI_EVENT_COLLECTION_MODE_KERNEL;
    else
	arg.sampling_mode = -1;

    arg.num_metrics = shared_mem_ptr->num_metrics;
    if (shared_mem_ptr->num_metrics > MAX_METRICS_PER_POLICY_MANAGER)
	printf("error: shared_mem_ptr->num_metrics=%d exceeds %d\n", 
		shared_mem_ptr->num_metrics, MAX_METRICS_PER_POLICY_MANAGER);

    for (int i = 0 ; i < arg.num_metrics ; ++i)
	arg.metrics[i] = shared_mem_ptr->metrics[i];

#if DEBUG_HOOKCUDA
    printf("so num_metrics=%d\n", shared_mem_ptr->num_metrics);
    for (int i = 0 ; i < shared_mem_ptr->num_metrics ; ++i)
	printf("%d ", shared_mem_ptr->metrics[i]);
    printf("\n");
#endif

    if (sem_post (metrics_sem) == -1)
	printf ("erro: sem_post: init_sem\n");

    arg.shm = shared_mem_ptr;
    cupti_init(arg);
}

__attribute__((destructor))
static void dtor(void) 
{
    sem_close(metrics_sem);
    sem_close(results_sem);

    cupti_exit();
}
