#ifndef __core_base_defs_h
#define __core_base_defs_h

#include "portability.h"

//VITAMINS ARCH DEFINITIONS

#include "_scaling.h"
#include "_lists.h"


//TODO this should become platform-specific definitions as well
#define MAX_NUM_CORES 256
#define MAX_NUM_TASKS 256//must be the same

//Ordered by platform and then according to average performance (fastest first)
typedef enum {
   COREARCH_GEM5_HUGE_HUGE = 0,
   COREARCH_GEM5_HUGE_BIG,
   COREARCH_GEM5_BIG_HUGE,
   COREARCH_GEM5_BIG_BIG,
   COREARCH_GEM5_HUGE_MEDIUM,
   COREARCH_GEM5_BIG_MEDIUM,
   COREARCH_GEM5_MEDIUM_HUGE,
   COREARCH_GEM5_MEDIUM_BIG,
   COREARCH_GEM5_HUGE_LITTLE,
   COREARCH_GEM5_BIG_LITTLE,
   COREARCH_GEM5_LITTLE_HUGE,
   COREARCH_GEM5_LITTLE_BIG,
   COREARCH_GEM5_MEDIUM_MEDIUM,
   COREARCH_GEM5_MEDIUM_LITTLE,
   COREARCH_GEM5_LITTLE_MEDIUM,
   COREARCH_GEM5_LITTLE_LITTLE,

   COREARCH_Exynos5422_BIG,
   COREARCH_Exynos5422_LITTLE,

   COREARCH_Exynos5422_BIG_LITTLE,
   COREARCH_Exynos5422_BIG_MEDIUM,
   COREARCH_Exynos5422_BIG_BIG,
   COREARCH_Exynos5422_BIG_HUGE,
   COREARCH_Exynos5422_LITTLE_LITTLE,
   COREARCH_Exynos5422_LITTLE_MEDIUM,
   COREARCH_Exynos5422_LITTLE_BIG,
   COREARCH_Exynos5422_LITTLE_HUGE,

   SIZE_COREARCH
} core_arch_t;

typedef enum {
   COREFREQ_3000MHZ = 0,
   COREFREQ_2000MHZ,
   COREFREQ_1900MHZ,
   COREFREQ_1800MHZ,
   COREFREQ_1700MHZ,
   COREFREQ_1600MHZ,
   COREFREQ_1500MHZ,
   COREFREQ_1400MHZ,
   COREFREQ_1300MHZ,
   COREFREQ_1200MHZ,
   COREFREQ_1100MHZ,
   COREFREQ_1000MHZ,
   COREFREQ_0900MHZ,
   COREFREQ_0800MHZ,
   COREFREQ_0700MHZ,
   COREFREQ_0600MHZ,
   COREFREQ_0500MHZ,
   COREFREQ_0400MHZ,
   COREFREQ_0300MHZ,
   COREFREQ_0200MHZ,
   SIZE_COREFREQ,
   COREFREQ_0000MHz,//This is the power gated state.
                    //This is a special case and it's defined after SIZE_COREFREQ
                    //because we don't want to hit this case when we iterate through all frequencies
} core_freq_t ;


//currently not used
typedef enum {
   TASKTYPE_COMPUTE_BOUND = 0,
   TASKTYPE_CAPACITY_BOUND,
   SIZE_TASKTYPE
} task_type_t ;


/*
 * note:
 *  floating point is forbidden in the kernel, so
 *  *_scaled fields are all scaled by a fixed scaling factor, use:
 *
 *  CONV_scaledINTany_DOUBLE(val)
 *      to remove the scaling factor and obtain a double
 *  CONV_scaledINTany_INTany(val)
 *      to remove the scaling factor and obtain a integer
 *  CONV_DOUBLE_scaledINT32
 *  CONV_DOUBLE_scaledUINT64
 *  CONV_DOUBLE_scaledUINT
 *  CONV_DOUBLE_scaledINT
 *  CONV_DOUBLE_scaledLLINT
 *  CONV_INTany_scaledINTany
 *  CONV_scaledINTany_DOUBLE
 *  CONV_scaledINTany_INTany
 *      to get a scaled integer from a floating point
 *
 *
 */

typedef struct {
    uint32_t initial_delay_ps;
    uint32_t current_delay_ps;
    //maximum value for the delay in order to still support the maximum frequency
    uint32_t max_delay_ps;
    //(current_delay_ps/initial_delay_ps)-1
    uint32_t rel_delay_degrad;
    //should be 1 for the core with smallest rel_delay_degrad
    //should be 0 for the core with highest rel_delay_degrad
    //linear interpolation for the values in between
    uint32_t rel_delay_degrad_penalty;
} core_aging_info_t;

