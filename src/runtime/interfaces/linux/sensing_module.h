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

#include <unistd.h>

#include <runtime/interfaces/performance_data.h>
#include <runtime/interfaces/sensor.h>

class LinuxSensingModule
{
    friend class SensingWindowManager;

  private:

	static LinuxSensingModule* _attached;

	PeriodicSensingManager<LinuxSensingModule> _psensingManager;
	int _module_file_if;
	void* _module_shared_mem_raw_ptr;
	volatile bool _sensingRunning;
	int _numCreatedWindows;
	PerformanceData _sensed_data;

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

	void attachSensor(PeriodicSensor<LinuxSensingModule> *sensor);

	void sleepMS(int timeMS)
	{
	    usleep(timeMS*1000);
	}

  private:

    // Returns true if the sensing module is currently modifying the given window
    bool isUpdating(int wid) const {
        assert_true(wid < MAX_WINDOW_CNT);
        return _sensed_data._raw_data->sensing_windows[wid].___updating;
    }

    // Called by WindowManager when this window is being read
    void isReading(int wid, bool yeah) {
        assert_true(wid < MAX_WINDOW_CNT);
        const_cast<perf_data_t*>(_sensed_data._raw_data)->sensing_windows[wid].___reading = yeah;
    }
};

#endif /* SENSING_MODULE_H_ */
