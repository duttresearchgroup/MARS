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
#include <unistd.h>
#include <semaphore.h>

#include "cupti_wrapper.h"
#include "nvidia_counters.h"

extern const char *cur_kernel_name;
extern void *cur_kernel_ptr;
extern sem_t *results_sem;

// CUPTI_EVENT_COLLECTION_MODE_CONTINUOUS currently only supports Tesla GPUs
int cupti_event_mode = CUPTI_EVENT_COLLECTION_MODE_KERNEL; 
int cupti_verbose = 0;
int cupti_exact_kernel_duration = 0;
int metric_list[MAX_METRICS_PER_POLICY_MANAGER];
struct shm_hookcuda *cupti_shm;

CUpti_SubscriberHandle subscriber;
CUcontext context = 0;
CUdevice device = 0;
const char *metricName[MAX_METRICS_PER_POLICY_MANAGER];
CUpti_MetricID metricId[MAX_METRICS_PER_POLICY_MANAGER];
CUpti_MetricValue metricValue[MAX_METRICS_PER_POLICY_MANAGER];
MetricData_t metricData;
CUpti_EventGroupSets *passData;
int numMetrics = 0;

static uint64_t kernelDuration;
static volatile int testComplete = 1;

void cupti_init(cupti_wrapper_arg_t &arg)
{
    int deviceCount = 0;
    int major = 0, minor = 0;
    const int maxLenDeviceName = 100;
    char name[maxLenDeviceName];
    CUresult cuerr = CUDA_SUCCESS;
    pthread_t pThread;
    int err;
    metric_callback func = cupti_event_mode == CUPTI_EVENT_COLLECTION_MODE_KERNEL? 
	getMetricValueCallbackKernMode:getMetricValueCallbackContMode;

    cupti_verbose = arg.verbose;
    cupti_exact_kernel_duration = arg.exact_kernel_duration;
    numMetrics = arg.num_metrics; 
    memcpy(metric_list, arg.metrics, MAX_METRICS_PER_POLICY_MANAGER);
    cupti_shm = arg.shm;

    if (numMetrics > MAX_METRICS_PER_POLICY_MANAGER)
    {
	fprintf(stderr, "error: exceeds maximum number of metrics %d < %d\n", MAX_METRICS_PER_POLICY_MANAGER, numMetrics);
	exit(-1);
    }

    for (int i = 0 ; i < numMetrics ; ++i)
    {
	if (arg.metrics[i] < NVIDIA_COUNTERS_MAX)
	{
	    metricName[i] = NV_CTR_NAME(arg.metrics[i]);
	}
	else
	{
	    fprintf(stderr, "error: unsupported metric %d\n", arg.metrics[i]);
	    exit(-1);
	}
    }
    // make sure activity is enabled before any CUDA API
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_KERNEL));

    DRIVER_API_CALL(cuInit(0));

    // get first CUDA device
    DRIVER_API_CALL(cuDeviceGet(&device, 0));
    if (cupti_verbose)
    {
	DRIVER_API_CALL(cuDeviceGetName(name, maxLenDeviceName, device));
	cupti_printf("> Using device 0: %s\n", name);

	// get compute capabilities and the devicename
	DRIVER_API_CALL( cuDeviceComputeCapability(&major, &minor, device) );
	cupti_printf("> GPU Device has SM %d.%d compute capability\n", major, minor);
    }
    cuerr = cuCtxCreate(&context, 0, device);
    if (cuerr != CUDA_SUCCESS) 
    {
	fprintf(stderr, "* Error initializing the CUDA context.\n");
	cuCtxDetach(context);
	exit(-1);
    }	

    // setup launch callback for event collection
    CUPTI_CALL(cuptiSubscribe(&subscriber, (CUpti_CallbackFunc)func, &metricData));
    CUPTI_CALL(cuptiEnableCallback(1, subscriber, CUPTI_CB_DOMAIN_RUNTIME_API,
		CUPTI_RUNTIME_TRACE_CBID_cudaLaunch_v3020));

    if (numMetrics > 0)
    {
	if (cupti_event_mode == CUPTI_EVENT_COLLECTION_MODE_KERNEL)
	{
	    // allocate space to hold all the events needed for the metric
	    metricData.numTotalEvents = 0;
	    for (int i = 0; i < numMetrics ; ++i)
	    {
		CUPTI_CALL(cuptiMetricGetIdFromName(device, metricName[i], &metricId[i]));
		CUPTI_CALL(cuptiMetricGetNumEvents(metricId[i], &metricData.numEvents[i]));
		metricData.numTotalEvents += metricData.numEvents[i];
	    }
	    metricData.device = device;
	    metricData.eventIdArray = (CUpti_EventID *)malloc(metricData.numTotalEvents * sizeof(CUpti_EventID));
	    metricData.eventValueArray = (uint64_t *)malloc(metricData.numTotalEvents * sizeof(uint64_t));
	    metricData.eventIdx = 0;

	    // get the number of passes required to collect all the events
	    // needed for the metric and the event groups for each pass
	    CUPTI_CALL(cuptiMetricCreateEventGroupSets(context, sizeof(CUpti_MetricID) * numMetrics, metricId, &passData));
	    metricData.eventGroups = passData->sets;
	    if (passData->numSets > 1)
	    {
		fprintf(stderr, "error: too many pass %d currently not supported\n", passData->numSets);
		exit(-1);
	    }

	    CUPTI_CALL(cuptiSetEventCollectionMode(context, CUPTI_EVENT_COLLECTION_MODE_KERNEL));
	}
	else 
	{
	    CUPTI_CALL(cuptiSetEventCollectionMode(context,
			CUPTI_EVENT_COLLECTION_MODE_CONTINUOUS));
	    testComplete=0;
	    err = pthread_create(&pThread, NULL, sampling_func, NULL);
	    if (err != 0) {
		perror("pthread_create");
		exit(-1);
	    }
	}
    }
    if (cupti_exact_kernel_duration)
	CUPTI_CALL(cuptiActivityRegisterCallbacks(bufferRequested, bufferCompleted));
}