typedef struct {
    //all scaled
    uint32_t iTLB_missrate;
    uint32_t dTLB_missrate;
    uint32_t icache_missrate;
    uint32_t dcache_missrate;
    uint32_t l2cache_local_missrate;//misses per L1 miss
    uint32_t l2cache_global_missrate;//missese per mem reference
    uint32_t dcache_misses_perinstr;
    uint32_t l2cache_misses_perinstr;//missese per instruction
    uint32_t br_misspredrate;
    uint32_t br_misspreds_perinstr;
    uint32_t memInstr_share;
    uint32_t fpInstr_share;
    uint32_t brInstr_share;
    uint32_t ipc_active;
    uint32_t ips_active;
    uint32_t avg_freq_mhz;
    core_freq_t avg_freq;

    uint32_t proc_time_share;//if 1 task per core, then this is the core utilization / thread tlc
    uint32_t proc_time_share_avg;//exponential average across different epochs
	#define update_proc_time_share_avg(data) \
		(data).proc_time_share_avg = ((data).proc_time_share_avg + (data).proc_time_share + (data).proc_time_share) / 3

    uint32_t nivcsw;
    uint32_t nvcsw;

} task_sensed_data_t;

typedef struct {
    //scaled
    uint32_t avg_load;
    int num_tasks;

    //sum of the active cycles this task used over the latest dvfs epoch and the frequency
    //if an external governor is doing dvfs this do not need to be set
    core_freq_t last_dvfs_epoch_freq;
    uint64_t last_dvfs_epoch_sumCyclesActive;
    uint64_t last_dvfs_epoch_ips;
    double last_dvfs_epoch_avg_power;
} core_sensed_data_t;

typedef struct {
    //NOT scaled
    uint32_t avg_freqMHz;
} freq_domain_sensed_data_t;

typedef struct {
    //scaled
    uint32_t avg_power;
} power_domain_sensed_data_t;

/*
 * predictor checkers
 *
 * Used to store previously pred. values so we can check and calculated prediction error
 */
typedef struct {
	uint32_t pred_error_acc_u;
	int32_t pred_error_acc_s;
	uint32_t pred_error_cnt;
} pred_error_t;
typedef struct{
	//TODO some possible things for pred correction
	//could store the exp avg of the pred ips/load for every tgt core/freq
	// -store a number between 0-1: 0 means we are closer to the pred value; 1 means we are close to the avg (valid only when we have this avg stored)
	// - if something between 0.4-0.6 the final pred value is always the mean between the pred and avg
	// - if betwen 0.6-1 we just take the avg; if between 0-0.4 we take the pred value only
    uint32_t prev_pred_ips_active;
    uint32_t prev_pred_load;
    bool prev_pred_valid;
    pred_error_t error_ips_active;
    //int32_t 			  correction_ips_active[SIZE_COREARCH];
    int32_t 			  correction_ips_active;
    pred_error_t error_load;
    int32_t 			  correction_load;
} pred_checker_task_t;
typedef struct {
    uint32_t prev_pred_load;
    bool prev_pred_valid;
    pred_error_t error_load;
    int32_t 			  correction_load;
} pred_checker_core_t;
typedef struct {
    uint32_t prev_pred_freq;
    bool prev_pred_valid;
    pred_error_t error_freq;
    int32_t 			  correction_freq;
} pred_checker_freq_domain_t;
typedef struct {
	//TODO some stuff from vitamins_pred_checker_task_t, but store the exp avg of power for every 'avg freq'/'load range'
	//maybe add an option to tell if the power domain is equal to or is included in a frequency domain (though a freq_domain ptr in the power domain) and use that to get the frequency instead of calculating the avg freq of the domain
    uint32_t prev_pred_power;
    bool prev_pred_valid;
    pred_error_t error_power;
    int32_t 			  correction_power;
} pred_checker_power_domain_t;


//forward def usided by load trackers
struct model_core_struct;

struct load_tracking_common_struct{
    int task_cnt;

    uint32_t load;

    //usualy something between 0.95-1.0
    //multiply the estimated task load by this value when computing performance
    //to consider context switch overhead
    uint32_t task_load_overhead;

    struct model_core_struct *core;
};

typedef struct {
    //TODO the first two fields must come first and be kept updated
    struct load_tracking_common_struct common;

    uint32_t equal_share;
    uint32_t residual_share;
    uint32_t app_tlc_sum;
    uint32_t total_tlc_sum;
    uint32_t tlc_sum_residual;

    uint32_t aged_load;
} cfs_load_estimator_t;

typedef struct {
    struct load_tracking_common_struct common;
    uint32_t app_load_acc;
    uint32_t total_load_acc;
} default_load_estimator_t;


