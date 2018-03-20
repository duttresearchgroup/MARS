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