void CUPTIAPI
getMetricValueCallbackKernMode(void *userdata, CUpti_CallbackDomain domain,
	CUpti_CallbackId cbid, const CUpti_CallbackData *cbInfo)
{
    MetricData_t *metricData = (MetricData_t*)userdata;
    unsigned int i, j, k;

    // This callback is enabled only for launch so we shouldn't see
    // anything else.
    if (cbid != CUPTI_RUNTIME_TRACE_CBID_cudaLaunch_v3020) {
	printf("%s:%d: unexpected cbid %d\n", __FILE__, __LINE__, cbid);
	exit(-1);
    }

    // on entry, enable all the event groups being collected this pass,
    // for metrics we collect for all instances of the event
    if (cbInfo->callbackSite == CUPTI_API_ENTER) {
	cudaDeviceSynchronize();

	if (metricData->eventGroups->numEventGroups > 1)
	{
	    fprintf(stderr, "error: too many event groups %d currently not supported\n", 
		    (int)metricData->eventGroups->numEventGroups);
	    exit(-1);
	}
	for (i = 0; i < metricData->eventGroups->numEventGroups; i++) {
	    uint32_t all = 1;
	    CUPTI_CALL(cuptiEventGroupSetAttribute(metricData->eventGroups->eventGroups[i],
			CUPTI_EVENT_GROUP_ATTR_PROFILE_ALL_DOMAIN_INSTANCES,
			sizeof(all), &all));
	    CUPTI_CALL(cuptiEventGroupEnable(metricData->eventGroups->eventGroups[i]));
	}
    }

    // on exit, read and record event values
    if (cbInfo->callbackSite == CUPTI_API_EXIT) {

	cudaDeviceSynchronize();
	if (cupti_exact_kernel_duration)
	    CUPTI_CALL(cuptiActivityFlushAll(0));

	metricData->eventIdx = 0;

	// for each group, read the event values from the group and record in metricData
	for (i = 0; i < metricData->eventGroups->numEventGroups; i++) {
	    CUpti_EventGroup group = metricData->eventGroups->eventGroups[i];
	    CUpti_EventDomainID groupDomain;
	    uint32_t numEvents, numInstances, numTotalInstances;
	    CUpti_EventID *eventIds;
	    size_t groupDomainSize = sizeof(groupDomain);
	    size_t numEventsSize = sizeof(numEvents);
	    size_t numInstancesSize = sizeof(numInstances);
	    size_t numTotalInstancesSize = sizeof(numTotalInstances);
	    uint64_t *values, normalized, sum;
	    size_t valuesSize, eventIdsSize;

	    CUPTI_CALL(cuptiEventGroupGetAttribute(group,
			CUPTI_EVENT_GROUP_ATTR_EVENT_DOMAIN_ID,
			&groupDomainSize, &groupDomain));
	    CUPTI_CALL(cuptiDeviceGetEventDomainAttribute(metricData->device, groupDomain,
			CUPTI_EVENT_DOMAIN_ATTR_TOTAL_INSTANCE_COUNT,
			&numTotalInstancesSize, &numTotalInstances));
	    CUPTI_CALL(cuptiEventGroupGetAttribute(group,
			CUPTI_EVENT_GROUP_ATTR_INSTANCE_COUNT,
			&numInstancesSize, &numInstances));
	    CUPTI_CALL(cuptiEventGroupGetAttribute(group,
			CUPTI_EVENT_GROUP_ATTR_NUM_EVENTS,
			&numEventsSize, &numEvents));
	    eventIdsSize = numEvents * sizeof(CUpti_EventID);
	    eventIds = (CUpti_EventID *)malloc(eventIdsSize);
	    CUPTI_CALL(cuptiEventGroupGetAttribute(group,
			CUPTI_EVENT_GROUP_ATTR_EVENTS,
			&eventIdsSize, eventIds));

	    metricData->numActualEvents = numEvents;
	    valuesSize = sizeof(uint64_t) * numInstances;
	    values = (uint64_t *)malloc(valuesSize);

	    for (j = 0; j < numEvents; j++) {
		CUPTI_CALL(cuptiEventGroupReadEvent(group, CUPTI_EVENT_READ_FLAG_NONE,
			    eventIds[j], &valuesSize, values));

		// sum collect event values from all instances
		sum = 0;
		for (k = 0; k < numInstances; k++)
		    sum += values[k];

		// normalize the event value to represent the total number of
		// domain instances on the device
		normalized = (sum * numTotalInstances) / numInstances;

		metricData->eventIdArray[metricData->eventIdx] = eventIds[j];
		metricData->eventValueArray[metricData->eventIdx] = normalized;
		metricData->eventIdx++;

		// print collected value
		{
		    char eventName[128];
		    size_t eventNameSize = sizeof(eventName) - 1;
		    CUPTI_CALL(cuptiEventGetAttribute(eventIds[j], CUPTI_EVENT_ATTR_NAME,
				&eventNameSize, eventName));
		    eventName[127] = '\0';
		    cupti_printf("\t%s = %llu (", eventName, (unsigned long long)sum);
		    if (numInstances > 1) {
			for (k = 0; k < numInstances; k++) {
			    if (k != 0)
				cupti_printf(", ");
			    cupti_printf("%llu", (unsigned long long)values[k]);
			}
		    }

		    cupti_printf(")\n");
		    cupti_printf("\t%s (normalized) (%llu * %u) / %u = %llu\n",
			    eventName, (unsigned long long)sum,
			    numTotalInstances, numInstances,
			    (unsigned long long)normalized);
		}
	    }

	    free(values);
	    free(eventIds);
	}

	for (i = 0; i < metricData->eventGroups->numEventGroups; i++)
	    CUPTI_CALL(cuptiEventGroupDisable(metricData->eventGroups->eventGroups[i]));
	get_metric_values();
	write_metric_values();	
	print_metric_values();
    }
}

