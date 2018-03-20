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

#ifndef __VITAMINS_SCALING_H__
#define __VITAMINS_SCALING_H__

//we don't want to use floating point inside the kernel, so all sensors are
//multiplied by this factor and converted to int
//convert a double or int to a scaled int and vice versa
#define CONV_factor_i 10000
#define CONV_factor_d 10000.0
#define CONV_DOUBLE_scaledINT64(src_double) ((int64_t)((src_double)*CONV_factor_d))
#define CONV_DOUBLE_scaledUINT64(src_double) ((uint64_t)((src_double)*CONV_factor_d))
#define CONV_DOUBLE_scaledINT32(src_double) ((int32_t)((src_double)*CONV_factor_d))
#define CONV_DOUBLE_scaledUINT32(src_double) ((uint32_t)((src_double)*CONV_factor_d))
#define CONV_DOUBLE_scaledUINT(src_double) ((unsigned int)((src_double)*CONV_factor_d))
#define CONV_DOUBLE_scaledINT(src_double) ((int)((src_double)*CONV_factor_d))
#define CONV_DOUBLE_scaledLLINT(src_double) ((long long int)((src_double)*CONV_factor_d))
#define CONV_INTany_scaledINTany(src_int) ((__typeof__(src_int))((src_int)*CONV_factor_i))
#define CONV_scaledINTany_DOUBLE(src_int) (((double)(src_int))/CONV_factor_d)
#define CONV_scaledINTany_INTany(src_int) ((__typeof__(src_int))((src_int)/CONV_factor_i))
/*inline uint64_t _CONV_scaledINTany_FPpart(uint64_t val, int digit){
    uint64_t mods[4] = {val%(int32_t)10000,val%(int32_t)1000,val%(int32_t)100,val%(int32_t)10};
    uint64_t divs[4] = {mods[0]/(int32_t)1000,mods[1]/(int32_t)100,mods[2]/(int32_t)10,mods[3]};
    return divs[digit];
}
#define CONV_scaledINTany_FP1st(src_int) _CONV_scaledINTany_FPpart((uint64_t)(((src_int)<0)?(-1*(src_int)):(src_int)),0)
#define CONV_scaledINTany_FP2nd(src_int) _CONV_scaledINTany_FPpart((uint64_t)(((src_int)<0)?(-1*(src_int)):(src_int)),1)
#define CONV_scaledINTany_FP3rd(src_int) _CONV_scaledINTany_FPpart((uint64_t)(((src_int)<0)?(-1*(src_int)):(src_int)),2)
#define CONV_scaledINTany_FP4th(src_int) _CONV_scaledINTany_FPpart((uint64_t)(((src_int)<0)?(-1*(src_int)):(src_int)),3)
*/
/*
#define CONV_DOUBLE_scaledINT64(src_double) ((int64_t)((src_double)*16384.0))
#define CONV_DOUBLE_scaledUINT64(src_double) ((uint64_t)((src_double)*16384.0))
#define CONV_DOUBLE_scaledUINT(src_double) ((unsigned int)((src_double)*16384.0))
#define CONV_DOUBLE_scaledINT(src_double) ((int)((src_double)*16384.0))
#define CONV_DOUBLE_scaledLLINT(src_double) ((long long int)((src_double)*16384.0))
#define CONV_INTany_scaledINTany(src_int) ((src_int)<<14)
#define CONV_scaledINTany_DOUBLE(src_int) (((double)(src_int))/16384.0)
#define CONV_scaledINTany_INTany(src_int) ((src_int)>>14)
#define CONV_scaledINTany_INTanyFPpart(src_int) ((((src_int)&16383)<0)?(-((src_int)&16383)):((src_int)&16383))
 */


#endif // __M5_SOLVER_DEFS_H__
