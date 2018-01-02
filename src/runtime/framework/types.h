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
// Sensing knob types
typedef enum {
	SEN_PERFCNT = 0,
	SEN_IPS,
	SEN_TOTALTIME,
	SEN_BUSYTIME,
	SEN_BEATS,
	SEN_POWER_W,
	SEN_FREQ_MHZ,
	/////////////
	SIZE_SEN_TYPES
} sensing_type ;


#endif