void CUPTIAPI
getMetricValueCallbackContMode(void *userdata, CUpti_CallbackDomain domain,
	CUpti_CallbackId cbid, const CUpti_CallbackData *cbInfo)
{
    MetricData_t *metricData = (MetricData_t*)userdata;
    unsigned int i, j, k;

    // This callback is enabled only for launch so we shouldn't see
    // anything else.
    if (cbid != CUPTI_RUNTIME_TRACE_CBID_cudaLaunch_v3020) {
	printf("%s:%d: unexpected cbid %d\n", __FILE__, __LINE__, cbid);
	exit(-1);
    }

    // on entry, enable all the event groups being collected this pass,
    // for metrics we collect for all instances of the event
    if (cbInfo->callbackSite == CUPTI_API_ENTER) {
	testComplete = 0;
    }

    // on exit, read and record event values
    if (cbInfo->callbackSite == CUPTI_API_EXIT) {
	cudaDeviceSynchronize();
	testComplete = 1;
    }
}

void cupti_exit()
{
    free(metricData.eventIdArray);
    free(metricData.eventValueArray);
    cuCtxDestroy(context);
}

void get_metric_values()
{
    for (int i = 0 ; i < numMetrics ; ++i) 
    {
	// use all the collected events to calculate the metric value
	CUPTI_CALL(cuptiMetricGetValue(device, metricId[i],
		    metricData.numActualEvents * sizeof(CUpti_EventID),
		    metricData.eventIdArray,
		    metricData.numActualEvents * sizeof(uint64_t),
		    metricData.eventValueArray,
		    kernelDuration, &metricValue[i]));
    }
}

