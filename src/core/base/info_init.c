
#include "_info_init.h"
#include "_exceptions.h"

void freq_domain_info_init(freq_domain_info_t *freq_domain, int freq_domain_id, set_freq_callback set_phy_f, get_freq_callback get_phy_f)
{
    clear_internal_list(freq_domain,cores);
    freq_domain->core_cnt = 0;
    clear_internal_list(freq_domain,power_domains);
    freq_domain->power_domain_cnt = 0;

    freq_domain->set_freq_callback = set_phy_f;
    freq_domain->get_freq_callback = get_phy_f;
    freq_domain->domain_id = freq_domain_id;

    freq_domain->this_domain = nullptr;
}

void power_domain_info_init(power_domain_info_t *power_domain, int power_domain_id, freq_domain_info_t *freq_domain)
{
    clear_internal_list(power_domain,cores);
    power_domain->domain_id = power_domain_id;

    clear_object(power_domain,freq_domain);
    set_pow_freq_domain(power_domain,freq_domain);

    power_domain->this_domain = nullptr;
}

void set_core_freq_domain(core_info_t *core, freq_domain_info_t *domain)
{
    add_to_internal_list(domain,cores,core,freq_domain);
    core->freq = domain;
    domain->core_cnt += 1;
}

void set_pow_freq_domain(struct power_domain_info_struct *power_domain, freq_domain_info_t *domain)
{
    add_to_internal_list(domain,power_domains,power_domain,freq_domain);
    power_domain->freq_domain = domain;
    domain->power_domain_cnt += 1;
}


void core_info_init(core_info_t *core, core_arch_t arch, int core_id, freq_domain_info_t *freq_domain, power_domain_info_t *power_domain)
{
	core->arch = arch;
	core->position = core_id;

	clear_object(core,freq_domain);
	clear_object(core,power_domain);

	set_core_freq_domain(core,freq_domain);
	set_power_domain(core,power_domain);
}

void set_power_domain(core_info_t *core, power_domain_info_t *domain)
{
    add_to_internal_list(domain,cores,core,power_domain);
    core->power = domain;
    domain->core_cnt += 1;
}

#define Fletcher32_acc(sum1, sum2, data)\
    sum1 = ((uint32_t)(sum1) + (uint32_t)(data)) % 0xFFFF;\
    sum2 = ((uint32_t)(sum2) + (uint32_t)(sum1)) % 0xFFFF;

#define Fletcher32_comp(sum1,sum2) (((uint32_t)(sum2) << 16) | (uint32_t)(sum1))

uint32_t sys_info_cksum(sys_info_t *info){
	uint32_t sum1=0,sum2=0;
	int i;

	Fletcher32_acc(sum1,sum2,info->core_list_size);
	Fletcher32_acc(sum1,sum2,info->freq_domain_list_size);
	Fletcher32_acc(sum1,sum2,info->power_domain_list_size);

	BUG_ON(info->freq_domain_list == nullptr);
	for(i=0;i<info->freq_domain_list_size;++i){
		Fletcher32_acc(sum1,sum2,info->freq_domain_list[i].domain_id);
		Fletcher32_acc(sum1,sum2,info->freq_domain_list[i].core_cnt);
		Fletcher32_acc(sum1,sum2,info->freq_domain_list[i].power_domain_cnt);
	}

	BUG_ON(info->power_domain_list == nullptr);
	for(i=0;i<info->power_domain_list_size;++i){
		Fletcher32_acc(sum1,sum2,info->power_domain_list[i].domain_id);
		Fletcher32_acc(sum1,sum2,info->power_domain_list[i].core_cnt);
		BUG_ON(info->power_domain_list[i].freq_domain == nullptr);
		Fletcher32_acc(sum1,sum2,info->power_domain_list[i].freq_domain->domain_id);
	}

	BUG_ON(info->core_list == nullptr);
	for(i=0;i<info->core_list_size;++i) {
		Fletcher32_acc(sum1,sum2,info->core_list[i].position);
		Fletcher32_acc(sum1,sum2,info->core_list[i].arch);
		BUG_ON(info->core_list[i].freq == nullptr);
		BUG_ON(info->core_list[i].power == nullptr);
		Fletcher32_acc(sum1,sum2,info->core_list[i].freq->domain_id);
		Fletcher32_acc(sum1,sum2,info->core_list[i].power->domain_id);
	}

	Fletcher32_acc(sum1,sum2,1);

	return Fletcher32_comp(sum1,sum2);
}

