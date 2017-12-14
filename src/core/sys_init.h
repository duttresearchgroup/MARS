#ifndef __core_sys_init_h
#define __core_sys_init_h

#include "base/base.h"
#include "dvfs.h"
#include "power.h"

void vitamins_task_init(model_task_t *task, int id);
void vitamins_sys_task_init(model_systask_t *task, model_core_t *core);

void vitamins_freq_domain_init(model_freq_domain_t *freq_domain, freq_domain_info_t *freq_domain_info, core_freq_t initial_freq);

void vitamins_power_domain_init(model_power_domain_t *power_domain, power_domain_info_t *power_domain_info);

void vitamins_core_init(model_core_t *core, core_info_t *core_info);

#endif
