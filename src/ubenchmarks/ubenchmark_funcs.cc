#include <stdlib.h>
#include <stdio.h>
#include "ubenchmark.h"
#include "kernels.h"

void exec_func(FuncType type, int iterations, int *workbuffer, int *workout){
    switch(type){
        case HIGH_IPC:
            vitamins_bm_high_ilp_cache_good_int_limited(workbuffer,WORK_BUFFER_SIZE,workout,iterations);
            return;
        case LOW_IPC_CACHE_MISS:
            vitamins_bm_high_ilp_cache_bad_int_limited(workbuffer,WORK_BUFFER_SIZE,workout,iterations);
            return;
        case LOW_IPC_LOW_ILP:
            vitamins_bm_low_ilp_cache_good_int_limited(workbuffer,WORK_BUFFER_SIZE,workout,iterations);
            return;
        default:
            printf("ERROR: Unknown function type %d\n",type);
            exit(-1);
    }
}
