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

#include "tracing.h"

void TracingSystem::_init()
{
	if(rt_param_trace_core() == -1){
		arm_throw(DaemonSystemException,"tracing core not set");
	}
}

TracingSystem::~TracingSystem()
{
    //deletes all the execution tracing objects
    for(auto iter : _execTraces){
        delete iter.second;
    }
}

void TracingSystem::setup()
{
	_manager->sensingModule()->enablePerTaskSensing();
	_manager->sensingModule()->pinAllTasksToCPU(rt_param_trace_core());
	sensingWindow = _manager->addSensingWindowHandler(WINDOW_LENGTH_MS,this,window_handler);
}

void TracingSystem::window_handler(int wid,System *owner)
{
	TracingSystem* self = dynamic_cast<TracingSystem*>(owner);
	const PerformanceData& data = self->sensedData();

	for(int p = 0; p < data.numCreatedTasks(); ++p){
		//has the task executed in this epoch ?
		auto task = data.task(p);
		auto last_cpu_used = sense<SEN_LASTCPU>(&task,wid);
		if(last_cpu_used == -1) continue; //task have not executed yet
		{
		    auto trace = self->getHandleForTask(task,wid);

		    //data for this epoch
			trace("total_time_s") = sense<SEN_TOTALTIME_S>(&task,wid);
			trace("busy_time_s") = sense<SEN_BUSYTIME_S>(&task,wid);

			trace("power_w") = sense<SEN_POWER_W>(self->info()->core_list[last_cpu_used].power,wid);

			trace("freq_mhz") = sense<SEN_FREQ_MHZ>(self->info()->core_list[last_cpu_used].freq,wid);
			for(int i = 0; i < data.numMappedPerfcnts(); ++i) {
				trace(perfcnt_str(data.perfcntFromIdx(i))) = sense<SEN_PERFCNT>(data.perfcntFromIdx(i),&task,wid);
			}

			trace("nivcsw") = sense<SEN_NIVCSW>(&task,wid);
			trace("nvcsw") = sense<SEN_NVCSW>(&task,wid);

			for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) {
				trace("beats"+std::to_string(j)) = sense<SEN_BEATS>(j,&task,wid);
			}

			trace("core") = last_cpu_used;
		}
	}

}

void TracingSystem::report()
{
	ExecutionSummaryWithTracedTask db(info());
	db.setWid(sensingWindow->wid);
	db.record();
}

