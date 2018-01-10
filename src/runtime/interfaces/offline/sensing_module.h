/*
 * sensing_module.h
 *
 *  Created on: Dec 8, 2016
 *      Author: tiago
 */

#ifndef OFFLINE_SENSING_MODULE_H_
#define OFFLINE_SENSING_MODULE_H_

#include <runtime/interfaces/sensed_data.h>
#include <offline_sim/exec_sim.h>

class OfflineSensingModule
{
  private:

	static bool _attached;

	int _module_file_if;
	void* _module_shared_mem_raw_ptr;
	volatile bool _sensingRunning;
	SensedData _sensed_data;
	sensed_data_t *vitsdata;

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

	void reset_perf_counters(sensed_data_perf_counters_t *sen_data);
	void reset_task_counters(int cpu,sensed_data_task_t *sen_data);
	void reset_cpu_counters(sensed_data_cpu_t *sen_data);
	//void reset_power_counters(sensed_data_power_domain_t *sen_data);
	void reset_freq_counters(sensed_data_freq_domain_t *sen_data);

	void vit_map_perfcnt();

  public:
	OfflineSensingModule();

	~OfflineSensingModule();

	//disconnects this object from the module without checks
	//will make this object invalid
	void forceDetach();

  public:

	void sensingStart();
	void sensingStop();

	bool isSensing() { return _sensingRunning; }

	int createSensingWindow(int period_ms);

	int nextSensingWindow();

	void resgisterAsDaemonProc();//registers calling process as a daemon process
	bool unresgisterAsDaemonProc();

	const SensedData& data() { return _sensed_data; }
	const sensed_data_t& vitsData() { return *vitsdata; }

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
