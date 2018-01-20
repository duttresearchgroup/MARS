#ifndef PREDICTOR_H
#define PREDICTOR_H

void high_ilp_cache_good_int(int numIterations);
void high_ilp_cache_bad_int(int numIterations);
void low_ilp_cache_good_int(int numIterations);
void low_ilp_cache_bad_int(int numIterations);
void low_ilp_cache_good_float(int numIterations);
void low_ilp_cache_bad_float(int numIterations);
void high_ilp_cache_good_float(int numIterations);
void high_ilp_cache_bad_float(int numIterations);
void low_ilp_icache_bad(int numIterations);
void low_ilp_branches_deep(int numIterations);

#endif
