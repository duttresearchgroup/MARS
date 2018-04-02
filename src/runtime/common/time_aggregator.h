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

// Aggregation of values across a sensing window
// See the comments at ActuationTypeInfo (runtime/framework/types.h)
// for more details.

#ifndef __arm_rt_common_time_aggregator
#define __arm_rt_common_time_aggregator

#include <cmath>

#include <base/base.h>
#include <type_traits>

// The aggregated value is the weighted avg of all values that were set
// and the time during which they were set
template<typename T>
class ContinuousAggregator
{
    double currAvg;
    int latestValTime;
    bool set;
    T latestVal;

    template<typename _T,bool TisIntegral>
    struct __roundIfInt {
        static _T round(double val) { return std::round(val); }
    };
    template<typename _T>
    struct __roundIfInt<_T,false> {
        static _T round(double val) { return val; }
    };
    inline T roundIfInt(double val)
    { return __roundIfInt<T,std::is_integral<T>::value>::round(val); }

  public:

    ContinuousAggregator()
        :currAvg(0), latestValTime(0), set(false)
    { }

    void addValue(const T &val, int time)
    {
        assert_true(time >= latestValTime);
        assert_true(set || (time == 0));//first added value must be for time 0
        set = true;
        if(time > latestValTime)
            currAvg = ((currAvg*latestValTime) + latestVal*(time-latestValTime))/time;
        latestVal = val;
        latestValTime = time;
    }

    T agg(int time)
    {
        assert_true(set);//no value was added
        assert_true(time >= latestValTime);
        return roundIfInt(((currAvg*latestValTime) + latestVal*(time-latestValTime))/time);
    }

    const T& latest()
    {
        assert_true(set);//no value was added
        return latestVal;
    }
};

// The aggregated value is the value that was set for the longest time
template<typename T>
class DiscreteAggregator
{
    int maxTime = 0;
    int currValTime = 0;
    bool set;
    T currVal;
    T max;

  public:

    DiscreteAggregator()
        :maxTime(0), currValTime(0), set(false)
    {}

    void addValue(const T &val, int time)
    {
        assert_true(time >= currValTime);
        assert_true(set || (time == 0));//first added value must be for time 0
        set = true;
        if(time > currValTime){
            if(maxTime == 0){
                max = currVal;
                maxTime = time;
            }
            else if(max == currVal){
                maxTime += time - currValTime;
            }
            else if((time - currValTime) > maxTime){
                maxTime = time - currValTime;
                max = currVal;
            }
        }
        currVal = val;
        currValTime = time;
    }

    const T& agg(int time)
    {
        assert_true(set);//no value was added
        assert_true(time >= currValTime);
        if(max == currVal){
            return max;
        }
        else if((time - currValTime) > maxTime){
            return currVal;
        }
        else{
            return max;
        }
    }

    const T& latest()
    {
        assert_true(set);//no value was added
        return currVal;
    }
};


#endif
