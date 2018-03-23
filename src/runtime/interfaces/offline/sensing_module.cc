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

#include "sensing_module.h"

#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdexcept>
#include <system_error>

#include <runtime/interfaces/common/sense_data_shared.h>
#include <runtime/interfaces/common/sensing_window_defs.h>
#include <runtime/interfaces/common/user_if_shared.h>

#include <base/base.h>

#include <runtime/common/rt_config_params.h>


struct sensing_window_ctrl_struct {
	int wid;
	int period;//In terms of MINIMUM_WINDOW_LENGHT
	int period_ms;
	int period_jiffies;
	int time_to_ready;//In terms of MINIMUM_WINDOW_LENGHT
	//Link for adding this to the list of sensing windows
	define_list_addable_default(struct sensing_window_ctrl_struct);
};
typedef struct sensing_window_ctrl_struct sensing_window_ctrl_t;

static sensing_window_ctrl_t sensing_windows[MAX_WINDOW_CNT];
int sensing_window_cnt = 0;
//list of sensing windows to keep track of the ones ready to execute
define_vitamins_list(static sensing_window_ctrl_t,next_window);
//list is ordered by time_to_ready
inline static bool sw_order_crit(sensing_window_ctrl_t *a, sensing_window_ctrl_t *b) {
	return (a->time_to_ready == b->time_to_ready) ? (a->period < b->period) : (a->time_to_ready < b->time_to_ready);
}

OfflineSensingModule* OfflineSensingModule::_attached = nullptr;

OfflineSensingModule::OfflineSensingModule()
	:_module_file_if(0), _module_shared_mem_raw_ptr(nullptr),
	 _sensingRunning(false)
{
	if(_attached) arm_throw(SensingModuleException,"There can be only one connection with the sensing module");

    resgisterAsDaemonProc();

    vitsdata = reinterpret_cast<perf_data_t*>(malloc(sizeof(perf_data_t)));
    if(vitsdata == nullptr) arm_throw(SensingModuleException,"malloc error");

    _sensed_data = PerformanceData(vitsdata);

    _attached = this;

    vit_map_perfcnt();
}

OfflineSensingModule::~OfflineSensingModule()
{
    _attached = nullptr;

	if(_sensingRunning)
    	sensingStop();

	free(vitsdata);
//	if(free((sensed_data_t*)vitsdata) < 0)
//    	pinfo("OfflineSensingModule::~OfflineSensingModule: munmap failed with errno=%d!\n",errno);

	if(!unresgisterAsDaemonProc())
		pinfo("OfflineSensingModule::~OfflineSensingModule: IOCTLCMD_UNREGISTER_DAEMON failed with errno=%d!\n",errno);
}

void OfflineSensingModule::forceDetach()
{
	free(vitsdata);
}

void OfflineSensingModule::sensingStart()
{
	pinfo("OfflineSensingModule::sensingStart: Sensing started\n");
	if(_sensingRunning)
		arm_throw(SensingModuleException,"Sensing already runing");

	//TODO: raw counters initially reset in sim
	pinfo("RESETTING ALL COUNTER\n");

	for (int ctr = 0; ctr < MAX_WINDOW_CNT; ctr++) {
		for (int i = 0; i < MAX_CREATED_TASKS; i++) {
			reset_task_counters(0,&(vitsdata->sensing_windows[ctr].curr.tasks[i]));
			reset_task_counters(0,&(vitsdata->sensing_windows[ctr].aggr.tasks[i]));
		}
		for(int i = 0; i < MAX_NR_CPUS; i++) {
			reset_cpu_counters(&(vitsdata->sensing_windows[ctr].curr.cpus[i]));
			reset_cpu_counters(&(vitsdata->sensing_windows[ctr].aggr.cpus[i]));
			reset_freq_counters(&(vitsdata->sensing_windows[ctr].curr.freq_domains[i]));
			reset_freq_counters(&(vitsdata->sensing_windows[ctr].aggr.freq_domains[i]));
			//reset_power_counters(&(vitsdata->sensing_windows[ctr].curr.power_domains[i]));
			//reset_power_counters(&(vitsdata->sensing_windows[ctr].aggr.power_domains[i]));
		}
	}

	_sensingRunning = true;
}

void OfflineSensingModule::sensingStop()
{
	if(_sensingRunning){
//		pinfo("OfflineSensingModule::sensingStop: sensing stopped\n");
		//TODO: any cleanup needed?
	}
	else {
		pinfo("OfflineSensingModule::sensingStop: sensing was not running!\n");
	}
	_sensingRunning = false;
	pinfo("OfflineSensingModule::sensingStop: sensing stopped %d\n",_sensingRunning);
}