void write_metric_values()
{
    int i, target=-1;

    sem_wait (results_sem);

    for (i = 0 ; i < MAX_KERNEL_PER_POLICY_MANAGER ; ++i)
    {
	if (cur_kernel_ptr == cupti_shm->kernels[i].ptr)
	{
	    target = i;
	    break;
	}
    }

    if (target < 0)
    {
	target = cupti_shm->num_kernels;
	strcpy(cupti_shm->kernels[target].name, cur_kernel_name);
	cupti_shm->kernels[target].ptr = cur_kernel_ptr;

	cupti_shm->num_kernels++;
	if (cupti_shm->num_kernels >= MAX_KERNEL_PER_POLICY_MANAGER)
	{
	    printf("WARNING: # of GPU kernels exceeds MAX_KERNEL_PER_POLICY_MANAGER=%d\n", MAX_KERNEL_PER_POLICY_MANAGER);
	    exit(-1);
	}
    }
    cupti_printf("target = %d\n", target);
    cupti_shm->kernels[target].num_launched++;
    cupti_printf("num_launched = %d\n", cupti_shm->kernels[target].num_launched);

    cupti_shm->kernels[target].pure_kernel_duration = kernelDuration;
    cupti_printf("kernel duration: %lf ms\n", kernelDuration/1000000.0);

    for (i = 0 ; i < numMetrics ; ++i) 
    {
	CUpti_MetricValueKind valueKind;
	size_t valueKindSize = sizeof(valueKind);
	CUPTI_CALL(cuptiMetricGetAttribute(metricId[i], CUPTI_METRIC_ATTR_VALUE_KIND,
		    &valueKindSize, &valueKind));
	cupti_printf("metric_list[%d]=%d\n", i, metric_list[i]);

	switch (valueKind) {
	    case CUPTI_METRIC_VALUE_KIND_DOUBLE:
		cupti_printf("metric_type double\n");
		cupti_shm->kernels[target].mres[metric_list[i]] += (double)metricValue[i].metricValueDouble;
		break;
	    case CUPTI_METRIC_VALUE_KIND_UINT64:
		cupti_printf("metric_type uint64\n");
		cupti_shm->kernels[target].mres[metric_list[i]] += (double)metricValue[i].metricValueUint64;
		break;
	    case CUPTI_METRIC_VALUE_KIND_INT64:
		cupti_printf("metric_type int64\n");
		cupti_shm->kernels[target].mres[metric_list[i]] += (double)metricValue[i].metricValueUint64;
		break;
	    case CUPTI_METRIC_VALUE_KIND_PERCENT:
		cupti_printf("pmetric_type ercent\n");
		cupti_shm->kernels[target].mres[metric_list[i]] += (double)metricValue[i].metricValuePercent;
		break;
	    case CUPTI_METRIC_VALUE_KIND_THROUGHPUT:
		cupti_printf("metric_type throughput\n");
		cupti_shm->kernels[target].mres[metric_list[i]] += (double)metricValue[i].metricValueThroughput;
		break;
	    case CUPTI_METRIC_VALUE_KIND_UTILIZATION_LEVEL:
		cupti_printf("metric_type utilization level\n");
		// not implemented yet
		break;
	    default:
		fprintf(stderr, "error: unknown value kind\n");
		exit(-1);
	}
	cupti_printf("mres: %lf\n", cupti_shm->kernels[target].mres[metric_list[i]]);
    }

    if (sem_post (results_sem) == -1)
	printf("sem_post: metrics_sem\n");
}

