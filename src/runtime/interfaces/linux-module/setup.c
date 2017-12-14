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

