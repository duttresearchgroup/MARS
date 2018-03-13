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

#include <runtime/framework/system.h>
#include <unordered_map>

class TracingSystem : public System {
  protected:
	static const int WINDOW_LENGTH_MS = 10;
	virtual void setup();
	virtual void report();

	const SensingWindowManager::WindowInfo *sensingWindow;

	static void window_handler(int wid,System *owner);

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

  	void _init();

public:
  	TracingSystem() :System(), sensingWindow(nullptr){
  	    _init();
  	};

  	TracingSystem(simulation_t *sim) :System(sim), sensingWindow(nullptr){
  	    _init();
  	};

  	virtual ~TracingSystem();
};


#endif

