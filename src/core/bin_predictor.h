#ifndef __core_bin_predictor_h
#define __core_bin_predictor_h

#include "base/base.h"


//The predictor that uses BINs

typedef uint32_t (*bin_pred_func)(task_sensed_data_t*);

typedef struct {
    uint32_t ipcActive;
    uint32_t powerActive;
} bin_pred_result_t;

struct bin_pred_struct {
    bin_pred_func metric;//the function used for this bin

    int num_of_bins;
    //stores the end of the bin
    uint32_t *bins;

    //the prediction for each bin. Only valid in the last layer, null otherwise
    bin_pred_result_t *bin_result;
    //the next bin layer. This is null for the last layer
    struct bin_pred_struct **next_layer;
};
typedef struct bin_pred_struct bin_pred_layer_t;

typedef bin_pred_layer_t ***** bin_pred_ptr_t;


//predicts stuff for all task on all core types
//always call BEFORE mapping
void vitamins_bin_predictor(model_sys_t *sys);
/*void vitamins_dumb_exynos_predictor(vitamins_sys_t *sys);
void vitamins_halfdumb_exynos_predictor(vitamins_sys_t *sys);*/

//commits pred. info once we know the next mapping (i.e task->next_mapping is set)
//always call AFTER mapping
void vitamins_bin_predictor_commit(model_sys_t *sys);

//must be called once at the begining of everything
void vitamins_bin_predictor_init(const char* filepath);

//returns true if any predictor is available for the given arch
bool vitamins_bin_predictor_exists(core_arch_t arch);

uint32_t vitamins_pred_correction(int32_t pred_correction, uint32_t predVal, uint32_t min, uint32_t max);

void vitamins_bin_predictor_error_report(model_sys_t *sys);
void vitamins_bin_predictor_task_error_reset(void);
uint32_t vitamins_bin_predictor_avg_task_ips_error(void);
uint32_t vitamins_bin_predictor_avg_task_load_error(void);
uint32_t vitamins_bin_predictor_avg_core_load_error(model_sys_t *sys);
uint32_t vitamins_bin_predictor_avg_freq_error(model_sys_t *sys);
uint32_t vitamins_bin_predictor_avg_power_error(model_sys_t *sys);


/*
 * mostly internals functions and stuff
 */

extern bin_pred_ptr_t bin_predictors_ipcpower;
extern pred_checker_task_t all_taks_error_acc;

typedef enum {
    BIN_PRED_FUNC_ID_ipcActive = 0,
    BIN_PRED_FUNC_ID_sumL1ILIDL2misses,
    BIN_PRED_FUNC_ID_LIDmissesPerInstr,
    BIN_PRED_FUNC_ID_L2missesPerInstr,
    BIN_PRED_FUNC_ID_sumLIDL2missesPerInstr,
    BIN_PRED_FUNC_ID_brMisspredrate,
    BIN_PRED_FUNC_ID_procTimeShare,
    BIN_PRED_FUNC_ID_powerActive,
    SIZE_BIN_PRED_FUNC_ID
} bin_pred_func_id;

bin_pred_func vitamins_get_bin_pred_func(bin_pred_func_id id);
const char* vitamins_get_bin_pred_func_name(bin_pred_func_id id);
bin_pred_func_id vitamins_get_bin_pred_func_id(bin_pred_func func);

bin_pred_ptr_t vitamins_bin_predictor_alloc_new(void);
void vitamins_bin_predictor_cleanup(void);

void vitamins_bin_predictor_writefile(const char* filepath, bin_pred_ptr_t pred);
void vitamins_bin_predictor_readfile(const char* filepath, bin_pred_ptr_t pred, bool print, bin_pred_ptr_t verify);







#endif
