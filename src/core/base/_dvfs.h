#ifndef __core_base_dvfs_h
#define __core_base_dvfs_h

#include "_defs.h"

//called to set the physical frequency for the frequency domain
typedef bool (set_freq_callback)(struct model_freq_domain_struct *fd, core_freq_t freq);
typedef core_freq_t (get_freq_callback)(struct model_freq_domain_struct *fd);

struct freq_domain_info_struct {
    int domain_id;

    //kernel callbacks to physicaly set the frequency
    set_freq_callback *set_freq_callback;
    get_freq_callback *get_freq_callback;

    //list of cores on this domain
    define_vitamins_list(core_info_t,cores);
    int core_cnt;

    //list of power domains on this domain
    define_vitamins_list(struct power_domain_info_struct,power_domains);
    int power_domain_cnt;

    //this domain dynamic data
    struct model_freq_domain_struct *this_domain;
};
typedef struct freq_domain_info_struct freq_domain_info_t;

void set_core_freq_domain(core_info_t *core, freq_domain_info_t *domain);
void set_pow_freq_domain(struct power_domain_info_struct *power_domain, freq_domain_info_t *domain);


#endif
