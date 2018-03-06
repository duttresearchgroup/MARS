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

#ifndef TRAINING_H
#define TRAINING_H

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
void matrix_mult(int numIterations);

#endif
