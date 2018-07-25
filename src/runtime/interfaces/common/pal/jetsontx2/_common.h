/*******************************************************************************
 * Copyright (C) 2018 Biswadip Maity <biswadip.maity@gmail.com>
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

#ifndef __jetsontx2_common_h
#define __jetsontx2_common_h
// This file contains platform specific helper functions

#include "../defs.h"

static inline bool core_is_big(int core) {return (core == 1 || core == 2);}

// Given a core number, returns what is the architecture
// Command to see core mapping 'cat /proc/cpuinfo'

static inline core_arch_t core_to_arch_cluster(int core){
    if (core == 0 || (core > 2 && core < 6))
    {
        return COREARCH_JetsonTX2_CortexA57;
    }
    else if (core == 1 || core == 2)
    {
        return COREARCH_JetsonTX2_Denver;
    }
    else
    {
        BUG_ON("Invalid core idx");
        return SIZE_COREARCH;
    }
}

// Index freq domains
static inline int arch_cluster_frequency_sensor(core_arch_t arch){
    if      (arch == COREARCH_JetsonTX2_Denver) return 0;
    else if (arch == COREARCH_JetsonTX2_CortexA57) return 1;
    else{
        BUG_ON("Invalid arch");
        return -1;
    }
}

// Index power domains
static inline int arch_cluster_pow_sensor(core_arch_t arch){
    if      (arch == COREARCH_JetsonTX2_Denver) return 0;
    else if (arch == COREARCH_JetsonTX2_CortexA57) return 0;
    else{
        BUG_ON("Invalid arch");
        return -1;
    }
}

#endif
