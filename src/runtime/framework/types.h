#ifndef __arm_rt_actutation_types_h
#define __arm_rt_actutation_types_h

//////////////////////////////////////////////////////////////////////////////
// This struct should be reviewed. Proably need to get rid of this "modes".
// If a system actuator need to be consiedered by the models, its model
// should be registered explicitly.
typedef enum {
	//actuation values are set by the framework
	ACTMODE_FRAMEWORK = 0,

	//actuation values are set by the system (linux)
	//the framework should include model of the system policy used
	ACTMODE_SYSTEM,
	//////////////////////////
	//////////////////////////
	SIZE_ACTMODE
} actuation_mode ;
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Actuation knob types
typedef enum {
	ACT_NULL = 0,
	ACT_FREQ_MHZ,
	ACT_ACTIVE_CORES,
	//////////////////////////
	//////////////////////////
	SIZE_ACT_TYPES
} actuation_type ;

// This struct defines the data types
// that can be set for actuation knobs.
// By default all values are double.
// Specialize this class for every knob type
template <actuation_type T>
struct actuation_type_val {
  using type = double;
};
//of course this template instantiation is invalid
template <> struct actuation_type_val<SIZE_ACT_TYPES>;

//Now the knob types values specializations

template <> struct actuation_type_val<ACT_FREQ_MHZ>{
  using type = int; //integer value in MHz
};

template <> struct actuation_type_val<ACT_ACTIVE_CORES>{
  using type = int;
};

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Sensing data types
typedef enum {
	SEN_PERFCNT = 0,
	SEN_IPS,
	SEN_TOTALTIME_S,
	SEN_BUSYTIME_S,
	SEN_BEATS,
	SEN_POWER_W,
	SEN_TEMP_C,
	SEN_FREQ_MHZ,
	/////////////
	SIZE_SEN_TYPES
} sensing_type ;

// This struct defines the value type
// for each type of sensing data
// By default all values are double, but should
// specialize this class for every sensing type
template <sensing_type T>
struct sensing_type_val {
  using type = double;
};
//of course this template instantiation is invalid
template <> struct sensing_type_val<SIZE_SEN_TYPES>;

//Now the knob types values specializations

template <> struct sensing_type_val<SEN_PERFCNT>{
  using type = uint64_t; //number of events
};

template <> struct sensing_type_val<SEN_IPS>{
  using type = double; // instr. per sec
};

template <> struct sensing_type_val<SEN_TOTALTIME_S>{
  using type = double; // total time elapsed in s
};

template <> struct sensing_type_val<SEN_BUSYTIME_S>{
  using type = double; // total time the cpu was busy in s
};

template <> struct sensing_type_val<SEN_BEATS>{
  using type = unsigned int; // number of heartbeats issued
};

template <> struct sensing_type_val<SEN_POWER_W>{
  using type = double; // average power in W
};

template <> struct sensing_type_val<SEN_TEMP_C>{
  using type = double; // average temperature in C
};

template <> struct sensing_type_val<SEN_FREQ_MHZ>{
  using type = double; // average frequency in MHz
};


#endif

