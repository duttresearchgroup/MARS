#ifndef __arm_rt_allocate_h
#define __arm_rt_allocate_h

#include <core/core.h>

//Prediciton, allocation, and remaping function defs

void vitamins_predict_allocate(model_sys_t *sys);
void vitamins_maping_test(model_sys_t *sys);
void vitamins_remap(model_sys_t *sys);
void vitamins_remap_overheadtest(model_sys_t *sys,int core0, int core1);


#endif

