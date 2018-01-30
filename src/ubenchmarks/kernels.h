/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
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
void vitamins_bm_matrix_mult_limited(int *workbuffer, int workbufferS, int *out,int numIterations);

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
