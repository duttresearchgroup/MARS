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

#include <base/base.h>
#include <runtime/interfaces/common/perfcnts.h>
#include <runtime/interfaces/common/sense_defs.h>

//////////////////////////////////////////////////////////////////////////////
// This struct should be reviewed. Proably need to get rid of this "modes".
// If a system actuator need to be consiedered by the models, its model
// should be registered explicitly.
enum ActuationMode {
	//actuation values are set by the framework
	ACTMODE_FRAMEWORK = 0,

	//actuation values are set by the system (linux)
	//the framework should include model of the system policy used
	ACTMODE_SYSTEM,
	//////////////////////////
	//////////////////////////
	SIZE_ACTMODE
};
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Actuation knob types
enum ActuationType {
	ACT_NULL = 0,
	ACT_FREQ_MHZ,
	ACT_ACTIVE_CORES,
	ACT_TASK_MAP,
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
};
//of course this template instantiation is invalid
template <> struct ActuationTypeInfo<SIZE_ACT_TYPES>;

//Now the knob types values specializations

template <> struct ActuationTypeInfo<ACT_FREQ_MHZ>{
    using ValType = int; //integer value in MHz
};

template <> struct ActuationTypeInfo<ACT_ACTIVE_CORES>{
    using ValType = int;
};

template <> struct ActuationTypeInfo<ACT_TASK_MAP>{
    using ValType = const tracked_task_data_t*;
};

//////////////////////////////////////////////////////////////////////////////


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
	SEN_FREQ_MHZ,
	SEN_LASTCPU,

	//Other sensing data
	SEN_POWER_W,
	SEN_TEMP_C,

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
// Same as sen_str() but takes SensingType as function param instead
// of template param. Notice this one has a higher runtime cost.
const std::string& sen_str(SensingType t);

// Helper function the get the SensingAggType using a non template param.
// This is akin to sen_str(SensingType t)
SensingAggType sen_agg(SensingType t);

#endif
