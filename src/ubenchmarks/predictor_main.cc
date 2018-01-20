
#include <stdio.h>
#include "time_util.h"

#include "predictor.h"

const int iterations_high_ilp_cache_good_int = ITER_high_ilp_cache_good_int;
const int iterations_high_ilp_cache_bad_int = ITER_high_ilp_cache_bad_int;
const int iterations_low_ilp_cache_good_int = ITER_low_ilp_cache_good_int;
const int iterations_low_ilp_cache_bad_int = ITER_low_ilp_cache_bad_int;
const int iterations_low_ilp_cache_good_float = ITER_low_ilp_cache_good_float;
const int iterations_low_ilp_cache_bad_float = ITER_low_ilp_cache_bad_float;
const int iterations_high_ilp_cache_good_float = ITER_high_ilp_cache_good_float;
const int iterations_high_ilp_cache_bad_float = ITER_high_ilp_cache_bad_float;
const int iterations_low_ilp_icache_bad = ITER_low_ilp_icache_bad;
const int iterations_low_ilp_branches_deep = ITER_low_ilp_branches_deep;

#define run_bench(bench, iter)\
    if(iter>0){\
        bench(iter);\
    }

int main(int argc, char* argv[])
{
#ifdef ITER_PRINT
	printf("Starting uBench with the following number of iterations:\n");
    printf("\t high_ilp_cache_good_int = %d\n", iterations_high_ilp_cache_good_int);
    printf("\t high_ilp_cache_bad_int = %d\n", iterations_high_ilp_cache_bad_int);
    printf("\t low_ilp_cache_good_int = %d\n", iterations_low_ilp_cache_good_int);
    printf("\t low_ilp_cache_bad_int = %d\n", iterations_low_ilp_cache_bad_int);
    printf("\t low_ilp_cache_good_float = %d\n", iterations_low_ilp_cache_good_float);
    printf("\t low_ilp_cache_bad_float = %d\n", iterations_low_ilp_cache_bad_float);
    printf("\t high_ilp_cache_good_float = %d\n", iterations_high_ilp_cache_good_float);
    printf("\t high_ilp_cache_bad_float = %d\n", iterations_high_ilp_cache_bad_float);
    printf("\t low_ilp_icache_bad = %d\n", iterations_low_ilp_icache_bad);
    printf("\t low_ilp_branches_deep = %d\n", iterations_low_ilp_branches_deep);
    
	long int t0 = vitamins_bm_time_us();
#endif
	
    run_bench(high_ilp_cache_good_int,iterations_high_ilp_cache_good_int);
    run_bench(high_ilp_cache_bad_int,iterations_high_ilp_cache_bad_int);
    run_bench(low_ilp_cache_good_int,iterations_low_ilp_cache_good_int);
    run_bench(low_ilp_cache_bad_int,iterations_low_ilp_cache_bad_int);
    run_bench(high_ilp_cache_good_float,iterations_high_ilp_cache_good_float);
    run_bench(high_ilp_cache_bad_float,iterations_high_ilp_cache_bad_float);
    run_bench(low_ilp_cache_good_float,iterations_low_ilp_cache_good_float);
    run_bench(low_ilp_cache_bad_float,iterations_low_ilp_cache_bad_float);
    run_bench(low_ilp_icache_bad,iterations_low_ilp_icache_bad);
    run_bench(low_ilp_branches_deep,iterations_low_ilp_branches_deep);
	
#ifdef ITER_PRINT
	t0 = vitamins_bm_time_us() - t0;
	printf("Finished in %ld us\n", t0);
#endif

	return 0;
}

