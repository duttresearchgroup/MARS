#ifndef __arm_rt_pal_setup_h
#define __arm_rt_pal_setup_h

#ifdef __KERNEL__
	#include "../../linux-module/core.h"
#else
	#include <core/base/base.h>
#endif

#include "../perfcnts.h"

#ifdef PLAT_DEF
//blackmagic to build an include using PLAT_DEF
#define _pIDENT(x) x
#define _pXSTR(x) #x
#define _pSTR(x) _pXSTR(x)
#define _pPATH(x,y) _pSTR(_pIDENT(x)_pIDENT(y))
#define _pFILE /defs.h
#include _pPATH(PLAT_DEF,_pFILE)

#else
#error "PLAT_DEF macro not defined"
#endif

/////////////////////////////////////////
//platform specific functions


//implemented at plat/setup_info.c
//Good for both kernel C and user C++

CBEGIN

core_arch_t pal_core_arch(int core);

bool pal_arch_has_freq(core_arch_t arch, core_freq_t freq);

void pal_setup_freq_domains_info(sys_info_t *sys);
void pal_setup_power_domains_info(sys_info_t *sys);

freq_domain_info_t * pal_core_freq_domain(int core);
power_domain_info_t * pal_core_power_domain(int core);

CEND

//These are not included in the module and implemented only for user level

#ifndef __KERNEL__

//TODO these should be platform independent and implemented elsewhere

void model_setup_freq_domains(sys_info_t *sys);
void model_setup_power_domains(sys_info_t *sys);

void model_setup_overheads(model_sys_t *sys);

#endif



#endif
