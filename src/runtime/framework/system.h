#ifndef __arm_rt_system_h
#define __arm_rt_system_h

#include <core/core.h>

#include <runtime/interfaces/window_manager.h>
#include <runtime/interfaces/sensed_data.h>
#include <runtime/common/reports.h>

#include "actuator.h"

class System : public ActuationInterface {
  private:
	//used to check only one system object should exist
	static bool _system_created;

	sys_info_t _sys_info;
	core_info_t _core_info_list[MAX_NR_CPUS];

	void _init_info();
	void _init_info(simulation_t *sim);
	void _sensing_setup_common();

	std::string _system_ready_file;

  protected:

	SensingWindowManager *_manager;

	System();
	System(simulation_t *sim);

	/*
	 * Called by System::start()
	 * You must implement this in order to setup your sensing windows
	 */
	virtual void setup() = 0;

	/*
	 * Called by System::stop() at the end
	 * Override to print execution repots and/or dump files with data
	 */
	virtual void report() {};

  public:
	virtual ~System();

	void start();
	void stop();

	sys_info_t* info() { return &_sys_info;}
	virtual model_sys_t* model() {return nullptr;}

	SensingModule *sensingModule() { return _manager->sensingModule(); }
	SensingWindowManager *windowManager() { return _manager; }
	const SensedData& sensedData() { return sensingModule()->data(); }

};

#endif