int OfflineSensingModule::createSensingWindow(int period_ms)
{
	int i;
	sensing_window_ctrl_t *iter;
	int period = period_ms / MINIMUM_WINDOW_LENGHT_MS;

	if((period <= 0) || ((period_ms % MINIMUM_WINDOW_LENGHT_MS)!=0))
		arm_throw(SensingModuleException,"Sensing period must be multiple of MINIMUM_WINDOW_LENGHT_MS errno=%d",errno);;

	if(sensing_window_cnt >= MAX_WINDOW_CNT)
		arm_throw(SensingModuleException,"Maximum number of sensing windows created errno=%d",errno);

	for(i = 0; i < sensing_window_cnt; ++i)
		if(sensing_windows[i].period == period)
			arm_throw(SensingModuleException,"Window for the specified period already exists errno=%d",errno);

	sensing_windows[sensing_window_cnt].wid = sensing_window_cnt;
	sensing_windows[sensing_window_cnt].period = period;
	sensing_windows[sensing_window_cnt].period_ms = period_ms;
//	sensing_windows[sensing_window_cnt].period_jiffies = (period_ms*CONFIG_HZ)/1000;
	sensing_windows[sensing_window_cnt].time_to_ready = period;
	clear_object_default(&(sensing_windows[sensing_window_cnt]));
	add_to_priority_list_default(next_window,&(sensing_windows[sensing_window_cnt]),sw_order_crit,iter);
	++sensing_window_cnt;

	int wid = sensing_windows[sensing_window_cnt-1].wid;

	//TODO: reset all counters to 0
	pinfo("RESETTING COUNTERZ FOR WIN\n");
		for (int i = 0; i < MAX_CREATED_TASKS; i++) {
			reset_task_counters(0,&(vitsdata->sensing_windows[wid].curr.tasks[i]));
			reset_task_counters(0,&(vitsdata->sensing_windows[wid].aggr.tasks[i]));
		}
		for(int i = 0; i < MAX_NR_CPUS; i++) {
			reset_cpu_counters(&(vitsdata->sensing_windows[wid].curr.cpus[i]));
			reset_cpu_counters(&(vitsdata->sensing_windows[wid].aggr.cpus[i]));
			reset_freq_counters(&(vitsdata->sensing_windows[wid].curr.freq_domains[i]));
			reset_freq_counters(&(vitsdata->sensing_windows[wid].aggr.freq_domains[i]));
			//reset_power_counters(&(vitsdata->sensing_windows[wid].curr.power_domains[i]));
			//reset_power_counters(&(vitsdata->sensing_windows[wid].aggr.power_domains[i]));
		}

	//returned id must be either a positive integer or one of the special window IDs
	if(wid < 0){
		arm_throw(SensingModuleException,"OFFLINE_SENSE_WINDOW_CREATE failed errno=%d",errno);
	}
	else return wid;
}


int OfflineSensingModule::nextSensingWindow()
{
	static bool has_tasks = true;

	if (!_sensingRunning)
		pinfo("OfflineSensingModule::nextSensingWindow: sensing not running!\n");

	if (!has_tasks)  {
		kill (getpid(), SIGINT);
	}
	int i;
	sensing_window_ctrl_t *iter;

	//no windows, no sensing
	if(sensing_window_cnt <= 0) return WINDOW_EXIT;

	//advance time by minimum sensing window length until the next window is ready
	//return ready window
//	assert(has_tasks);
	while(vitamins_list_head(next_window)->time_to_ready != 0){
		has_tasks = _sim->advance_time_standalone((double)MINIMUM_WINDOW_LENGHT_MS/1000.0,simulation_t::VB_BASIC);

		//decrease the time for all windows
		for(i = 0; i < sensing_window_cnt; ++i)
			sensing_windows[i].time_to_ready -= 1;
	}
	sensing_window_ctrl_t *w = vitamins_list_head(next_window);
	//reset period for ending widow and and update list
	w->time_to_ready = w->period;
	remove_from_list_default(next_window, w);
	add_to_priority_list_default(next_window,w,sw_order_crit,iter);

    //TODO:  reset all per-window counters?
	sense_cpus(w->wid);
	sense_tasks(w->wid);
	vitsdata->sensing_windows[w->wid].num_of_samples += 1;

	int wid = w->wid;

	//returned id must be either a positive integer or the special WINDOW_EXIT IDs
	if(wid < 0){
		if(wid == WINDOW_EXIT) return wid;
		else arm_throw(SensingModuleException,"OFFLINE_SENSE_WINDOW_WAIT_ANY failed errno=%d",errno);
	}
	else return wid;
}

