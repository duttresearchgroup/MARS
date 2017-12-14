
#include "core.h"

#define pred_checker_err_init(pred_checker)\
	do {\
    (pred_checker).pred_error_acc_s = 0;\
    (pred_checker).pred_error_acc_u = 0;\
    (pred_checker).pred_error_cnt = 0;\
    } while(0)


void vitamins_task_init(model_task_t *task, int id)
{
	//int arch;
	task->id = id;
    task->sensed_data.proc_time_share = 0;
    task->sensed_data.proc_time_share_avg = 0;

    task->_curr_mapping_ = nullptr;
    task->_next_mapping_ = nullptr;
    task->_next_mapping_load = INVALID_METRIC_VAL;
    clear_object(task,mapping);

    task->pred_checker.prev_pred_valid = false;
    pred_checker_err_init(task->pred_checker.error_ips_active);
    //for(arch = 0; arch < SIZE_COREARCH; ++arch) task->pred_checker.correction_ips_active[arch] = 0;
    task->pred_checker.correction_ips_active = 0;
    pred_checker_err_init(task->pred_checker.error_load);
}

void vitamins_sys_task_init(model_systask_t *task, model_core_t *core)
{
	int arch,freq;
    task->core = core;
    core->systask = task;

    //sys task perf/power info is initially constant
    for(arch = 0; arch < SIZE_COREARCH; ++arch){
    	for(freq = 0; freq < SIZE_COREFREQ; ++freq){
    		if(((core_arch_t)arch == core->info->arch) && vitamins_arch_freq_available((core_arch_t)arch,(core_freq_t)freq)){
    			//assume ipc is always 0.25, power is idle, and tlc is 0
    			//tweak only the TLC during platform-specific setup
    			task->ips_active[arch][freq] = freqToValMHz_i((core_freq_t)freq)/4;
    			task->power_active[arch][freq] = arch_idle_power_scaled((core_arch_t)arch,(core_freq_t)freq);
    			task->tlc[arch][freq] = 0;
    		}
    		else{
    			//should never use these guys, so we set to easily detectable trash
    			task->ips_active[arch][freq] = INVALID_METRIC_VAL;
    			task->power_active[arch][freq] = INVALID_METRIC_VAL;
    			task->tlc[arch][freq] = INVALID_METRIC_VAL;
    		}
    	}
    }
}

void init_ctrl(Controller &_ctrl, double kp,double ki,double mu,double my,double ref)
{
	_ctrl = Controller(kp,ki,0);
	_ctrl.offsets(0,mu);
	_ctrl.referenceOutput(ref);
	_ctrl.errorTolerance(0.02);
	_ctrl.state(mu,my);
}

void vitamins_freq_domain_init(model_freq_domain_t *freq_domain, freq_domain_info_t *freq_domain_info, core_freq_t initial_freq)
{
	freq_domain->info = freq_domain_info;
	freq_domain_info->this_domain = freq_domain;

	freq_domain->freq = initial_freq;
    freq_domain->manual_freq = freq_domain->freq;

    freq_domain->pred_checker.prev_pred_valid = false;
    pred_checker_err_init(freq_domain->pred_checker.error_freq);

    init_ctrl(freq_domain->ctrl_freq, 2.96, 3.11, 2.396, 1.6732, 0.8);
    init_ctrl(freq_domain->ctrl_cache, 2.96, 3.11, 2.396, 1.6732, 1.2);
}

void vitamins_power_domain_init(model_power_domain_t *power_domain, power_domain_info_t *power_domain_info)
{
	power_domain->info = power_domain_info;
	power_domain_info->this_domain = power_domain;

    power_domain->pred_checker.prev_pred_valid = false;
    pred_checker_err_init(power_domain->pred_checker.error_power);
}

void vitamins_core_init(model_core_t *core, core_info_t *core_info)
{
	core->info = core_info;
	core_info->this_core = core;

	clear_object_default(core);

	core->load_tracking.common.core = core;
	core->load_tracking.common.load = 0;
	core->load_tracking.common.task_cnt = 0;
	clear_internal_list(core,mapped_tasks);

	core->aging_info.current_delay_ps = freqToPeriodPS_i((core_freq_t)0)-1;
	core->aging_info.initial_delay_ps = freqToPeriodPS_i((core_freq_t)0)-1;
	core->aging_info.max_delay_ps = freqToPeriodPS_i((core_freq_t)0);
	core->aging_info.rel_delay_degrad = 0;
	core->aging_info.rel_delay_degrad_penalty= CONV_INTany_scaledINTany(1);

	core->pred_checker.prev_pred_valid = false;
    pred_checker_err_init(core->pred_checker.error_load);
}

