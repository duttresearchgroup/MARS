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

#include "core.h"

//we don't need a super uniform distribution, so this simple stuff is fine

static unsigned long long int g_seed = 555;

void
vitamins_random_seed(unsigned int seed)
{
    g_seed = seed;
}

int
vitamins_random()
{
    g_seed = (214013*(g_seed+2531011));
    return (int)((g_seed>>32)&0x7FFFFFFF);
}

int
vitamins_random_range(int min, int max) {
    if(min == max) return min;
    return min + (vitamins_random() % (max-min));
}
