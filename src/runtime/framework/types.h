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

#ifndef __arm_rt_actutation_types_h
#define __arm_rt_actutation_types_h

#include <vector>
#include <base/base.h>
#include <runtime/common/time_aggregator.h>
#include <runtime/interfaces/common/performance_data.h>
#include <external/hookcuda/nvidia_counters.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
// Actuation knob types
enum ActuationType {
	ACT_NULL = 0,
	ACT_FREQ_MHZ,
	ACT_FREQ_GOV,
	ACT_ACTIVE_CORES,
	ACT_TASK_MAP,

	//Dummy type for testing.
	//Actuation values for these types are just stored in memory and
	//have no effect in the system. These actuators are defined only for
	//the null resource type
	ACT_DUMMY1,
	ACT_DUMMY2,
	//////////////////////////
	//////////////////////////
	SIZE_ACT_TYPES
};

// This struct has information
// describing each actuation knob type.
// Some default information is provided, but
// this struct should be specialized for every
// knob type.
template <ActuationType T>
struct ActuationTypeInfo {
	//the data type of the actuation knob value
	using ValType = double;

	// Type that defines ranges for actuation values
	// Can be void if it doesn't make sense for that
	// particular actuation knob
	using Ranges = void;

	// The aggregator is used to combine multiple actuation setting when
	// predicting values for a sensing window. For instance, in a 50ms
	// window the frequency is initially 100MHz, then set to 200MHz at time
	// 20, then set to 300MHz at time 40. At time 50, the avg. frequency in the
	// 50ms window would have been 180MHz. The ContinuousAggregator provides
	// this average number. For actutuation types that cannot be averaged-out
	// DiscreateAggregator should be used to get the more frequent value
	using AggregatorType = void;
};
//of course this template instantiation is invalid
template <> struct ActuationTypeInfo<SIZE_ACT_TYPES>;

//Now the knob types values specializations

template <> struct ActuationTypeInfo<ACT_FREQ_MHZ>{
    using ValType = int; //integer value in MHz

    struct Ranges {
        int min;
        int max;
        int steps;
    };

    using AggregatorType = ContinuousAggregator<ValType>;
};

template <> struct ActuationTypeInfo<ACT_FREQ_GOV>{
    using ValType = std::string; //Name of the governor

    typedef ActuationTypeInfo<ACT_FREQ_MHZ>::Ranges Ranges;

    using AggregatorType = DiscreteAggregator<ValType>;
};

template <> struct ActuationTypeInfo<ACT_ACTIVE_CORES>{
    using ValType = int;

    struct Ranges {
        int min;
        int max;
        static constexpr int steps = 1;
    };

    using AggregatorType = ContinuousAggregator<ValType>;
};

template <> struct ActuationTypeInfo<ACT_TASK_MAP>{
    using ValType = core_info_t*;

    struct Ranges {
        int min;
        int max;
        static constexpr int steps = 1;
    };

    using AggregatorType = DiscreteAggregator<ValType>;
};

template <> struct ActuationTypeInfo<ACT_DUMMY1>{
    using ValType = int;

    struct Ranges {
        int min;
        int max;
        static constexpr int steps = 1;
    };

    using AggregatorType = ContinuousAggregator<ValType>;
};

template <> struct ActuationTypeInfo<ACT_DUMMY2>{
    using ValType = int;

    struct Ranges {
        int min;
        int max;
        static constexpr int steps = 1;
    };

    using AggregatorType = ContinuousAggregator<ValType>;
};

//////////////////////////////////////////////////////////////////////////////

class GpuPerfCtrRes {
public:
    int num_kernels;
    kernel_data_t kernels[MAX_KERNEL_PER_POLICY_MANAGER];

    GpuPerfCtrRes() {
	num_kernels = 0;
	memset(kernels, 0, sizeof(kernel_data_t) * MAX_KERNEL_PER_POLICY_MANAGER);
    }

    GpuPerfCtrRes(int x) {
	if (x != 0)
	    printf("WARNING: Possible wrong assignment, GpuPerfCtrRes=%d\n", x);
	num_kernels = 0;
	memset(kernels, 0, sizeof(kernel_data_t) * MAX_KERNEL_PER_POLICY_MANAGER);
    }

