#ifndef __core_base_power_domain_h
#define __core_base_power_domain_h

#include "_defs.h"

/*
 * This is more like a "sensing domain".
 * Real platforms may not have power sensors for every core,
 * so power domain is used mainly to group cores that
 * share the same power sensor.
 * Notes:
 *    -in core, we used this to calculate total system power and to apply power corrections
 *    -this guy is more relevant on the linux module implementaiton.
 *     On offline simulations, there is always one power domain per core.
 *    -Currently power domains are also part of a frequency domain (to make it easier to
 *     calculate the avg freq. of a power domain) a frequency domain may contain multiple
 *     power domains. This could change if some platform does not support this model.
 */
struct power_domain_info_struct {
    int domain_id;

    //freq domain this power domain belongs to
    struct freq_domain_info_struct* freq_domain;

    //list of cores on this domain
    define_vitamins_list(core_info_t,cores);
    int core_cnt;

    //Link for adding this domain to a freq_domain
    define_list_addable(struct power_domain_info_struct,freq_domain);

    //pointer to dynamic domain data
    struct model_power_domain_struct *this_domain;
};
typedef struct power_domain_info_struct power_domain_info_t;

void set_power_domain(core_info_t *core, power_domain_info_t *domain);


#endif
