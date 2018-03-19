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

/*
 * Some helper functions that do template magic
 */

#ifndef __arm_rt_meta_h
#define __arm_rt_meta_h

#include <string>
#include "types.h"

//trick to unroll the check loop using templates
template <int First, int Last>
struct static_for
{
    // TODO use lambdas instead of multiple ()'s
    // and make this generic
    bool operator()(const std::string* &val, SensingType t) const
    {
        if (First < Last)
        {
            if((SensingType)First == t){
                val = &(sen_str<(SensingType)First>());
                return true;
            }
            else
                return static_for<First+1, Last>()(val,t);
        }
        return false;
    }

    SensingAggType operator()(SensingType t) const
        {
            if (First < Last)
            {
                if(First == t){
                    return SensingTypeInfo<(SensingType)First>::agg;
                }
                else
                    return static_for<First+1, Last>()(t);
            }
            return SIZE_SEN_AGG;
        }
};

template <int N>
struct static_for<N, N>
{
    bool operator()(const std::string* &val,SensingType t) const
    { return false; }

    SensingAggType operator()(SensingType t) const
    { return SIZE_SEN_AGG; }
};


#endif
