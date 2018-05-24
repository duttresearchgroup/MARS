/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * Copyright (C) 2018 Bryan Donyanavard <bdonyana@uci.edu>
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

#include "defs.h"

/* TODO: this needs to be implemented accurately
 * for now, just assume we have a single domain
 * and allow any frequency
 */
static freq_domain_info_t _freq_domains_info[1];//0=big 1=little
static power_domain_info_t _power_domains_info[1];//0=big 1=little
static core_info_t _core_info_list[MAX_NR_CPUS];
static sys_info_t _sys_info;
static bool _initialized = false;

// this shouldn't matter
core_arch_t _pal_core_arch(int core)
{
    return COREARCH_GEM5_GENERIC_ARM;
}

static void _pal_setup_freq_domains_info(sys_info_t *sys)
{
	int i;
	for(i = 0; i < 1; ++i){
		freq_domain_info_init(&(_freq_domains_info[i]),i,nullptr,nullptr);
	}
	sys->freq_domain_list = &(_freq_domains_info[0]);
	sys->freq_domain_list_size = 1;
}

static void _pal_setup_power_domains_info(sys_info_t *sys)
{
	int i;
	for(i = 0; i < 1; ++i){
        power_domain_info_init(&(_power_domains_info[i]),i,&(_freq_domains_info[i]));
	}
	sys->power_domain_list = &(_power_domains_info[0]);
	sys->power_domain_list_size = 1;
}


freq_domain_info_t * _pal_core_freq_domain(int core)
{
    return &(_freq_domains_info[0]);
}

power_domain_info_t * _pal_core_power_domain(int core)
{
    return &(_power_domains_info[0]);
}

sys_info_t* pal_sys_info(int online_cpus)
{
    int core;
    if(!_initialized){
        _sys_info.core_list = &(_core_info_list[0]);
        _sys_info.core_list_size = online_cpus;

        _pal_setup_freq_domains_info(&_sys_info);
        _pal_setup_power_domains_info(&_sys_info);

        for(core = 0; core < online_cpus; ++core){
            core_info_init(&(_core_info_list[core]), _pal_core_arch(core), core, _pal_core_freq_domain(core), _pal_core_power_domain(core));
        }
        _initialized = true;
    }
    return &_sys_info;
}
