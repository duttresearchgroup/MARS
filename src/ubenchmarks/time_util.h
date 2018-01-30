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

#ifndef VITAMINS_TIME_UTIL_H
#define VITAMINS_TIME_UTIL_H

#include <sys/time.h>
#include <stddef.h>


inline long int vitamins_bm_time_us(){
	struct timeval now;
	gettimeofday(&now, NULL);
	long int us_now = now.tv_sec * 1000000 + now.tv_usec;
	return us_now;
}

#endif