bool OfflineSensingModule::isPerfCntAvailable(perfcnt_t cnt)
{
	//printk("vitamins_is_perfcnt_available(%d)=%d %d",perfcnt,perfcnt_to_idx_map[perfcnt],perfcnt_to_idx_map[perfcnt] >= 0);
//	return _sensed_data.perfCntAvailable(cnt);
	return true;
}

void OfflineSensingModule::vit_map_perfcnt() {

	int pcnt = 0;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_INSTR_EXE;
	vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_EXE] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_INSTR_MEM;
	vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_MEM] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_INSTR_BRANCHES;
	vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_BRANCHES] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_BRANCH_MISPRED;
	vitsdata->perfcnt_to_idx_map[PERFCNT_BRANCH_MISPRED] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_INSTR_FP;
	vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_FP] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_BUSY_CY;
	vitsdata->perfcnt_to_idx_map[PERFCNT_BUSY_CY] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_ITLB_ACCESS;
	vitsdata->perfcnt_to_idx_map[PERFCNT_ITLB_ACCESS] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_ITLB_MISSES;
	vitsdata->perfcnt_to_idx_map[PERFCNT_ITLB_MISSES] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_DTLB_ACCESS;
	vitsdata->perfcnt_to_idx_map[PERFCNT_DTLB_ACCESS] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_DTLB_MISSES;
	vitsdata->perfcnt_to_idx_map[PERFCNT_DTLB_MISSES] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_L1ICACHE_HITS;
	vitsdata->perfcnt_to_idx_map[PERFCNT_L1ICACHE_HITS] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_L1ICACHE_MISSES;
	vitsdata->perfcnt_to_idx_map[PERFCNT_L1ICACHE_MISSES] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_L1DCACHE_HITS;
	vitsdata->perfcnt_to_idx_map[PERFCNT_L1DCACHE_HITS] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_L1DCACHE_MISSES;
	vitsdata->perfcnt_to_idx_map[PERFCNT_L1DCACHE_MISSES] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_LLCACHE_HITS;
	vitsdata->perfcnt_to_idx_map[PERFCNT_LLCACHE_HITS] = pcnt;
	++pcnt;
	vitsdata->idx_to_perfcnt_map[pcnt] = PERFCNT_LLCACHE_MISSES;
	vitsdata->perfcnt_to_idx_map[PERFCNT_LLCACHE_MISSES] = pcnt;

	vitsdata->perfcnt_mapped_cnt = 16;

}

void OfflineSensingModule::enablePerTaskSensing()
{
	if(_sensingRunning)
		arm_throw(SensingModuleException,"Cannot do enablePerTaskSensing with sensing running errno=%d",errno);

//	if(ioctl(_module_file_if, IOCTLCMD_ENABLE_PERTASK_SENSING,1) < 0)
//		arm_throw(SensingModuleException,"IOCTLCMD_ENABLE_PERTASK_SENSING failed errno=%d",errno);
}

void OfflineSensingModule::pinAllTasksToCPU(int cpu)
{
	if(_sensingRunning)
		arm_throw(SensingModuleException,"Cannot do pinAllTasksToCPU with sensing running errno=%d",errno);

//	if(ioctl(_module_file_if, IOCTLCMD_ENABLE_PINTASK,cpu) < 0)
//		arm_throw(SensingModuleException,"IOCTLCMD_ENABLE_PINTASK failed errno=%d",errno);
}

