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
