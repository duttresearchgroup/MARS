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