void OfflineSensingModule::tracePerfCounter(perfcnt_t perfcnt)
{
	if(_sensingRunning)
		arm_throw(SensingModuleException,"Cannot do tracePerfCounter with sensing running errno=%d",errno);

	/* we hardcode all traced counters for both tasks and cpus:
     *   uint64_t sumInstr;
     *   uint64_t sumMemInstr;
     *   uint64_t sumBRInstr;
     *   uint64_t sumBRMisspred;
     *   uint64_t sumFPInstr;
     *   uint64_t sumCyclesActive[SIZE_COREFREQ];
     *   uint64_t sumiTLBaccesses;
     *   uint64_t sumiTLBmisses;
     *   uint64_t sumdTLBaccesses;
     *   uint64_t sumdTLBmisses;
     *   uint64_t sumICacheHits;
     *   uint64_t sumICacheMisses;
     *   uint64_t sumDCacheHits;
     *   uint64_t sumDCacheMisses;
     *   uint64_t sumL2CacheHits;
     *   uint64_t sumL2CacheMisses;
	 */
//	if(ioctl(_module_file_if, IOCTLCMD_PERFCNT_ENABLE,perfcnt) < 0)
//		arm_throw(SensingModuleException,"IOCTLCMD_PERFCNT_ENABLE failed errno=%d",errno);
}

 void OfflineSensingModule::reset_perf_counters(perf_data_perf_counters_t *sen_data){
	int cnt;
	for(cnt = 0; cnt < MAX_PERFCNTS; ++cnt) sen_data->perfcnts[cnt] = 0;
	sen_data->nivcsw= 0;
	sen_data->nvcsw = 0;
	sen_data->time_busy_ms = 0;
	sen_data->time_total_ms = 0;
}

 void OfflineSensingModule::reset_task_counters(int cpu,perf_data_task_t *sen_data){
	int cnt;
	reset_perf_counters(&(sen_data->perfcnt));
	for(cnt = 0; cnt < MAX_BEAT_DOMAINS; ++cnt) sen_data->beats[cnt] = 0;
	sen_data->last_cpu_used = cpu;
}

 void OfflineSensingModule::reset_cpu_counters(perf_data_cpu_t *sen_data){
	int cnt;
	for(cnt = 0; cnt < MAX_BEAT_DOMAINS; ++cnt) sen_data->beats[cnt] = 0;
	reset_perf_counters(&(sen_data->perfcnt));
}

// void OfflineSensingModule::reset_power_counters(sensed_data_power_domain_t *sen_data){
//	sen_data->avg_power_uW_acc = 0;
//	sen_data->time_ms_acc = 0;
//}

 void OfflineSensingModule::reset_freq_counters(perf_data_freq_domain_t *sen_data){
	sen_data->avg_freq_mhz_acc = 0;
	sen_data->time_ms_acc = 0;
}

void OfflineSensingModule::tracePerfCounterResetAll()
{
	if(_sensingRunning)
		arm_throw(SensingModuleException,"Cannot do tracePerfCounter with sensing running errno=%d",errno);


//	if(ioctl(_module_file_if, IOCTLCMD_PERFCNT_RESET) < 0)
//		arm_throw(SensingModuleException,"IOCTLCMD_PERFCNT_DISABLE failed errno=%d",errno);

}


void OfflineSensingModule::cleanUpCreatedTasks()
{
//	for(int i = 0; i < _sensed_data.numCreatedTasks(); ++i) {
//		if(_sensed_data._raw_data->created_tasks[i].tsk_model != nullptr)
//			delete _sensed_data._raw_data->created_tasks[i].tsk_model;
//	}
//	const_cast<sensed_data_t*>(_sensed_data._raw_data)->created_tasks_cnt = 0;
}


void OfflineSensingModule::resgisterAsDaemonProc()
{
//	if(ioctl(_module_file_if, IOCTLCMD_REGISTER_DAEMON,SECRET_WORD) !=0)
//		arm_throw(SensingModuleException,"IOCTLCMD_REGISTER_DAEMON failed errno=%d",errno);
}

bool OfflineSensingModule::unresgisterAsDaemonProc()
{
//	return ioctl(_module_file_if, IOCTLCMD_UNREGISTER_DAEMON,0) == 0;
	return true;
}


