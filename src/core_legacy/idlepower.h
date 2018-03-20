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

#ifndef __core_idle_power_h
#define __core_idle_power_h


void vitamins_init_idle_power(core_arch_t arch, core_freq_t freq, uint32_t power);
uint32_t arch_idle_power_scaled(core_arch_t arch, core_freq_t freq);
bool vitamins_arch_freq_available(core_arch_t arch, core_freq_t freq);
bool vitamins_arch_freq_available_nomask(core_arch_t arch, core_freq_t freq);

//limit the available frequencies to the list provided
void vitamins_arch_freq_available_set(core_arch_t arch, core_freq_t *freq_list,int freq_list_size);
core_freq_t vitamins_arch_highest_freq_available(core_arch_t arch);
void vitamins_reset_archs(void);

static inline uint32_t core_curr_idle_power_scaled(model_core_t *core) { return arch_idle_power_scaled(core->info->arch, vitamins_dvfs_get_freq(core)); }
static inline uint32_t core_idle_power_scaled(model_core_t *core, core_freq_t freq) { return arch_idle_power_scaled(core->info->arch, freq); }

void vitamins_writefile_idle_power(const char* filepath);
void vitamins_init_idle_power_fromfile(const char* filepath);

#endif
