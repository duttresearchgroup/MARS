/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

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
