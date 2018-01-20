#ifndef VITAMINS_UBENCH_KERNELS
#define VITAMINS_UBENCH_KERNELS


//Base benchmarks
void vitamins_bm_high_ilp_cache_good_int(int *workbuffer, int workbufferS, int *out);
void vitamins_bm_high_ilp_cache_bad_int(int *workbuffer, int workbufferS, int *out);
void vitamins_bm_low_ilp_cache_good_int(int *workbuffer, int workbufferS, int *out);
void vitamins_bm_low_ilp_cache_bad_int(int *workbuffer, int workbufferS, int *out);
void vitamins_bm_branches(int *workbuffer, int workbufferS, int *out);
void vitamins_bm_branches_deep(int *workbuffer, int workbufferS, int *out);
void vitamins_bm_high_ilp_cache_good_int_limited(int *workbuffer, int workbufferS, int *out,int numIterations);
void vitamins_bm_high_ilp_cache_bad_int_limited(int *workbuffer, int workbufferS, int *out,int numIterations);
void vitamins_bm_low_ilp_cache_good_int_limited(int *workbuffer, int workbufferS, int *out,int numIterations);
void vitamins_bm_low_ilp_cache_bad_int_limited(int *workbuffer, int workbufferS, int *out,int numIterations);
void vitamins_bm_branches_limited(int *workbuffer, int workbufferS, int *out,int numIterations);
void vitamins_bm_branches_deep_limited(int *workbuffer, int workbufferS, int *out,int numIterations);
void vitamins_bm_low_ilp_icache_bad(int *workbuffer, int workbufferS, int *out);
void vitamins_bm_low_ilp_icache_bad_limited(int *workbuffer, int workbufferS, int *out,int numIterations);

//Floating point
void vitamins_bm_low_ilp_cache_good_float(float *workbuffer, int workbufferS, int *out);
void vitamins_bm_low_ilp_cache_bad_float(float *workbuffer, int workbufferS, int *out);
void vitamins_bm_high_ilp_cache_good_float(float *workbuffer, int workbufferS, int *out);
void vitamins_bm_high_ilp_cache_bad_float(float *workbuffer, int workbufferS, int *out);
void vitamins_bm_low_ilp_cache_good_float_limited(float *workbuffer, int workbufferS, int *out,int numIterations);
void vitamins_bm_low_ilp_cache_bad_float_limited(float *workbuffer, int workbufferS, int *out,int numIterations);
void vitamins_bm_high_ilp_cache_good_float_limited(float *workbuffer, int workbufferS, int *out,int numIterations);
void vitamins_bm_high_ilp_cache_bad_float_limited(float *workbuffer, int workbufferS, int *out,int numIterations);


//timed versions
int vitamins_bm_high_ilp_cache_good_int_timed(int *workbuffer, int workbufferS, int *out,long int timeus);
int vitamins_bm_high_ilp_cache_bad_int_timed(int *workbuffer, int workbufferS, int *out,long int timeus);
int vitamins_bm_low_ilp_cache_good_int_timed(int *workbuffer, int workbufferS, int *out,long int timeus);
int vitamins_bm_low_ilp_cache_bad_int_timed(int *workbuffer, int workbufferS, int *out,long int timeus);
int vitamins_bm_branches_timed(int *workbuffer, int workbufferS, int *out,long int timeus);
int vitamins_bm_branches_deep_timed(int *workbuffer, int workbufferS, int *out,long int timeus);
int vitamins_bm_low_ilp_cache_good_float_timed(float *workbuffer, int workbufferS, int *out,long int timeus);
int vitamins_bm_low_ilp_cache_bad_float_timed(float *workbuffer, int workbufferS, int *out,long int timeus);
int vitamins_bm_high_ilp_cache_good_float_timed(float *workbuffer, int workbufferS, int *out,long int timeus);
int vitamins_bm_high_ilp_cache_bad_float_timed(float *workbuffer, int workbufferS, int *out,long int timeus);
int vitamins_bm_low_ilp_icache_bad_timed(int *workbuffer, int workbufferS, int *out,long int timeus);


#endif
