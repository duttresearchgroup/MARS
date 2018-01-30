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

#include "kernels_macros.h"

unsigned long long int _vitamins_bm_rnd_seed = 0xABCDEF01;

int _vitamins_bm_low_ilp_int(int mod){
    _vitamins_bm_low_ilp_body(_vitamins_bm_low_ilp_int);
    return 0;
}

float _vitamins_bm_low_ilp_float(float mod){
    _vitamins_bm_low_ilp_body(_vitamins_bm_low_ilp_float);
    return 0;
}
