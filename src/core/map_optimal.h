#ifndef __core_optimal_h
#define __core_optimal_h

#include "base/base.h"

void vitamins_optimal_map(model_sys_t *sys);
void vitamins_optimal_shared_map(model_sys_t *sys);
void vitamins_optimalIPS_map(model_sys_t *sys);
void vitamins_optimalIPS_shared_map(model_sys_t *sys);
void vitamins_optimal_shared_freq_map(model_sys_t *sys);
void vitamins_optimal_sparta_map(model_sys_t *sys);
void vitamins_optimal_sparta_shared_map(model_sys_t *sys);
void vitamins_optimal_energy_given_ips(model_sys_t *sys, uint32_t ips_tgt);


#endif
