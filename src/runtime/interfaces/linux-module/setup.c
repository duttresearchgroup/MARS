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

#include "../linux-module/setup.h"



#include "../linux-module/core.h"
#include "../linux-module/helpers.h"
#include "../linux-module/pal/pal.h"

//store task and core information
static core_info_t core_info_list[NR_CPUS];
static sys_info_t _vitamins_sys_info;

sys_info_t* system_info() {
	return &_vitamins_sys_info;
}


void init_system_info()
{
	int core;

	sys_info_t* sys_info = system_info();

	BUG_ON(num_online_cpus() > NR_CPUS);
	BUG_ON(MAX_NUM_TASKS <= num_online_cpus());

	sys_info->core_list = &(core_info_list[0]);
	sys_info->core_list_size = num_online_cpus();

	pal_setup_freq_domains_info(sys_info);
	pal_setup_power_domains_info(sys_info);

	for_each_online_cpu(core){
		core_info_init(&(core_info_list[core]), pal_core_arch(core), core, pal_core_freq_domain(core), pal_core_power_domain(core));
	}
}

