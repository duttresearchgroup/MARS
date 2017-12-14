#ifndef __core_power_domain_h
#define __core_power_domain_h

#include "base/base.h"

//static domain info
struct model_power_domain_struct {
	power_domain_info_t *info;

    power_domain_sensed_data_t sensed_data;

    pred_checker_power_domain_t pred_checker;
};
typedef struct model_power_domain_struct model_power_domain_t;

#endif
