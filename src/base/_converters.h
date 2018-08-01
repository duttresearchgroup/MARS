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

#ifndef __core_base_converters_h
#define __core_base_converters_h

#include "_defs.h"
#include "_exceptions.h"

static inline uint32_t freqToValMHz_i(core_freq_t freq){
    switch (freq) {
    case COREFREQ_3000MHZ: return 3003;
    case COREFREQ_2000MHZ: return 2000;
    case COREFREQ_1900MHZ: return 1900;
    case COREFREQ_1800MHZ: return 1798;
    case COREFREQ_1700MHZ: return 1700;
    case COREFREQ_1600MHZ: return 1600;
    case COREFREQ_1500MHZ: return 1499;
    case COREFREQ_1400MHZ: return 1400;
    case COREFREQ_1300MHZ: return 1300;
    case COREFREQ_1200MHZ: return 1200;
    case COREFREQ_1100MHZ: return 1100;
    case COREFREQ_1000MHZ: return 1000;
    case COREFREQ_0900MHZ: return  900;
    case COREFREQ_0800MHZ: return  800;
    case COREFREQ_0700MHZ: return  699;
    case COREFREQ_0600MHZ: return  599;
    case COREFREQ_0500MHZ: return  500;
    case COREFREQ_0400MHZ: return  400;
    case COREFREQ_0300MHZ: return  300;
    case COREFREQ_0200MHZ: return  200;
    case COREFREQ_0000MHz: return  0;
    default:
    	BUG_ON("Invalid frequency");
    	return 0;
    }
}

static inline core_freq_t valToFreqMHz_d(double freq){
	/*if (freq >= 3000) return  COREFREQ_3000MHZ;
	else */if (freq >= 1950) return  COREFREQ_2000MHZ;
	else if (freq >= 1850) return  COREFREQ_1900MHZ;
	else if (freq >= 1750) return  COREFREQ_1800MHZ;
	else if (freq >= 1650) return  COREFREQ_1700MHZ;
	else if (freq >= 1550) return  COREFREQ_1600MHZ;
	else if (freq >= 1450) return  COREFREQ_1500MHZ;
	else if (freq >= 1350) return  COREFREQ_1400MHZ;
	else if (freq >= 1250) return  COREFREQ_1300MHZ;
	else if (freq >= 1150) return  COREFREQ_1200MHZ;
	else if (freq >= 1050) return  COREFREQ_1100MHZ;
	else if (freq >= 950) return  COREFREQ_1000MHZ;
	else if (freq >= 850) return  COREFREQ_0900MHZ;
	else if (freq >= 750) return  COREFREQ_0800MHZ;
	else if (freq >= 650) return  COREFREQ_0700MHZ;
	else if (freq >= 550) return  COREFREQ_0600MHZ;
	else if (freq >= 450) return  COREFREQ_0500MHZ;
	else if (freq >= 350) return  COREFREQ_0400MHZ;
	else if (freq >= 250) return  COREFREQ_0300MHZ;
	else return  COREFREQ_0200MHZ;
	BUG_ON("Invalid frequency");
}

#define arcToStringCase(val) case COREARCH_##val: return #val
#define freqToStringCase(val) case COREFREQ_##val: return #val

static inline const char* archToString(core_arch_t arch){
    switch (arch) {
    arcToStringCase(GEM5_HUGE_HUGE);
    arcToStringCase(GEM5_HUGE_BIG);
    arcToStringCase(GEM5_BIG_HUGE);
    arcToStringCase(GEM5_BIG_BIG);
    arcToStringCase(GEM5_HUGE_MEDIUM);
    arcToStringCase(GEM5_BIG_MEDIUM);
    arcToStringCase(GEM5_MEDIUM_HUGE);
    arcToStringCase(GEM5_MEDIUM_BIG);
    arcToStringCase(GEM5_HUGE_LITTLE);
    arcToStringCase(GEM5_BIG_LITTLE);
    arcToStringCase(GEM5_LITTLE_HUGE);
    arcToStringCase(GEM5_LITTLE_BIG);
    arcToStringCase(GEM5_MEDIUM_MEDIUM);
    arcToStringCase(GEM5_MEDIUM_LITTLE);
    arcToStringCase(GEM5_LITTLE_MEDIUM);
    arcToStringCase(GEM5_LITTLE_LITTLE);
    arcToStringCase(GEM5_GENERIC_ARM);
    arcToStringCase(JetsonTX2_Denver);
    arcToStringCase(JetsonTX2_CortexA57);
    arcToStringCase(Exynos5422_BIG);
    arcToStringCase(Exynos5422_LITTLE);
    arcToStringCase(Exynos5422_BIG_LITTLE);
    arcToStringCase(Exynos5422_BIG_MEDIUM);
    arcToStringCase(Exynos5422_BIG_BIG);
    arcToStringCase(Exynos5422_BIG_HUGE);
    arcToStringCase(Exynos5422_LITTLE_LITTLE);
    default:
    	BUG_ON("Invalid architecture");
    	return "";
    }
}

static inline const char* freqToString(core_freq_t freq){
    switch (freq) {
    freqToStringCase(3000MHZ);
    freqToStringCase(2000MHZ);
    freqToStringCase(1900MHZ);
    freqToStringCase(1800MHZ);
    freqToStringCase(1700MHZ);
    freqToStringCase(1600MHZ);
    freqToStringCase(1500MHZ);
    freqToStringCase(1400MHZ);
    freqToStringCase(1300MHZ);
    freqToStringCase(1200MHZ);
    freqToStringCase(1100MHZ);
    freqToStringCase(1000MHZ);
    freqToStringCase(0900MHZ);
    freqToStringCase(0800MHZ);
    freqToStringCase(0700MHZ);
    freqToStringCase(0600MHZ);
    freqToStringCase(0500MHZ);
    freqToStringCase(0400MHZ);
    freqToStringCase(0300MHZ);
    freqToStringCase(0200MHZ);
    freqToStringCase(0000MHz);
    default:
        BUG_ON("Invalid frequency");
        return "";
    }
}

static inline uint32_t freqToPeriodPS_i(core_freq_t freq){
    return 1000000/(freqToValMHz_i(freq));
}

#endif
