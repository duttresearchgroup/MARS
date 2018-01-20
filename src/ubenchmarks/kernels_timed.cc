#include "kernels.h"
#include "kernels_macros.h"
#include "time_util.h"

int vitamins_bm_low_ilp_cache_bad_int_timed(int *workbuffer, int workbufferS, int *out, long int timeLimit){
    long int timeBegin = vitamins_bm_time_us();
	int numIterations = 0;
	while((vitamins_bm_time_us()-timeBegin) < timeLimit){
		workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)] += _vitamins_bm_low_ilp_int(0);
		++numIterations;
	}
	return numIterations;
}

int vitamins_bm_low_ilp_cache_good_int_timed(int *workbuffer, int workbufferS, int *out, long int timeLimit){
    long int timeBegin = vitamins_bm_time_us();
	int numIterations = 0;
	while((vitamins_bm_time_us()-timeBegin) < timeLimit){
		*out += _vitamins_bm_low_ilp_int(0);
		++numIterations;
	}
	return numIterations;
}

int vitamins_bm_branches_timed(int *workbuffer, int workbufferS, int *out, long int timeLimit){
    long int timeBegin = vitamins_bm_time_us();
	int numIterations = 0;
	while((vitamins_bm_time_us()-timeBegin) < timeLimit){
		vitamins_bm_branches_limited(workbuffer, workbufferS, out,256);
		numIterations+=256;
	}
	return numIterations;
}

int vitamins_bm_branches_deep_timed(int *workbuffer, int workbufferS, int *out, long int timeLimit){
    long int timeBegin = vitamins_bm_time_us();
	int numIterations = 0;
	while((vitamins_bm_time_us()-timeBegin) < timeLimit){
		vitamins_bm_branches_deep_limited(workbuffer, workbufferS, out,128);
		numIterations+=128;
	}
	return numIterations;
}

int vitamins_bm_low_ilp_cache_bad_float_timed(float *workbuffer, int workbufferS, int *out, long int timeLimit){
    long int timeBegin = vitamins_bm_time_us();
	int numIterations = 0;
	while((vitamins_bm_time_us()-timeBegin) < timeLimit){
		workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)] += _vitamins_bm_low_ilp_float(0);
		++numIterations;
	}
	return numIterations;
}

int vitamins_bm_low_ilp_cache_good_float_timed(float *workbuffer, int workbufferS, int *out, long int timeLimit){
    long int timeBegin = vitamins_bm_time_us();
	int numIterations = 0;
	while((vitamins_bm_time_us()-timeBegin) < timeLimit){
		*out += (int)_vitamins_bm_low_ilp_float(0);
		++numIterations;
	}
	return numIterations;
}


#define _vitamins_bm_high_ilp_int_good_cache_timed \
    int numIterations = 0;\
    int *f0 = (int*)&(workbuffer[0]);\
    int *f1 = (int*)&(workbuffer[1]);\
    int *f2 = (int*)&(workbuffer[2]);\
    int *f3 = (int*)&(workbuffer[3]);\
    int *f4 = (int*)&(workbuffer[4]);\
    int *f5 = (int*)&(workbuffer[5]);\
    int *f6 = (int*)&(workbuffer[6]);\
    int *f7 = (int*)&(workbuffer[7]);\
       	while((vitamins_bm_time_us()-timeBegin) < timeLimit) for(i = 0; i < 128; ++i){\
       	++numIterations;
    

#define _vitamins_bm_high_ilp_int_bad_cache_timed \
    int numIterations = 0;\
    volatile int *f0 = 0;\
    volatile int *f1 = 0;\
    volatile int *f2 = 0;\
    volatile int *f3 = 0;\
    volatile int *f4 = 0;\
    volatile int *f5 = 0;\
    volatile int *f6 = 0;\
    volatile int *f7 = 0;\
    while((vitamins_bm_time_us()-timeBegin) < timeLimit) for(i = 0; i < 16; ++i){\
        ++numIterations;\
        f0 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f1 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f2 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f3 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f4 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f5 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f6 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f7 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);

#define _vitamins_bm_high_ilp_float_good_cache_timed \
    int numIterations = 0;\
    float *f0 = (float*)&(workbuffer[0]);\
    float *f1 = (float*)&(workbuffer[1]);\
    float *f2 = (float*)&(workbuffer[2]);\
    float *f3 = (float*)&(workbuffer[3]);\
    float *f4 = (float*)&(workbuffer[4]);\
    float *f5 = (float*)&(workbuffer[5]);\
    float *f6 = (float*)&(workbuffer[6]);\
    float *f7 = (float*)&(workbuffer[7]);\
       	while((vitamins_bm_time_us()-timeBegin) < timeLimit) for(i = 0; i < 128; ++i){\
       	    ++numIterations;
    
#define _vitamins_bm_high_ilp_float_bad_cache_timed \
    int numIterations = 0;\
    volatile float *f0 = 0;\
    volatile float *f1 = 0;\
    volatile float *f2 = 0;\
    volatile float *f3 = 0;\
    volatile float *f4 = 0;\
    volatile float *f5 = 0;\
    volatile float *f6 = 0;\
    volatile float *f7 = 0;\
    while((vitamins_bm_time_us()-timeBegin) < timeLimit) for(i = 0; i < 16; ++i){\
        ++numIterations;\
        f0 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f1 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f2 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f3 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f4 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f5 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f6 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f7 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);

int vitamins_bm_high_ilp_cache_good_int_timed(int *workbuffer, int workbufferS, int *out, long int timeLimit){
    long int timeBegin = vitamins_bm_time_us();
    _vitamins_bm_high_ilp_int_begin
    _vitamins_bm_high_ilp_int_good_cache_timed
    _vitamins_bm_high_ilp_end
    return numIterations;
}

int vitamins_bm_high_ilp_cache_bad_int_timed(int *workbuffer, int workbufferS, int *out, long int timeLimit){
    long int timeBegin = vitamins_bm_time_us();
    _vitamins_bm_high_ilp_int_begin
    _vitamins_bm_high_ilp_int_bad_cache_timed
    _vitamins_bm_high_ilp_end
    return numIterations;
}

int vitamins_bm_high_ilp_cache_good_float_timed(float *workbuffer, int workbufferS, int *out, long int timeLimit){
    long int timeBegin = vitamins_bm_time_us();
    _vitamins_bm_high_ilp_float_begin
    _vitamins_bm_high_ilp_float_good_cache_timed
    _vitamins_bm_high_ilp_end
    return numIterations;
}

int vitamins_bm_high_ilp_cache_bad_float_timed(float *workbuffer, int workbufferS, int *out, long int timeLimit){
    long int timeBegin = vitamins_bm_time_us();
    _vitamins_bm_high_ilp_float_begin
    _vitamins_bm_high_ilp_float_bad_cache_timed
    _vitamins_bm_high_ilp_end
    return numIterations;
}

