#ifndef __core_cfs_h
#define __core_cfs_h

#include "base/base.h"

void vitamins_load_tracker_set(load_tracker_type_t t);
load_tracker_type_t vitamins_load_tracker_get(void);


uint32_t vitamins_predict_tlc(model_task_t* task,
                      uint32_t srcCoreIPS, uint32_t srcCoreUtil,
                      uint32_t tgtCoreIPS);

uint32_t vitamins_estimate_tlc(model_task_t* task);

//MUST be called whenever change the core mapping
void vitamins_load_tracker_map_changed(model_core_t *core);
uint32_t vitamins_load_tracker_task_load(model_task_t *task);
uint32_t vitamins_load_tracker_task_load_with_overhead(model_task_t *task);

static inline cfs_load_estimator_t* core_ptr_cfs(model_core_t *core){
    return &(core->load_tracking.cfs);
}
#define core_cfs(core) core_ptr_cfs(&(core))

#endif
