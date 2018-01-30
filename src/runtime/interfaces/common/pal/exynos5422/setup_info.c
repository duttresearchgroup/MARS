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

#include "_common.h"

freq_domain_info_t _freq_domains_info[2];//0=big 1=little
bool _freq_domain_info_set = false;
power_domain_info_t _power_domains_info[2];//0=big 1=little
bool _power_domain_info_set = false;

core_arch_t pal_core_arch(int core)
{
    return core_to_arch_cluster(core);
}

bool pal_arch_has_freq(core_arch_t arch, core_freq_t freq)
{
    if      (arch == COREARCH_Exynos5422_BIG) return available_freq_big(freq);
    else if (arch == COREARCH_Exynos5422_LITTLE) return available_freq_little(freq);
    else{
        BUG_ON("Invalid arch");
        return false;
    }
}

void pal_setup_freq_domains_info(sys_info_t *sys)
{
	int i;
	BUG_ON(_freq_domain_info_set);
	for(i = 0; i < 2; ++i){
		freq_domain_info_init(&(_freq_domains_info[i]),i,nullptr,nullptr);
	}
	sys->freq_domain_list = &(_freq_domains_info[0]);
	sys->freq_domain_list_size = 2;
	_freq_domain_info_set = true;
}

void pal_setup_power_domains_info(sys_info_t *sys)
{
	int i;
	BUG_ON(_power_domain_info_set);
	BUG_ON(!_freq_domain_info_set);
	for(i = 0; i < 2; ++i){
        power_domain_info_init(&(_power_domains_info[i]),i,&(_freq_domains_info[i]));
	}
	sys->power_domain_list = &(_power_domains_info[0]);
	sys->power_domain_list_size = 2;
	_power_domain_info_set = true;
}


freq_domain_info_t * pal_core_freq_domain(int core)
{
    BUG_ON(!_freq_domain_info_set);
    return &(_freq_domains_info[arch_cluster_pow_sensor(core_to_arch_cluster(core))]);
}

power_domain_info_t * pal_core_power_domain(int core)
{
    BUG_ON(!_freq_domain_info_set);
    return &(_power_domains_info[arch_cluster_pow_sensor(core_to_arch_cluster(core))]);
}

