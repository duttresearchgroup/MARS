
#include "kernels.h"
#include "kernels_macros.h"
#include "time_util.h"
#include <big_switch.h> //specify a custom include dir that has this guy


void vitamins_bm_low_ilp_icache_bad_limited(int *workbuffer, int workbufferS, int *out, int numIterations){
    int i = 0;
    for(i = 0; i < numIterations; ++i){
        vitamins_bm_bigswitch(vitamins_bm_rnd()%vitamins_bm_bigswitch_size, &*out);
    }
}
void vitamins_bm_low_ilp_icache_bad(int *workbuffer, int workbufferS, int *out){
    vitamins_bm_low_ilp_icache_bad_limited(workbuffer, workbufferS, out,1000);
}

int vitamins_bm_low_ilp_icache_bad_timed(int *workbuffer, int workbufferS, int *out, long int timeLimit){
    long int timeBegin = vitamins_bm_time_us();
	int numIterations = 0;
	while((vitamins_bm_time_us()-timeBegin) < timeLimit){
        vitamins_bm_bigswitch(vitamins_bm_rnd()%vitamins_bm_bigswitch_size, &*out);
        ++numIterations;
	}
	return numIterations;
}

