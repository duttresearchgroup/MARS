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

#ifndef LINUX_SENSING_MODULE_H_
#define LINUX_SENSING_MODULE_H_

#include "../performance_data.h"
#include "sensor.h"

class LinuxSensingModule
{
  private:

	static LinuxSensingModule* _attached;

	int _module_file_if;
	void* _module_shared_mem_raw_ptr;
	volatile bool _sensingRunning;
	int _numCreatedWindows;
	PerformanceData _sensed_data;
	PeriodicSensingManager _psensingManager;

  public:
	LinuxSensingModule();

	~LinuxSensingModule();

	//disconnects this object from the module without checks
	//will make this object invalid
	void forceDetach();

	static LinuxSensingModule& get()
	{
		if(_attached == nullptr)
			arm_throw(LinuxSensingModuleException,"Sensing module not attached");
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

	//Returns true if counter is being collected.
	bool isPerfCntAvailable(perfcnt_t cnt);

	void enablePerTaskSensing();
	void pinAllTasksToCPU(int cpu);

	void tracePerfCounter(perfcnt_t perfcnt);
	void tracePerfCounterResetAll();

	void cleanUpCreatedTasks();

	void attachSensor(PeriodicSensor *sensor);
};

#endif /* SENSING_MODULE_H_ */
