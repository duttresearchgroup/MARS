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

#include <runtime/interfaces/performance_data.h>
#include <runtime/interfaces/sensor.h>

class TraceSimulator;

// Needed by the ActuationInterface global functions
class OfflineSensingModule;
extern thread_local OfflineSensingModule* __localModule;

class OfflineSensingModule
{
    friend class SensingWindowManager;
    friend class TraceSimulator;

  private:

    TraceSimulator *_sim;
    sys_info_t *_sys_info;
    PeriodicSensingManager<OfflineSensingModule> _psensingManager;
    volatile bool _sensingRunning;
    PerformanceData _sensed_data;

  public:
    OfflineSensingModule(TraceSimulator *sim);

    ~OfflineSensingModule();

  private:

    static void localModule(OfflineSensingModule* sm)
    {
        __localModule = sm;
    }

  public:

    static OfflineSensingModule* localModule()
    {
        assert_true(__localModule != nullptr);
        return __localModule;
    }

    TraceSimulator* sim() { return _sim; }

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

    void attachSensor(PeriodicSensor<OfflineSensingModule> *sensor);

    void sleepMS(int timeMS);

    sys_info_t* info() { return _sys_info;}

  private:

    // Returns true if the sensing module is currently modifying the given window
    bool isUpdating(int wid) const {
        return false;
    }

    // Called by WindowManager when this window is being read
    void isReading(int wid, bool yeah) {

    }
};


#endif /* SENSING_MODULE_H_ */