    // below operators are mostly used in sensor.h
    GpuPerfCtrRes operator + (GpuPerfCtrRes const &rhs)  {
	GpuPerfCtrRes res;

	//TODO: detect & remove empty holes in the array(should not happen anyway) 
	for (int i = 0 ;  i < MAX_KERNEL_PER_POLICY_MANAGER ; ++i)
	{
	    if (kernels[i].ptr == rhs.kernels[i].ptr)
	    {
		res.kernels[i].num_launched = kernels[i].num_launched + rhs.kernels[i].num_launched;
		res.kernels[i].pure_kernel_duration = kernels[i].pure_kernel_duration + rhs.kernels[i].pure_kernel_duration;
		res.kernels[i].kernel_duration = kernels[i].kernel_duration + rhs.kernels[i].kernel_duration;
		for (int j = 0 ; j < MAX_METRICS ; ++j)
		    res.kernels[i].mres[j] = kernels[i].mres[j] + rhs.kernels[i].mres[j];
	    }
	    else if (rhs.kernels[i].ptr != NULL)
	    {
		if (kernels[i].ptr == NULL)
		    res.kernels[i] = rhs.kernels[i];
		else
		{
		    printf("WARNING: Conflict occurred during GpuPerfCtrRes + operation iter#%d\n", i);
		    fflush(stdout);
		}
	    }
	    else if (kernels[i].ptr != NULL)
	    {
		res.kernels[i] = kernels[i];
	    }
	    else
		break;
	}

	for (int i = 0 ;  i < MAX_KERNEL_PER_POLICY_MANAGER ; ++i)
	{
	    if (res.kernels[i].ptr != NULL)
		res.num_kernels++;
	    else
		break;
	}
	return res;
    }

    // ignore += int. doesn't quite fit
    GpuPerfCtrRes & operator += (int val)  
    {
	return *this;
    }

    //TODO: detect & remove empty holes in the array(should not happen anyway) 
    GpuPerfCtrRes & operator += (GpuPerfCtrRes const &rhs)  
    {
	for (int i = 0 ;  i < MAX_KERNEL_PER_POLICY_MANAGER ; ++i)
	{
	    if (kernels[i].ptr == rhs.kernels[i].ptr)
	    {
		kernels[i].num_launched += rhs.kernels[i].num_launched;
		kernels[i].pure_kernel_duration += rhs.kernels[i].pure_kernel_duration;
		kernels[i].kernel_duration += rhs.kernels[i].kernel_duration;
		for (int j = 0 ; j < MAX_METRICS ; ++j)
		    kernels[i].mres[j] += rhs.kernels[i].mres[j];
	    }
	    else if (rhs.kernels[i].ptr != NULL)
	    {
		if (kernels[i].ptr == NULL)
		    kernels[i] = rhs.kernels[i];
		else
		{
		    printf("WARNING: Conflict occurred during GpuPerfCtrRes += operation iter#%d\n", i);
		    fflush(stdout);
		}
	    }
	    else
		break;	
	}

	num_kernels = 0;
	for (int i = 0 ;  i < MAX_KERNEL_PER_POLICY_MANAGER ; ++i)
	{
	    if (kernels[i].ptr != NULL)
		num_kernels++;
	    else
		break;
	}

	return *this;
    }
};

//////////////////////////////////////////////////////////////////////////////
// Sensing data types
enum SensingType {
    //Performance stuff
	SEN_PERFCNT = 0,
	SEN_TOTALTIME_S,
	SEN_BUSYTIME_S,
	SEN_NIVCSW,
	SEN_NVCSW,
	SEN_BEATS,
	SEN_BEATS_TGT,
	SEN_FREQ_MHZ,
	SEN_LASTCPU,

	// Other sensing data
	SEN_POWER_W,
	SEN_TEMP_C,

	// NVIDIA GPU counters
	SEN_NV_GPU_PERFCNT,

	//Dummy sensed type for testing.
	//It's value is the avg. value of ACT_DUMMY1+ACT_DUMMY2 in the sensing window
	SEN_DUMMY,

	/////////////
	SIZE_SEN_TYPES
};

// How to aggregate SensingType values.
enum SensingAggType {
    //Performance stuff
    SEN_AGG_SUM = 0,//Aggregation is the sum of all values
    SEN_AGG_MEAN,//Aggregation is the mean of all values
    SEN_AGG_INT_COUNT,//Values converted to ints and the aggregated value
                      //equals the most frequent value
    /////////////
    SIZE_SEN_AGG
};

// This struct has information
// describing each sensinng type.
// Some default information is provided, but
// this struct should be specialized type.
template <SensingType T>
struct SensingTypeInfo {
    // The data type of the sensing value
    // Default is double
    using ValType = double;

    // ParamType. Used by SEN_PERFCNT to
    // specify which counter we are reading,
    // and by SEN_BEATS to specify beats domain.
    // For most SEN_* this won't be used and
    // is void
    using ParamType = void;

    // How we should aggregate values of this type
    static constexpr SensingAggType agg = SIZE_SEN_AGG;

    // string name of this type
    static const std::string str;
};
//of course this template instantiation is invalid
template <> struct SensingTypeInfo<SIZE_SEN_TYPES>;

// Now the knob types values specializations.
// For C++11 reasons we still have to define the string
// representation in a separate file (types_str.cc).

template <> struct SensingTypeInfo<SEN_PERFCNT>{
    using ValType = uint64_t; //number of events
    using ParamType = perfcnt_t; //which perf. counter
    static constexpr SensingAggType agg = SEN_AGG_SUM;
    static const std::string str;
};

template <> struct SensingTypeInfo<SEN_TOTALTIME_S>{
    using ValType = double; // total time elapsed in s
    using ParamType = void;
    static constexpr SensingAggType agg = SEN_AGG_SUM;
    static const std::string str;
};