///////////////////////////////////////////////////////////
// SENSING MODULE EMULATION
///////////////////////////////////////////////////////////
void OfflineSensingModule::sense_tasks(int wid)
{
//	sys_info_t *sys = _sim->vitamins_sys()->info;
	int p = 0;

	uint64_t time_total_ms = (_sim->getTime() * 1000.0) - vitsdata->sensing_windows[wid].prev_sample_time_ms;
//	for(p = 0; p < vitsdata->created_tasks_cnt; ++p){
	for(auto task : _sim->task_list_vector()) {

//		private_hook_data_t *task_priv_hook = &(priv_hook_created_tasks[p]);
		perf_data_task_t *last_total = &(vitsdata->sensing_windows[wid].aggr.tasks[task->id-1]);
		perf_data_task_t *curr_epoch = &(vitsdata->sensing_windows[wid].curr.tasks[task->id-1]);
		p++;

//		spin_lock(&(task_priv_hook->sen_data_lock));
		const simulation_t::vitamins_task_sensed_data_raw_t *counters = _sim->task_counters(task);
	    uint64_t active_cy = 0;
	    double active_ms = 0;
	    for(int _f = 0; _f < SIZE_COREFREQ; ++_f){
	        core_freq_t f = (core_freq_t)_f;
	        active_cy += counters->sumCyclesActive[f];
	        active_ms += ((double)counters->sumCyclesActive[f] * (1 / (freqToValMHz_d(f)*1000000)) * 1000.0);
	    }

//	    assert_false("commented code due to error: error: array subscript is above array bounds [-Werror=array-bounds]");
	    int pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_EXE];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumInstr -  last_total->perfcnt.perfcnts[pcnt];
	    pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_MEM];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumMemInstr -  last_total->perfcnt.perfcnts[pcnt];
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_BRANCHES];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumBRInstr -  last_total->perfcnt.perfcnts[pcnt];
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_BRANCH_MISPRED];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumBRMisspred -  last_total->perfcnt.perfcnts[pcnt];
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_FP];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumFPInstr -  last_total->perfcnt.perfcnts[pcnt];
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_BUSY_CY];
		curr_epoch->perfcnt.perfcnts[pcnt] = active_cy - last_total->perfcnt.perfcnts[pcnt];
		pinfo("active %lu - last %lu = %lu\n",active_cy,last_total->perfcnt.perfcnts[pcnt],active_cy - last_total->perfcnt.perfcnts[pcnt]);
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_ITLB_ACCESS];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumiTLBaccesses -  last_total->perfcnt.perfcnts[pcnt];
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_ITLB_MISSES];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumiTLBmisses -  last_total->perfcnt.perfcnts[pcnt];
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_DTLB_ACCESS];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumdTLBaccesses -  last_total->perfcnt.perfcnts[pcnt];
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_DTLB_MISSES];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumdTLBmisses -  last_total->perfcnt.perfcnts[pcnt];
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1ICACHE_HITS];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumICacheHits -  last_total->perfcnt.perfcnts[pcnt];
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1ICACHE_MISSES];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumICacheMisses -  last_total->perfcnt.perfcnts[pcnt];
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1DCACHE_HITS];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumDCacheHits -  last_total->perfcnt.perfcnts[pcnt];
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1DCACHE_MISSES];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumDCacheMisses -  last_total->perfcnt.perfcnts[pcnt];
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_LLCACHE_HITS];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumL2CacheHits -  last_total->perfcnt.perfcnts[pcnt];
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_LLCACHE_MISSES];
		curr_epoch->perfcnt.perfcnts[pcnt] = counters->sumL2CacheMisses -  last_total->perfcnt.perfcnts[pcnt];

		curr_epoch->perfcnt.nvcsw = counters->sumNvcsw - last_total->perfcnt.nvcsw;
		curr_epoch->perfcnt.nivcsw = counters->sumNivcsw -  last_total->perfcnt.nivcsw;
		curr_epoch->perfcnt.time_busy_ms = active_ms - last_total->perfcnt.time_busy_ms;
		curr_epoch->perfcnt.time_total_ms = time_total_ms;

//			for(cnt = 0; cnt < MAX_BEAT_DOMAINS; ++cnt)
//				curr_epoch->beats[cnt] = data_cnt->beats[cnt] -  last_total->beats[cnt];

			//this is not updated at the sensing hooks
//		assert_false("commented code due to error: error: array subscript is above array bounds [-Werror=array-bounds]");
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_EXE];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumInstr;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_MEM];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumMemInstr;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_BRANCHES];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumBRInstr;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_BRANCH_MISPRED];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumBRMisspred;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_FP];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumFPInstr;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_BUSY_CY];
		last_total->perfcnt.perfcnts[pcnt] = active_cy;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_ITLB_ACCESS];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumiTLBaccesses;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_ITLB_MISSES];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumiTLBmisses;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_DTLB_ACCESS];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumdTLBaccesses;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_DTLB_MISSES];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumdTLBmisses;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1ICACHE_HITS];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumICacheHits;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1ICACHE_MISSES];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumICacheMisses;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1DCACHE_HITS];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumDCacheHits;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1DCACHE_MISSES];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumDCacheMisses;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_LLCACHE_HITS];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumL2CacheHits;
		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_LLCACHE_MISSES];
		last_total->perfcnt.perfcnts[pcnt] = counters->sumL2CacheMisses;


		last_total->perfcnt.nvcsw += counters->sumNvcsw;
		last_total->perfcnt.nivcsw += counters->sumNivcsw;
		last_total->perfcnt.time_busy_ms += curr_epoch->perfcnt.time_busy_ms;
		last_total->perfcnt.time_total_ms += curr_epoch->perfcnt.time_total_ms;

