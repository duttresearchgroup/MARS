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

#ifndef __arm_rt_system_tracing_h
#define __arm_rt_system_tracing_h

#include <unordered_map>
#include <runtime/framework/policy.h>

class TracingSystem : public PolicyManager {
  protected:
	static const int WINDOW_LENGTH_MS = 10;
	virtual void setup();
	virtual void report();

	const SensingWindowManager::WindowInfo *sensingWindow;

	static void window_handler(int wid,PolicyManager *owner);

	// Tracing system command line options
	static const std::string OPT_TRACED_CORE;
	using OPT_TRACED_CORE_TYPE = int;

	static const std::string OPT_TRACED_PERFCNTS;
	using OPT_TRACED_PERFCNTS_TYPE = std::string;

	int _traced_core;

private:
	std::unordered_map<int,ExecutionTrace*> _execTraces;

	ExecutionTrace::ExecutionTraceHandle& getHandleForTask(const tracked_task_data_t &task, int wid)
	{
	    auto iter = _execTraces.find(task.this_task_pid);
	    if(iter != _execTraces.end())
	        return iter->second->getHandle(wid);
	    else{
	        ExecutionTrace *execTrace = new ExecutionTrace("trace.pid"+std::to_string(task.this_task_pid)+"."+task.this_task_name);
	        _execTraces[task.this_task_pid] = execTrace;
	        return execTrace->getHandle(wid);
	    }
	}

public:
  	TracingSystem() :PolicyManager(), sensingWindow(nullptr),_traced_core(-1){	};

#if defined(IS_OFFLINE_PLAT)
  	TracingSystem(simulation_t *sim) :PolicyManager(sim), sensingWindow(nullptr){
  	    _init();
  	};
#endif

  	virtual ~TracingSystem();

    // Name of the columns included in the task traces.
    // For perfcnts we use the counter name given by perfcnt_str
    static const std::string& T_TOTAL_TIME_S;
    static const std::string& T_BUSY_TIME_S;
    static const std::string& T_POWER_W;
    static const std::string& T_FREQ_MHZ;
    static const std::string& T_NIVCSW;
    static const std::string& T_NVCSW;
    static const std::string& T_CORE;
    static const std::string& T_BEATS(int domain);

};


#endif

