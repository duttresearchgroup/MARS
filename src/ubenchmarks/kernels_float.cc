#include "kernels.h"
#include "kernels_macros.h"


void vitamins_bm_low_ilp_cache_bad_float_limited(float *workbuffer, int workbufferS, int *out, int numIterations){
	int i = 0;
	for(i = 0; i < numIterations; ++i){
		workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)] += _vitamins_bm_low_ilp_float(0);
	}
}

void vitamins_bm_low_ilp_cache_good_float_limited(float *workbuffer, int workbufferS, int *out, int numIterations){
	int i = 0;
	for(i = 0; i < numIterations; ++i){
		*out += (int)_vitamins_bm_low_ilp_float(0);
	}
}

void vitamins_bm_low_ilp_cache_bad_float(float *workbuffer, int workbufferS, int *out){
    vitamins_bm_low_ilp_cache_bad_float_limited(workbuffer, workbufferS, out,512);
}

void vitamins_bm_low_ilp_cache_good_float(float *workbuffer, int workbufferS, int *out){
	vitamins_bm_low_ilp_cache_good_float_limited(workbuffer, workbufferS, out,512);
}


#define _vitamins_bm_high_ilp_float_good_cache \
    float *f0 = (float*)&(workbuffer[0]);\
    float *f1 = (float*)&(workbuffer[1]);\
    float *f2 = (float*)&(workbuffer[2]);\
    float *f3 = (float*)&(workbuffer[3]);\
    float *f4 = (float*)&(workbuffer[4]);\
    float *f5 = (float*)&(workbuffer[5]);\
    float *f6 = (float*)&(workbuffer[6]);\
    float *f7 = (float*)&(workbuffer[7]);\
       	for(i = 0; i < numIterations; ++i){
    
#define _vitamins_bm_high_ilp_float_bad_cache \
    volatile float *f0 = 0;\
    volatile float *f1 = 0;\
    volatile float *f2 = 0;\
    volatile float *f3 = 0;\
    volatile float *f4 = 0;\
    volatile float *f5 = 0;\
    volatile float *f6 = 0;\
    volatile float *f7 = 0;\
    for(i = 0; i < numIterations; ++i){\
        f0 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f1 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f2 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f3 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f4 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f5 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f6 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);\
        f7 = &(workbuffer[vitamins_bm_mem_rnd_idx(workbufferS)]);
        


void vitamins_bm_high_ilp_cache_good_float_limited(float *workbuffer, int workbufferS, int *out, int numIterations){
    _vitamins_bm_high_ilp_float_begin
    _vitamins_bm_high_ilp_float_good_cache
    _vitamins_bm_high_ilp_end
}

void vitamins_bm_high_ilp_cache_bad_float_limited(float *workbuffer, int workbufferS, int *out, int numIterations){
    _vitamins_bm_high_ilp_float_begin
    _vitamins_bm_high_ilp_float_bad_cache
    _vitamins_bm_high_ilp_end
}

void vitamins_bm_high_ilp_cache_good_float(float *workbuffer, int workbufferS, int *out){
    vitamins_bm_high_ilp_cache_good_float_limited(workbuffer, workbufferS, out,2048);
}

void vitamins_bm_high_ilp_cache_bad_float(float *workbuffer, int workbufferS, int *out){
    vitamins_bm_high_ilp_cache_bad_float_limited(workbuffer, workbufferS, out,256);
}

