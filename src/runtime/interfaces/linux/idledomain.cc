#include "idledomain.h"

std::map<pid_t,cpu_set_t> IdleDomain::_task_map_orig_mask;