//		spin_unlock(&(task_priv_hook->sen_data_lock));
	}
	vitsdata->created_tasks_cnt = p;
}


//static int tasks_being_created = 0;
//void OfflineSensingModule::new_task_created(int at_cpu, struct task_struct *tsk,struct task_struct *parent_tsk){
//    //allocates the hook data
//    tracked_task_data_t *hooks;
//    private_hook_data_t *priv_hooks;
//    private_hook_data_t *parent;
//    sys_info_t *sys;
//    int i;
//    int task_idx;
//
//    if((unsigned)vitsdata->created_tasks_cnt >= MAX_CREATED_TASKS){
//    	pinfo("Task buffer full ! New task ignored\n");
//    	return;
//    }
//
//    task_idx = vitsdata->__created_tasks_cnt_tmp;
//    vitsdata->__created_tasks_cnt_tmp += 1;
//    tasks_being_created += 1;
//
//    hooks = &(vitsdata->created_tasks[task_idx]);
//    priv_hooks = &(priv_hook_created_tasks[task_idx]);
//    sys = system_info();
//
//    clear_object(priv_hooks,hashmap);
//
//    priv_hooks->hook_data = hooks;
//    priv_hooks->this_task = tsk;
//    priv_hooks->beats = nullptr;
//    hooks->this_task_pid = tsk->pid;
//    hooks->task_idx = task_idx;
//    BUG_ON(TASK_NAME_SIZE > TASK_COMM_LEN);
//    memcpy(hooks->this_task_name,tsk->comm,TASK_NAME_SIZE);
//
//    //if we want to pin this task
//    if(at_cpu >= 0){
//    	BUG_ON(at_cpu >= sys->core_list_size);
//    	cpus_clear(tsk->cpus_allowed);
//    	cpu_set(at_cpu, tsk->cpus_allowed);
//    }
//
//    priv_hooks->sen_data_lock = __SPIN_LOCK_UNLOCKED(priv_hooks->sen_data_lock);
//
//    for(i=0;i<sensing_window_cnt;++i){
//    	reset_task_counters(at_cpu,&(vitsdata->sensing_windows[i]._acc.tasks[task_idx]));
//    	reset_task_counters(at_cpu,&(vitsdata->sensing_windows[i].curr.tasks[task_idx]));
//    	reset_task_counters(at_cpu,&(vitsdata->sensing_windows[i].aggr.tasks[task_idx]));
//    }
//    hooks->num_beat_domains = 0;
//    //check if parent has beat domain
//    parent = hook_hashmap_get(parent_tsk);
//    if (parent && (parent->hook_data != nullptr))
//    	hooks->parent_has_beats = (parent->hook_data->num_beat_domains > 0) || parent->hook_data->parent_has_beats;
//    else
//    	hooks->parent_has_beats = false;
//
//    hooks->task_finished = false;
//
//    hooks->tsk_model = nullptr;//initialized on demand at user level
//
//    hook_hashmap_add(tsk,priv_hooks);
//    BUG_ON(hook_hashmap_get(tsk) != priv_hooks);
//
//
//    spin_lock(&new_task_created_lock);
//		tasks_being_created -= 1;
//    	if(tasks_being_created==0)
//    		vitsdata->created_tasks_cnt = vitsdata->__created_tasks_cnt_tmp;
//    spin_unlock(&new_task_created_lock);
//}


