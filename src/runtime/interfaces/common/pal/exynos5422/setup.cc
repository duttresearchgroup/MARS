#include "_common.h"
#include <core/core.h>

model_freq_domain_t _freq_domains[2];//0=big 1=little
bool _freq_domain_set = false;
model_power_domain_t _power_domains[2];//0=big 1=little
bool _power_domain_set = false;

void model_setup_freq_domains(sys_info_t *sys)
{
	int i;
	BUG_ON(_freq_domain_set);
	BUG_ON(!_freq_domain_info_set);
	for(i = 0; i < 2; ++i){
		vitamins_freq_domain_init(&(_freq_domains[i]),&(_freq_domains_info[i]),COREFREQ_0000MHz);
	}
	_freq_domain_set = true;
}

void model_setup_power_domains(sys_info_t *sys)
{
	int i;
	BUG_ON(_power_domain_set);
	BUG_ON(!_power_domain_info_set);
	for(i = 0; i < 2; ++i){
        vitamins_power_domain_init(&(_power_domains[i]),&(_power_domains_info[i]));
	}
	_power_domain_set = true;
}


void model_setup_overheads(model_sys_t *sys)
{
	/*
	 * Baseline sys load based on experimental observations:
	 *   BIG cores: 0.1 % (just enough to keep the cluster ON)
	 *   Little cores (at lowest freq):
	 *   	core 0: 4%
	 *   	core 1: 2%
	 *   	core 2: 1%
	 *   	core 3: 1%
	 *
	 * TODO this sould be used as initial estimation and updated during the sensing phase
	 */
	uint32_t big_load = CONV_DOUBLE_scaledUINT32(0.001);
	uint32_t little_load_baseline[4] = { CONV_DOUBLE_scaledUINT32(0.04),
										 CONV_DOUBLE_scaledUINT32(0.02),
										 CONV_DOUBLE_scaledUINT32(0.01),
										 CONV_DOUBLE_scaledUINT32(0.01) };
	for(int core = 0; core < sys->info->core_list_size; ++core){
		if(core_to_arch_cluster(core)==COREARCH_Exynos5422_BIG){
			for_enum(core_freq_t,freq,0,SIZE_COREFREQ,+1){
				if(vitamins_arch_freq_available(COREARCH_Exynos5422_BIG,freq))
					sys->core_systask_list[core].tlc[COREARCH_Exynos5422_BIG][freq] = big_load;
			}
		}
		else{
			BUG_ON(core_to_arch_cluster(core)!=COREARCH_Exynos5422_LITTLE);
			for_enum(core_freq_t,freq,0,SIZE_COREFREQ,+1){
				if(vitamins_arch_freq_available(COREARCH_Exynos5422_LITTLE,freq)){
					uint32_t scale = CONV_INTany_scaledINTany(freqToValMHz_i(freq))/freqToValMHz_i(vitamins_dvfs_get_minimum_freq(sys->info->core_list[core].this_core));
					uint32_t load = little_load_baseline[core];

					BUG_ON(scale < CONV_INTany_scaledINTany(1));
					BUG_ON(scale > CONV_INTany_scaledINTany(2));

					sys->core_systask_list[core].tlc[COREARCH_Exynos5422_LITTLE][freq] = CONV_INTany_scaledINTany(load)/scale;
					BUG_ON(sys->core_systask_list[core].tlc[COREARCH_Exynos5422_LITTLE][freq] < CONV_DOUBLE_scaledUINT32(0.001));
					BUG_ON(sys->core_systask_list[core].tlc[COREARCH_Exynos5422_LITTLE][freq] > load);
				}
			}
		}
	}
}

