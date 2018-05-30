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
#include "trace_simulator.h"

#include <runtime/interfaces/common/dummy.h>

thread_local OfflineSensingModule* __localModule;

OfflineSensingModule::OfflineSensingModule(TraceSimulator *sim)
    :_sim(sim), _sys_info(sim->info()),
     _psensingManager(*this),
     _sensingRunning(false)
{

    _sensed_data = PerformanceData(_sim->perf_data());

    resgisterAsDaemonProc();

    //setup the dummy sensor
    DummySensor::create(this);
}

OfflineSensingModule::~OfflineSensingModule()
{
    if(_sensingRunning)
        sensingStop();

    //unsetup the dummy sensor
    DummySensor::destroy();
}


void OfflineSensingModule::sensingStart()
{
    if(_sensingRunning)
        arm_throw(OfflineSensingModuleException,"Sensing already running");

    perf_data_cleanup_counters(_sys_info,_sim->perf_data(),_sim->simTimeMS());

    // Start the other sensors
    _psensingManager.setSensingWindows(_sim->perf_data()->sensing_window_cnt);
    _psensingManager.startSensing();

    _sensingRunning = true;

    _sim->perf_data()->starttime_ms = _sim->simTimeMS();
}

void OfflineSensingModule::sensingStop()
{
    _sim->perf_data()->stoptime_ms = _sim->simTimeMS();

    if(_sensingRunning){
        _sim->finishSimulationStop();
        _psensingManager.stopSensing();
    }
    else
        pinfo("OfflineSensingModule::sensingStop: sensing was not running!\n");
    _sensingRunning = false;
}

int OfflineSensingModule::createSensingWindow(int period_ms)
{
    int wid = _sim->registerWindow(period_ms);
    //returned id must be either a positive integer or one of the special window IDs
    if(wid < 0){
        if(wid & WINDOW_ID_MASK){
            switch (wid) {
            case WINDOW_EXIT:
                arm_throw(OfflineSensingModuleException,"IOCTLCMD_SENSE_WINDOW_CREATE returned WINDOW_EXIT errno=%d",errno);
            case WINDOW_INVALID_PERIOD:
                arm_throw(OfflineSensingModuleException,"Sensing period must be multiple of MINIMUM_WINDOW_LENGTH_MS errno=%d",errno);
            case WINDOW_MAX_NWINDOW:
                arm_throw(OfflineSensingModuleException,"Maximum number of sensing windows created errno=%d",errno);
            case WINDOW_EXISTS:
                arm_throw(OfflineSensingModuleException,"Window for the specified period already exists errno=%d",errno);
            default:
                arm_throw(OfflineSensingModuleException,"IOCTLCMD_SENSE_WINDOW_CREATE failed errno=%d",errno);
            }
        }
        arm_throw(OfflineSensingModuleException,"IOCTLCMD_SENSE_WINDOW_CREATE failed errno=%d",errno);
    }
    else{
        return wid;
    }
}

int OfflineSensingModule::nextSensingWindow()
{
    //blocks until a window is ready
    int wid = _sim->waitForWindow();

    //returned id must be either a positive integer or the special WINDOW_EXIT IDs
    if(wid < 0){
        if(wid == WINDOW_EXIT) return wid;
        else arm_throw(OfflineSensingModuleException,"IOCTLCMD_SENSE_WINDOW_WAIT_ANY failed errno=%d",errno);
    }
    else {
        _psensingManager.windowReady(wid);
        return wid;
    }
}

bool OfflineSensingModule::isPerfCntAvailable(perfcnt_t cnt)
{
    return _sensed_data.perfCntAvailable(cnt);
}

void OfflineSensingModule::enablePerTaskSensing()
{
    //always enable on offline
}

void OfflineSensingModule::pinAllTasksToCPU(int cpu)
{
    //TODO moves all tasks to a specific CPU
    arm_throw(SensingException,"Function not implemented");
}

void OfflineSensingModule::tracePerfCounter(perfcnt_t perfcnt)
{
    if(_sensingRunning)
        arm_throw(OfflineSensingModuleException,"Cannot do tracePerfCounter with sensing running errno=%d",errno);

    if(isPerfCntAvailable(perfcnt)) return;//already enabled

    _sim->tracePerfCounter(perfcnt);
}

void OfflineSensingModule::tracePerfCounterResetAll()
{
    if(_sensingRunning)
        arm_throw(LinuxSensingModuleException,"Cannot do tracePerfCounter with sensing running errno=%d",errno);

    _sim->tracePerfCounterResetAll();
}

void OfflineSensingModule::cleanUpCreatedTasks()
{
    for(int i = 0; i < _sensed_data.numCreatedTasks(); ++i) {
        if(_sensed_data._raw_data->created_tasks[i].tsk_model != nullptr)
            delete _sensed_data._raw_data->created_tasks[i].tsk_model;
    }
    const_cast<perf_data_t*>(_sensed_data._raw_data)->created_tasks_cnt = 0;

    for(int i = 0; i < MAX_WINDOW_CNT; ++i)
        const_cast<perf_data_t*>(_sensed_data._raw_data)->sensing_windows[i].created_tasks_cnt = 0;
}


void OfflineSensingModule::resgisterAsDaemonProc()
{
    PerformanceData::localData(&_sensed_data);
    localModule(this);
}

bool OfflineSensingModule::unresgisterAsDaemonProc()
{
    //Nothing to do here
    return true;
}

void OfflineSensingModule::attachSensor(PeriodicSensor<OfflineSensingModule> *sensor)
{
    _psensingManager.attachSensor(sensor);
}

void OfflineSensingModule::sleepMS(int timeMS)
{
    _sim->sleepMS(timeMS);
}


