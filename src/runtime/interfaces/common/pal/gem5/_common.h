/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * Copyright (C) 2018 Bryan Donyanavard <bdonyana@uci.edu>
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

#ifndef __gem5_common_h
#define __gem5_common_h

#include "../pal_setup.h"

//appears in both C/C++
// for now, just assume we have a single domain
CBEGIN
extern freq_domain_info_t _freq_domains_info[1];
extern bool _freq_domain_info_set;
extern power_domain_info_t _power_domains_info[1];
extern bool _power_domain_info_set;
CEND

static inline int available_freq(core_freq_t freq){
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

#endif
