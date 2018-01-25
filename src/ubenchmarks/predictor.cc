
#include "predictor.h"
#include "kernels.h"

const int WORK_BUFFER_SIZE = (1*1024*1024);

int workbuffer_int[WORK_BUFFER_SIZE];
float workbuffer_float[WORK_BUFFER_SIZE];
int work_out = 0;

void high_ilp_cache_good_int(int numIterations){
    vitamins_bm_high_ilp_cache_good_int_limited(&(workbuffer_int[0]),WORK_BUFFER_SIZE,&work_out,numIterations);
}
void high_ilp_cache_bad_int(int numIterations){
    vitamins_bm_high_ilp_cache_bad_int_limited(&(workbuffer_int[0]),WORK_BUFFER_SIZE,&work_out,numIterations);
}
void low_ilp_cache_good_int(int numIterations){
    vitamins_bm_low_ilp_cache_good_int_limited(&(workbuffer_int[0]),WORK_BUFFER_SIZE,&work_out,numIterations);
}
void low_ilp_cache_bad_int(int numIterations){
    vitamins_bm_low_ilp_cache_bad_int_limited(&(workbuffer_int[0]),WORK_BUFFER_SIZE,&work_out,numIterations);
}
void low_ilp_cache_good_float(int numIterations){
    vitamins_bm_low_ilp_cache_good_float_limited(&(workbuffer_float[0]),WORK_BUFFER_SIZE,&work_out,numIterations);
}
void low_ilp_cache_bad_float(int numIterations){
    vitamins_bm_low_ilp_cache_bad_float_limited(&(workbuffer_float[0]),WORK_BUFFER_SIZE,&work_out,numIterations);
}
void high_ilp_cache_good_float(int numIterations){
    vitamins_bm_high_ilp_cache_good_float_limited(&(workbuffer_float[0]),WORK_BUFFER_SIZE,&work_out,numIterations);
}
void high_ilp_cache_bad_float(int numIterations){
    vitamins_bm_high_ilp_cache_bad_float_limited(&(workbuffer_float[0]),WORK_BUFFER_SIZE,&work_out,numIterations);
}
void low_ilp_icache_bad(int numIterations){
    vitamins_bm_low_ilp_icache_bad_limited(&(workbuffer_int[0]),WORK_BUFFER_SIZE,&work_out,numIterations);
}
void low_ilp_branches_deep(int numIterations){
    vitamins_bm_branches_deep_limited(&(workbuffer_int[0]),WORK_BUFFER_SIZE,&work_out,numIterations);
}
void matrix_mult(int numIterations){
    vitamins_bm_matrix_mult_limited(&(workbuffer_int[0]),WORK_BUFFER_SIZE,&work_out,numIterations);
}


