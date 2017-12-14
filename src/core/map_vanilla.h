#ifndef __core_vanilla_h
#define __core_vanilla_h

#include "base/base.h"

void vitamins_vanilla_map(model_sys_t *sys);
void vitamins_vanilla_shared_map(model_sys_t *sys);
void vitamins_vanilla_shared_agingaware_map(model_sys_t *sys);

void vitamins_vanilla_load_balance(model_task_t **task_list, int task_list_size, model_core_t *core_list, bool agingaware);

#endif
