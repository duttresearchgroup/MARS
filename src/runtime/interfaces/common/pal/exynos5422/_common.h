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

//appenars in both C/C++
CBEGIN
extern freq_domain_info_t _freq_domains_info[2];//0=big 1=little
extern bool _freq_domain_info_set;
extern power_domain_info_t _power_domains_info[2];//0=big 1=little
extern bool _power_domain_info_set;
CEND

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

static inline int available_freq_big(core_freq_t freq){
    switch (freq) {
		case COREFREQ_0200MHZ:
		case COREFREQ_0300MHZ:
		case COREFREQ_0400MHZ:
		case COREFREQ_0500MHZ:
		case COREFREQ_0600MHZ:
		case COREFREQ_0700MHZ:
		case COREFREQ_0800MHZ:
		case COREFREQ_0900MHZ:
		case COREFREQ_1000MHZ:
		case COREFREQ_1100MHZ:
        case COREFREQ_1200MHZ:
        case COREFREQ_1300MHZ:
        case COREFREQ_1400MHZ:
        case COREFREQ_1500MHZ:
        case COREFREQ_1600MHZ:
        case COREFREQ_1700MHZ:
        case COREFREQ_1800MHZ:
        case COREFREQ_1900MHZ:
        case COREFREQ_2000MHZ:
            return true;
        default:
            return false;
    }
}
static inline int available_freq_little(core_freq_t freq){
    switch (freq) {
		case COREFREQ_0200MHZ:
		case COREFREQ_0300MHZ:
		case COREFREQ_0400MHZ:
		case COREFREQ_0500MHZ:
		case COREFREQ_0600MHZ:
		case COREFREQ_0700MHZ:
		case COREFREQ_0800MHZ:
		case COREFREQ_0900MHZ:
        case COREFREQ_1000MHZ:
        case COREFREQ_1100MHZ:
        case COREFREQ_1200MHZ:
        case COREFREQ_1300MHZ:
        case COREFREQ_1400MHZ:
            return true;
        default:
            return false;
    }
}

#endif
