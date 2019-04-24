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

// based on CUPTI samples callback_metric and event_sampling 
#ifndef __CUPT_WRAPPER_H__
#define __CUPT_WRAPPER_H__

#include <stdio.h>
#include <stdlib.h>

#include <cuda.h>
#include <builtin_types.h>
#include <cupti.h>

#include <pthread.h>
#include "nvidia_counters.h" // MAX_METRICS_PER_POLICY_MANAGER

typedef struct cupti_wrapper_arg_st {
    int verbose;
    int exact_kernel_duration;
    int sampling_mode;
    int num_metrics;
    int metrics[MAX_METRICS_PER_POLICY_MANAGER];
    struct shm_hookcuda *shm;
} cupti_wrapper_arg_t;

// User data for event collection callback
typedef struct MetricData_st {
  // the device where metric is being collected
  CUdevice device;
  // the set of event groups to collect for a pass
  CUpti_EventGroupSet *eventGroups;
  // the current number of events collected in eventIdArray and
  // eventValueArray
  uint32_t eventIdx;
  // the number of entries per metric
  uint32_t numEvents[MAX_METRICS_PER_POLICY_MANAGER];
  uint32_t numTotalEvents;
  // the number of entries in eventIdArray and eventValueArray
  // should be less than the runnig sum of numEvents(numTotalEvents)
  uint32_t numActualEvents;
  // array of event ids
  CUpti_EventID *eventIdArray;
  // array of event values
  uint64_t *eventValueArray;
} MetricData_t;

typedef void (*metric_callback)(void *, CUpti_CallbackDomain, CUpti_CallbackId, const CUpti_CallbackData *);



#define ALIGN_SIZE (8)
#define ALIGN_BUFFER(buffer, align)                                            \
  (((uintptr_t) (buffer) & ((align)-1)) ? ((buffer) + (align) - ((uintptr_t) (buffer) & ((align)-1))) : (buffer))

#define DRIVER_API_CALL(apiFuncCall)                                           \
do {                                                                           \
    CUresult _status = apiFuncCall;                                            \
    if (_status != CUDA_SUCCESS) {                                             \
        fprintf(stderr, "%s:%d: error: function %s failed with error %d.\n",   \
                __FILE__, __LINE__, #apiFuncCall, _status);                    \
        exit(-1);                                                              \
    }                                                                          \
} while (0)

#define RUNTIME_API_CALL(apiFuncCall)                                          \
do {                                                                           \
    cudaError_t _status = apiFuncCall;                                         \
    if (_status != cudaSuccess) {                                              \
        fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n",   \
                __FILE__, __LINE__, #apiFuncCall, cudaGetErrorString(_status));\
        exit(-1);                                                              \
    }                                                                          \
} while (0)

#define CUPTI_CALL(call)                                                \
  do {                                                                  \
    CUptiResult _status = call;                                         \
    if (_status != CUPTI_SUCCESS) {                                     \
      const char *errstr;                                               \
      cuptiGetResultString(_status, &errstr);                           \
      fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n", \
              __FILE__, __LINE__, #call, errstr);                       \
      exit(-1);                                                         \
    }                                                                   \
  } while (0)

#define CHECK_CUPTI_ERROR(err, cuptifunc)                       \
  if (err != CUPTI_SUCCESS)                                     \
    {                                                           \
      const char *errstr;                                       \
      cuptiGetResultString(err, &errstr);                       \
      printf ("%s:%d:Error %s for CUPTI API function '%s'.\n",  \
              __FILE__, __LINE__, errstr, cuptifunc);           \
      exit(-1);                                                 \
    }

#define cupti_printf(args...) \
    if (cupti_verbose) printf(args)

void cupti_init(cupti_wrapper_arg_t &arg);
void cupti_exit();
void* sampling_func (void *arg);
void CUPTIAPI getMetricValueCallbackKernMode (void *userdata, CUpti_CallbackDomain domain,
                       CUpti_CallbackId cbid, const CUpti_CallbackData *cbInfo);
void CUPTIAPI getMetricValueCallbackContMode (void *userdata, CUpti_CallbackDomain domain,
                       CUpti_CallbackId cbid, const CUpti_CallbackData *cbInfo);

void get_metric_values();
void write_metric_values();
void print_metric_values();

static void CUPTIAPI bufferRequested(uint8_t **buffer, size_t *size, size_t *maxNumRecords);
static void CUPTIAPI bufferCompleted(CUcontext ctx, uint32_t streamId, uint8_t *buffer, size_t size, size_t validSize);
#endif
