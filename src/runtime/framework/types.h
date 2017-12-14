#ifndef __arm_rt_actutation_types_h
#define __arm_rt_actutation_types_h

struct NullResource;

typedef enum {
	ACT_NULL = 0,
	ACT_FREQ_MHZ,
	ACT_ACTIVE_CORES,
	//////////////////////////
	//////////////////////////
	SIZE_ACT_TYPES
} actuation_type ;

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

