#ifndef __core_base_info_init_h
#define __core_base_info_init_h

#include "_defs.h"
#include "_dvfs.h"
#include "_power.h"

void freq_domain_info_init(freq_domain_info_t *freq_domain, int freq_domain_id, set_freq_callback set_phy_f, get_freq_callback get_phy_f);
void power_domain_info_init(power_domain_info_t *power_domain, int power_domain_id, freq_domain_info_t *freq_domain);
void core_info_init(core_info_t *core, core_arch_t arch, int core_id, freq_domain_info_t *freq_domain, power_domain_info_t *power_domain);
uint32_t sys_info_cksum(sys_info_t *info);


#endif