typedef union {
    struct load_tracking_common_struct common;
    //note that this is an union so the generic_load_metrics
    //are modified through the structs below

    //available tracking modes. Only one of this can be used
    default_load_estimator_t defaut;
    cfs_load_estimator_t cfs;
} core_load_tracking_t;

typedef enum {
    LT_DEFAULT,
    LT_DEFAULT_AGINGAWARE,
    LT_CFS
    //There is no LT_CFS_AGINGAWARE because cfs_load_estimator_t
    //already has a separate field for aged_load that is always set
    //and it must be used explicituly by the mapping approach.
    //This is necessary in order not to disturb the power estimation that uses
    //the common.load field (such estimations usually do not happend when using
    //LT_DEFAULT and LT_DEFAULT_AGINGAWARE)
} load_tracker_type_t;

#define LT_AGINGAWARE(lt,agingware) ((agingware) ? (lt##_AGINGAWARE) : (lt))

//forward defs used by core and task
struct model_task_struct;
struct model_systask_struct;
struct model_freq_domain_struct;
struct model_power_domain_struct;
struct freq_domain_info_struct;
struct power_domain_info_struct;
struct pred_checker_task_struct;
struct pred_checker_core_struct;

struct core_info_struct {
    //the type of the core
    core_arch_t arch;
    //the freq. the core is currently running
    struct freq_domain_info_struct *freq;
    struct power_domain_info_struct *power;

    //this cpu position
    int position;

    //Link for adding this core to a freq_domain
    define_list_addable(struct core_info_struct,freq_domain);

    //Link for adding this core to a power domain
    define_list_addable(struct core_info_struct,power_domain);

    //pointer to dynamic core data
    struct model_core_struct *this_core;
};
typedef struct core_info_struct core_info_t;

struct model_core_struct {
    //static core info
	core_info_t *info;

    //this core's system task (for keeping track of os ovrhead)
    struct model_systask_struct *systask;

    //information for tracking the core's load during the execution of the mapping phase
    //after the mapping phase this contains the estm. load for the mapping selected for
    //the next epoch
    core_load_tracking_t load_tracking;

    core_aging_info_t aging_info;

    core_sensed_data_t sensed_data;

    pred_checker_core_t pred_checker;

    //list of tasks mapped to this core
    define_vitamins_list(struct model_task_struct,mapped_tasks);

    //Default link for link for intrusive lists
    define_list_addable_default(struct model_core_struct);
};
typedef struct model_core_struct model_core_t;

//ips, power, and utilization for each possible core type and frequency
//ips is instr / uS
//power is in watts (scaled by 10000)
//tlc is between 0 and 10000 (100%)
#define def_task_metrics \
		uint32_t ips_active[SIZE_COREARCH][SIZE_COREFREQ];\
		uint32_t power_active[SIZE_COREARCH][SIZE_COREFREQ];\
		uint32_t tlc[SIZE_COREARCH][SIZE_COREFREQ]

#define INVALID_METRIC_VAL ((uint32_t)-1)

struct model_task_struct {
	int id;

    task_sensed_data_t sensed_data;

    def_task_metrics;

    //used to calculate errrors and maybe correct predicted values in a per task basis
    pred_checker_task_t pred_checker;

    //criteria for QoS
    uint32_t max_ips_watt_scaled;
    core_arch_t max_ips_watt_core_type;
    uint32_t max_ips_scaled;
    core_arch_t max_ips_core_type;
    uint32_t max_ips_sat_scaled; //threashold for IPS saturation
    int constraining;//number of core types that yield an IPS within [max_ips_sat,max_ips]

    //this task's prev,current and future core
    //always the functions in vitamins_defs_map.h to get/set these values
    model_core_t *_curr_mapping_;
    model_core_t *_next_mapping_;
    //the cpu utilization of this task. Updated every time _next_mapping_ is updated
    uint32_t _next_mapping_load;

    //Default link for link for intrusive lists
    define_list_addable_default(struct model_task_struct);

    //before the mapping phase, this link is used by the core pointed by curr_mapping
    //after the mapping phase, this link is used by the core pointed by next_mapping
    define_list_addable(struct model_task_struct,mapping);
};
typedef struct model_task_struct model_task_t;

struct model_systask_struct {
	model_core_t *core;
	def_task_metrics;
};
typedef struct model_systask_struct model_systask_t;

//system static info
typedef struct {
	core_info_t *core_list;
	int core_list_size;

	struct freq_domain_info_struct *freq_domain_list;
	int freq_domain_list_size;

	struct power_domain_info_struct *power_domain_list;
	int power_domain_list_size;
} sys_info_t;

//system dynamic info
typedef struct {
	sys_info_t *info;

	model_task_t **task_list;
	int task_list_size;

	//1-1 mapping to cores; number of tasks is info->core_list_size
	model_systask_t *core_systask_list;
} model_sys_t;


#endif
