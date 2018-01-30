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
