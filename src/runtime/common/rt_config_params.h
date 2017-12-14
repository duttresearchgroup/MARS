#ifndef __arm_rt_rt_config_params_h
#define __arm_rt_rt_config_params_h

#include <string>

#include <runtime/interfaces/common/perfcnts.h>

/*
 * Runtime configurations and external functions definitions
 *
 * Implemented at for both kernel module and daemon
 */

//not implemented in for the module (user the module_param macros to retrieve arguments)
bool init_rt_config_params(int argc, const char * argv[]);

const std::string& rt_param_mode(void);
int rt_param_trace_core(void);

int rt_param_overheadtest_core0(void);
int rt_param_overheadtest_core1(void);

bool rt_param_trace_perfcnt(perfcnt_t perfcnt);

const std::string& rt_param_model_predictor_file(void);
const std::string& rt_param_model_idlepower_file(void);

const std::string& rt_param_outdir(void);

void rt_param_print(void);

const std::string& rt_param_daemon_file();

const std::string& rt_param_sisotest_ctrlname();
double rt_param_sisotest_ref0();
double rt_param_sisotest_ref1();
double rt_param_sisotest_ref2();

const std::string& rt_param_sys_ctrl_gains();
double* rt_param_sys_ctrl_refs_perf();
double* rt_param_sys_ctrl_refs_power();
int* rt_param_sys_ctrl_refs_enable_timeS();
int rt_param_sys_ctrl_total_refs_count();
int rt_param_sys_ctrl_sub_refs_count();


#endif