void print_metric_values()
{
    for (int i = 0 ; i < numMetrics ; ++i) 
    {
	// print metric value, we format based on the value kind
	{
	    CUpti_MetricValueKind valueKind;
	    size_t valueKindSize = sizeof(valueKind);
	    CUPTI_CALL(cuptiMetricGetAttribute(metricId[i], CUPTI_METRIC_ATTR_VALUE_KIND,
			&valueKindSize, &valueKind));
	    switch (valueKind) {
		case CUPTI_METRIC_VALUE_KIND_DOUBLE:
		    cupti_printf("Metric %s = %f\n", metricName[i], metricValue[i].metricValueDouble);
		    break;
		case CUPTI_METRIC_VALUE_KIND_UINT64:
		    cupti_printf("Metric %s = %llu\n", metricName[i],
			    (unsigned long long)metricValue[i].metricValueUint64);
		    break;
		case CUPTI_METRIC_VALUE_KIND_INT64:
		    cupti_printf("Metric %s = %lld\n", metricName[i],
			    (long long)metricValue[i].metricValueInt64);
		    break;
		case CUPTI_METRIC_VALUE_KIND_PERCENT:
		    cupti_printf("Metric %s = %f%%\n", metricName[i], metricValue[i].metricValuePercent);
		    break;
		case CUPTI_METRIC_VALUE_KIND_THROUGHPUT:
		    cupti_printf("Metric %s = %llu bytes/sec\n", metricName[i],
			    (unsigned long long)metricValue[i].metricValueThroughput);
		    break;
		case CUPTI_METRIC_VALUE_KIND_UTILIZATION_LEVEL:
		    cupti_printf("Metric %s = utilization level %u\n", metricName[i],
			    (unsigned int)metricValue[i].metricValueUtilizationLevel);
		    break;
		default:
		    fprintf(stderr, "error: unknown value kind\n");
		    exit(-1);
	    }
	}
    }
}

static void CUPTIAPI
bufferRequested(uint8_t **buffer, size_t *size, size_t *maxNumRecords)
{
  uint8_t *rawBuffer;

  cupti_printf("buffer_requested\n");

  *size = 16 * 1024;
  rawBuffer = (uint8_t *)malloc(*size + ALIGN_SIZE);

  *buffer = ALIGN_BUFFER(rawBuffer, ALIGN_SIZE);
  *maxNumRecords = 0;

  if (*buffer == NULL) {
    printf("Error: out of memory\n");
    exit(-1);
  }
}

