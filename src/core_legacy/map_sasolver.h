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

#ifndef __core_sasolver_h
#define __core_sasolver_h

#include "base/base.h"

struct sasolver_solver_conf_struct {
    unsigned int max_iter;
    unsigned int gen_nb_temp;
    unsigned int gen_nb_temp_alpha_scaled;
    unsigned int accept_temp;
    unsigned int accept_temp_alpha_scaled;
    unsigned int diff_scaling_factor_scaled;
    unsigned int task_expl_factor_scaled;
    unsigned int rnd_seed;
};
typedef struct sasolver_solver_conf_struct sasolver_solver_conf_t;


void vitamins_sasolver_map(model_sys_t *sys);
sasolver_solver_conf_t* vitamins_sasolver_get_conf(void);
void vitamins_sasolver_set_conf(sasolver_solver_conf_t *conf);


void vitamins_sasolver_cleanup(void);



#endif
