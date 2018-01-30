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

