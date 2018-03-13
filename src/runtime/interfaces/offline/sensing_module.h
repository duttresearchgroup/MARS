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

#ifndef OFFLINE_SENSING_MODULE_H_
#define OFFLINE_SENSING_MODULE_H_

#include <offline_sim/exec_sim.h>
#include "../performance_data.h"

class OfflineSensingModule
{
  private:

	static OfflineSensingModule* _attached;

	int _module_file_if;
	void* _module_shared_mem_raw_ptr;
	volatile bool _sensingRunning;
	PerformanceData _sensed_data;
	perf_data_t *vitsdata;

	simulation_t *_sim;

//	struct sensing_window_ctrl_struct {
//		int wid;
//		int period;//In terms of MINIMUM_WINDOW_LENGHT
//		int period_ms;
//		int period_jiffies;
//		int time_to_ready;//In terms of MINIMUM_WINDOW_LENGHT
//		//Link for adding this to the list of sensing windows
//		define_list_addable_default(struct sensing_window_ctrl_struct);
//	};
//	typedef struct sensing_window_ctrl_struct sensing_window_ctrl_t;
//
//	static sensing_window_ctrl_t sensing_windows[MAX_WINDOW_CNT];
//	int sensing_window_cnt = 0;
//	//list of sensing windows to keep track of the ones ready to execute
//	define_vitamins_list(static sensing_window_ctrl_t,next_window);
//	//list is ordered by time_to_ready
//	inline static bool sw_order_crit(sensing_window_ctrl_t *a, sensing_window_ctrl_t *b) {
//	    return (a->time_to_ready == b->time_to_ready) ? (a->period < b->period) : (a->time_to_ready < b->time_to_ready);
//	}

	void sense_tasks(int wid);

	void sense_cpus(int wid);

	void reset_perf_counters(perf_data_perf_counters_t *sen_data);
	void reset_task_counters(int cpu,perf_data_task_t *sen_data);
	void reset_cpu_counters(perf_data_cpu_t *sen_data);
	//void reset_power_counters(sensed_data_power_domain_t *sen_data);
	void reset_freq_counters(perf_data_freq_domain_t *sen_data);

	void vit_map_perfcnt();

  public:
	OfflineSensingModule();

	~OfflineSensingModule();

	//disconnects this object from the module without checks
	//will make this object invalid
	void forceDetach();

    static OfflineSensingModule& get()
    {
        if(_attached == nullptr)
            arm_throw(OfflineSensingModuleException,"Sensing module not attached");
        return *_attached;
    }

  public:

	void sensingStart();
	void sensingStop();

	bool isSensing() { return _sensingRunning; }

	int createSensingWindow(int period_ms);

	int nextSensingWindow();

	void resgisterAsDaemonProc();//registers calling process as a daemon process
	bool unresgisterAsDaemonProc();

	const PerformanceData& data() { return _sensed_data; }
	const perf_data_t& vitsData() { return *vitsdata; }

	//Returns true if counter is being collected.
	bool isPerfCntAvailable(perfcnt_t cnt);

	void enablePerTaskSensing();
	void pinAllTasksToCPU(int cpu);

	void tracePerfCounter(perfcnt_t perfcnt);
	void tracePerfCounterResetAll();

	void cleanUpCreatedTasks();

	void setSim(simulation_t *sim);
};

#endif /* SENSING_MODULE_H_ */