void OfflineSensingModule::sense_cpus(int wid)
{
	sys_info_t *sys = _sim->vitamins_sys()->info;
	int i;
	uint64_t time_total_ms,curr_time_ms;

	curr_time_ms = _sim->getTime() * 1000.0;
    time_total_ms = curr_time_ms - vitsdata->sensing_windows[wid].curr_sample_time_ms;

    vitsdata->sensing_windows[wid].prev_sample_time_ms = vitsdata->sensing_windows[wid].curr_sample_time_ms;
    vitsdata->sensing_windows[wid].curr_sample_time_ms = curr_time_ms;

    for(auto core : _sim->core_list_vector()) {
    	i = core.info->position;
	    perf_data_cpu_t *last_total = &(vitsdata->sensing_windows[wid].aggr.cpus[i]);
	    perf_data_cpu_t *curr_epoch = &(vitsdata->sensing_windows[wid].curr.cpus[i]);

	    reset_cpu_counters(curr_epoch);

	    for(auto task : _sim->task_list_vector()){
	    	if(task_curr_core_idx(task) == (int)i){
	    		const simulation_t::vitamins_task_sensed_data_raw_t *counters = _sim->task_counters(task);
	    		perf_data_task_t *last_total_task = &(vitsdata->sensing_windows[wid].aggr.tasks[task->id-1]);

	    	    double active_cy = 0;
	    	    uint64_t active_ms = 0;
	    	    for(int _f = 0; _f < SIZE_COREFREQ; ++_f){
	    	        core_freq_t f = (core_freq_t)_f;
	    	        active_cy += counters->sumCyclesActive[f];
	    	        active_ms += ((double)counters->sumCyclesActive[f] * (1 / (freqToValMHz_d(f)*1000000)) * 1000.0);
	    	    }

//	    	    assert_false("commented code due to error: error: array subscript is above array bounds [-Werror=array-bounds]");
	    		int pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_EXE];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumInstr -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_MEM];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumMemInstr -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_BRANCHES];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumBRInstr -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_BRANCH_MISPRED];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumBRMisspred -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_FP];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumFPInstr -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_BUSY_CY];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += active_cy -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_ITLB_ACCESS];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumiTLBaccesses -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_ITLB_MISSES];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumiTLBmisses -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_DTLB_ACCESS];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumdTLBaccesses -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_DTLB_MISSES];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumdTLBmisses -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1ICACHE_HITS];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumICacheHits -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1ICACHE_MISSES];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumICacheMisses -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1DCACHE_HITS];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumDCacheHits -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1DCACHE_MISSES];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumDCacheMisses -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_LLCACHE_HITS];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumL2CacheHits -  last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_LLCACHE_MISSES];
	    		curr_epoch->perfcnt.perfcnts[pcnt] += counters->sumL2CacheMisses -  last_total_task->perfcnt.perfcnts[pcnt];


	    		curr_epoch->perfcnt.nvcsw += counters->sumNvcsw - last_total_task->perfcnt.nvcsw;
	    		curr_epoch->perfcnt.nivcsw += counters->sumNivcsw -  last_total_task->perfcnt.nivcsw;
	    		curr_epoch->perfcnt.time_busy_ms += active_ms - last_total_task->perfcnt.time_busy_ms;

	    		//		for(cnt = 0; cnt < MAX_BEAT_DOMAINS; ++cnt)
	    		//			curr_epoch->beats[cnt] = data_cnt->beats[cnt] -  last_total->beats[cnt];

	    		//this is not updated at the sensing hooks
