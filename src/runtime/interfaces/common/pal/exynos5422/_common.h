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

#ifndef __exynos5422_common_h
#define __exynos5422_common_h

#include "../pal_setup.h"

static inline bool core_is_big(int core) {return core >= 4;}

static inline core_arch_t core_to_arch_cluster(int core){
    if      (core <= 3) return COREARCH_Exynos5422_LITTLE;
    else if (core <= 7) return COREARCH_Exynos5422_BIG;
    else{
        BUG_ON("Invalid core idx");
        return SIZE_COREARCH;
    }
}

//we used this to index both sensors and freq domains
static inline int arch_cluster_pow_sensor(core_arch_t arch){
    if      (arch == COREARCH_Exynos5422_BIG) return 0;
    else if (arch == COREARCH_Exynos5422_LITTLE) return 1;
    else{
        BUG_ON("Invalid arch");
        return -1;
    }
}

#endif