static void CUPTIAPI
bufferCompleted(CUcontext ctx, uint32_t streamId, uint8_t *buffer, size_t size, size_t validSize)
{
  CUpti_Activity *record = NULL;
  CUpti_ActivityKernel3 *kernel;

  cupti_printf("buffer_completed\n");

  //since we launched only 1 kernel, we should have only 1 kernel record
  CUPTI_CALL(cuptiActivityGetNextRecord(buffer, validSize, &record));

  kernel = (CUpti_ActivityKernel3 *)record;
  if (kernel->kind != CUPTI_ACTIVITY_KIND_KERNEL) {
    fprintf(stderr, "Error: expected kernel activity record, got %d\n", (int)kernel->kind);
    exit(-1);
  }

  // Measures only the kernel execution in nano sec
  // Beware that it does not include kernel launching overhead which seems to be around 5ms on Jetson TX2 platform.
  // In order to measure kernel launching over head wrap around the GPU kernel call with time measurement from the host
  // or use cudaEvent.
  kernelDuration = kernel->end - kernel->start;
  cupti_printf("kernel duration: %lf ms\n", kernelDuration/1000000.0);
  free(buffer);
}

void* sampling_func(void *arg)
{
    CUptiResult cuptiErr;
    CUpti_EventGroup eventGroup;
    CUpti_EventID eventId;
    size_t bytesRead;
    uint64_t eventVal;
//  static const char *eventName = "inst_control";
//  static const char *eventName = "dram_utilization";
//  static const char *eventName = "inst_issued";
//  static const char *eventName = "inst_integer";
//  static const char *eventName = "tex0_cache_sector_queries";

//  static const char *eventName = "inst_executed";
    static const char *eventName = "local_load";
//  static const char *eventName = "generic_load";
//  static const char *eventName = "elapsed_cycles_sm";
//  static const char *eventName = "divergent_branch";
    const int SAMPLE_PERIOD_MS = 50;

    DRIVER_API_CALL(cuInit(0));
    DRIVER_API_CALL(cuDeviceGet(&device, 0));
    cuCtxCreate(&context, 0, device);

    CHECK_CUPTI_ERROR(cuptiErr, "cuptiSetEventCollectionMode");

    cuptiErr = cuptiEventGroupCreate(context, &eventGroup, 0);
    CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGroupCreate"); 

    cuptiErr = cuptiEventGetIdFromName(device, eventName, &eventId);
    CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGetIdFromName");

    cuptiErr = cuptiEventGroupAddEvent(eventGroup, eventId);
    CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGroupAddEvent");

    uint32_t all = 1;
    cuptiErr = cuptiEventGroupSetAttribute(eventGroup,
	    CUPTI_EVENT_GROUP_ATTR_PROFILE_ALL_DOMAIN_INSTANCES,
	    sizeof(all), &all);
    CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGroupSetAttribute");

    cuptiErr = cuptiEventGroupEnable(eventGroup);
    CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGroupEnable");

    while (!testComplete) {
	bytesRead = sizeof(eventVal);
	cuptiErr = cuptiEventGroupReadEvent(eventGroup,
		CUPTI_EVENT_READ_FLAG_NONE,
		eventId, &bytesRead, &eventVal);
	CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGroupReadEvent");
	if (bytesRead != sizeof(eventVal)) {
	    printf("Failed to read value for \"%s\"\n", eventName);
	    exit(-1);
	}

	if (eventVal != 0)
	    cupti_printf("%s: %llu\n", eventName, (unsigned long long)eventVal);

	usleep(SAMPLE_PERIOD_MS * 1000);
    }

    cuptiErr = cuptiEventGroupDisable(eventGroup);
    CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGroupDisable");

    cuptiErr = cuptiEventGroupDestroy(eventGroup);
    CHECK_CUPTI_ERROR(cuptiErr, "cuptiEventGroupDestroy");

    return NULL;
}