//	    		assert_false("commented code due to error: error: array subscript is above array bounds [-Werror=array-bounds]");
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_EXE];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumInstr - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_MEM];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumMemInstr - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_BRANCHES];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumBRInstr - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_BRANCH_MISPRED];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumBRMisspred - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_INSTR_FP];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumFPInstr - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_BUSY_CY];
	    		last_total->perfcnt.perfcnts[pcnt] += active_cy - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_ITLB_ACCESS];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumiTLBaccesses - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_ITLB_MISSES];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumiTLBmisses - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_DTLB_ACCESS];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumdTLBaccesses - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_DTLB_MISSES];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumdTLBmisses - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1ICACHE_HITS];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumICacheHits - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1ICACHE_MISSES];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumICacheMisses - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1DCACHE_HITS];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumDCacheHits - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_L1DCACHE_MISSES];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumDCacheMisses - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_LLCACHE_HITS];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumL2CacheHits - last_total_task->perfcnt.perfcnts[pcnt];
	    		pcnt = vitsdata->perfcnt_to_idx_map[PERFCNT_LLCACHE_MISSES];
	    		last_total->perfcnt.perfcnts[pcnt] += counters->sumL2CacheMisses - last_total_task->perfcnt.perfcnts[pcnt];


	    		last_total->perfcnt.nvcsw += counters->sumNvcsw;
	    		last_total->perfcnt.nivcsw += counters->sumNivcsw;
	    		last_total->perfcnt.time_busy_ms += active_ms - last_total_task->perfcnt.time_busy_ms;

	    	}
	    }

		curr_epoch->perfcnt.time_total_ms = time_total_ms;
		last_total->perfcnt.time_total_ms += curr_epoch->perfcnt.time_total_ms;

	}

    //freq sense
    for(i = 0; i < sys->freq_domain_list_size; ++i){
		perf_data_freq_domain_t *last_total = &(vitsdata->sensing_windows[wid].aggr.freq_domains[i]);
		perf_data_freq_domain_t *curr_epoch = &(vitsdata->sensing_windows[wid].curr.freq_domains[i]);

	    double active_time = 0;
	    uint64_t active_cy = 0;
	    double avg_freq = 0;

    	for(auto task : _sim->task_list_vector()){
    		if (_sim->core_list_vector()[task_curr_core_idx(task)].info->freq->domain_id == (int)i) {
    			const simulation_t::vitamins_task_sensed_data_raw_t *counters = _sim->task_counters(task);

	    	    for(int _f = 0; _f < SIZE_COREFREQ; ++_f){
	    	        core_freq_t f = (core_freq_t)_f;
	    	        active_time += counters->sumCyclesActive[f] * (1 / (freqToValMHz_d(f)*1000000));
	    	        active_cy += counters->sumCyclesActive[f];
	    	        avg_freq += freqToValMHz_d(f) * counters->sumCyclesActive[f];
	    	    }

    		}
    	}

//    	avg_freq /= active_cy;
    	curr_epoch->avg_freq_mhz_acc = avg_freq / active_cy;
		curr_epoch->time_ms_acc = (active_time * 1000) - last_total->time_ms_acc;

		last_total->avg_freq_mhz_acc += avg_freq / active_cy;
		last_total->time_ms_acc += (active_time * 1000) - last_total->time_ms_acc;
    }

	//power sense
    /*for(i = 0; i < sys->power_domain_list_size; ++i){
		sensed_data_power_domain_t *last_total = &(vitsdata->sensing_windows[wid].aggr.power_domains[i]);
		sensed_data_power_domain_t *curr_epoch = &(vitsdata->sensing_windows[wid].curr.power_domains[i]);

	    double active_time = 0;
	    uint64_t active_cy = 0;
	    uint64_t power_acc = 0;

    	for(auto task : _sim->task_list_vector()){
    		if (_sim->core_list_vector()[task_curr_core_idx(task)].info->power->domain_id == (int)i) {
    			const simulation_t::vitamins_task_sensed_data_raw_t *counters = _sim->task_counters(task);

	    		for(int _f = 0; _f < SIZE_COREFREQ; ++_f){
	    	        core_freq_t f = (core_freq_t)_f;
	    	        power_acc += counters->sumPowerTimesCycles[f];
	    	        active_cy += counters->sumCyclesActive[f];
	    	        active_time += counters->sumCyclesActive[f] * (1 / (freqToValMHz_d(f)*1000000));
	    		}

    		}
    	}

		curr_epoch->avg_power_uW_acc = (power_acc * 1000000.0 / active_cy);
		curr_epoch->time_ms_acc = (active_time * 1000) - last_total->time_ms_acc;

		last_total->avg_power_uW_acc += (power_acc * 1000000.0 / active_cy);
		last_total->time_ms_acc += (active_time * 1000) - last_total->time_ms_acc;

		pinfo("this pow %lu total pow %lu (%lu/%lu)\n",curr_epoch->avg_power_uW_acc,last_total->avg_power_uW_acc,curr_epoch->time_ms_acc,last_total->time_ms_acc);
    }*/

}

void OfflineSensingModule::setSim(simulation_t *sim) {
	_sim = sim;
}

/////////////////////////////
// perf conter maping funcs

//static void vit_reset_mapped_perfcnt(void)
//{
//	int i;
//	plat_reset_perfcnts();
//	for(i = 0; i < MAX_PERFCNTS;++i) vitsdata->idx_to_perfcnt_map[i] = -1;
//	for(i = 0; i < SIZE_PERFCNT;++i) vitsdata->perfcnt_to_idx_map[i] = -1;
//	vitsdata->perfcnt_mapped_cnt = 0;
//}
//static bool vit_map_perfcnt(perfcnt_t perfcnt)
//{
//	BUG_ON(vitsdata->perfcnt_mapped_cnt > MAX_PERFCNTS);
//	if(vitsdata->perfcnt_mapped_cnt >= MAX_PERFCNTS){
//		pinfo("Cannot use more then %d pmmu counters!",MAX_PERFCNTS);
//		return false;
//	}
//	plat_enable_perfcnt(perfcnt);
//	vitsdata->idx_to_perfcnt_map[vitsdata->perfcnt_mapped_cnt] = perfcnt;
//	vitsdata->perfcnt_to_idx_map[perfcnt] = vitsdata->perfcnt_mapped_cnt;
//	vitsdata->perfcnt_mapped_cnt += 1;
//	return true;
//}


template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
SensingInterface::Impl::sense<SEN_POWER_W,power_domain_info_t>(const power_domain_info_t *rsc, int wid)
{
    return 0;
}

template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
SensingInterface::Impl::senseAgg<SEN_POWER_W,power_domain_info_t>(const power_domain_info_t *rsc, int wid)
{
    return 0;
}