template <> struct SensingTypeInfo<SEN_BUSYTIME_S>{
    using ValType = double; // total time the cpu was busy in s
    using ParamType = void;
    static constexpr SensingAggType agg = SEN_AGG_SUM;
    static const std::string str;
};

template <> struct SensingTypeInfo<SEN_BEATS>{
    using ValType = unsigned int; // number of heartbeats issued
    using ParamType = unsigned int; //beats domain
    static constexpr SensingAggType agg = SEN_AGG_SUM;
    //nasty workaround to get a const str for diff beats domains
    static const std::string str,str0,str1,str2,str3,str4;
};

template <> struct SensingTypeInfo<SEN_BEATS_TGT>{
    using ValType = unsigned int; // avg target heartbeat
    using ParamType = unsigned int; //beat domain
    static constexpr SensingAggType agg = SEN_AGG_MEAN;
    //nasty workaround to get a const str for diff beats domains
    static const std::string str,str0,str1,str2,str3,str4;
};

template <> struct SensingTypeInfo<SEN_NIVCSW>{
    using ValType = unsigned int; // number of involuntary ctx switches
    using ParamType = void;
    static constexpr SensingAggType agg = SEN_AGG_SUM;
    static const std::string str;
};
template <> struct SensingTypeInfo<SEN_NVCSW>{
    using ValType = unsigned int; // number of voluntary ctx switches
    using ParamType = void;
    static constexpr SensingAggType agg = SEN_AGG_SUM;
    static const std::string str;
};

template <> struct SensingTypeInfo<SEN_POWER_W>{
    using ValType = double; // average power in W
    using ParamType = void;
    static constexpr SensingAggType agg = SEN_AGG_MEAN;
    static const std::string str;
};

template <> struct SensingTypeInfo<SEN_TEMP_C>{
    using ValType = double; // average temperature in C
    using ParamType = void;
    static constexpr SensingAggType agg = SEN_AGG_MEAN;
    static const std::string str;
};

template <> struct SensingTypeInfo<SEN_FREQ_MHZ>{
    using ValType = double; // average frequency in MHz
    using ParamType = void;
    static constexpr SensingAggType agg = SEN_AGG_MEAN;
    static const std::string str;
};

template <> struct SensingTypeInfo<SEN_LASTCPU>{
    using ValType = int; //last cpu used by some task
    using ParamType = void;
    static constexpr SensingAggType agg = SEN_AGG_INT_COUNT;
    static const std::string str;
};

template <> struct SensingTypeInfo<SEN_DUMMY>{
    using ValType = double; //last cpu used by some task
    using ParamType = void;
    static constexpr SensingAggType agg = SEN_AGG_MEAN;
    static const std::string str;
};

template <> struct SensingTypeInfo<SEN_NV_GPU_PERFCNT>{
    using ValType = GpuPerfCtrRes; //last cpu used by some task
    using ParamType = nvidia_counters_t;
    static constexpr SensingAggType agg = SEN_AGG_INT_COUNT;
    static const std::string str;
};

// Helper function to get the name of sen_types
template<SensingType T>
inline const std::string& sen_str(){
    return SensingTypeInfo<T>::str;
}
template<SensingType T>
inline const std::string& sen_str(int){
    return SensingTypeInfo<T>::str;
}
// Specialized for beats
template<>
inline const std::string& sen_str<SEN_BEATS>(int domain){
    switch (domain) {
    case 0: return SensingTypeInfo<SEN_BEATS>::str0;
    case 1: return SensingTypeInfo<SEN_BEATS>::str1;
    case 2: return SensingTypeInfo<SEN_BEATS>::str2;
    case 3: return SensingTypeInfo<SEN_BEATS>::str3;
    default: return SensingTypeInfo<SEN_BEATS>::str4;
    }
}
template<>
inline const std::string& sen_str<SEN_BEATS_TGT>(int domain){
    switch (domain) {
    case 0: return SensingTypeInfo<SEN_BEATS_TGT>::str0;
    case 1: return SensingTypeInfo<SEN_BEATS_TGT>::str1;
    case 2: return SensingTypeInfo<SEN_BEATS_TGT>::str2;
    case 3: return SensingTypeInfo<SEN_BEATS_TGT>::str3;
    default: return SensingTypeInfo<SEN_BEATS_TGT>::str4;
    }
}

// Same as sen_str() but takes SensingType as function param instead
// of template param. Notice this one has a higher runtime cost.
const std::string& sen_str(SensingType t);

// Helper function the get the SensingAggType using a non template param.
// This is akin to sen_str(SensingType t)
SensingAggType sen_agg(SensingType t);


//////////////////////////////////////////////////////////////////////////////
// Null resource type

//Won't be implemented anywhere since we only pass around an invelid
//ptr of this type
struct NullResource;
inline const NullResource* nullResource() { return reinterpret_cast<const NullResource*>(1234567890);}


#endif
