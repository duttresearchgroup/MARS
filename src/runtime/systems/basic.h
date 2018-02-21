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

#ifndef __arm_rt_system_basic_h
#define __arm_rt_system_basic_h

#include <runtime/framework/system.h>
#include <runtime/framework/actuator.h>
#include <runtime/interfaces/actuation_interface.h>
#include <unordered_map>

class MeasuringSystem : public System {
protected:
	static const int WINDOW_LENGTH_MS = 50;

	virtual void setup();
	virtual void report();

	const SensingWindowManager::WindowInfo *sensingWindow;

	static void window_handler(int wid,System *owner);

private:
	TimeTracer _timeTracer;

public:
	MeasuringSystem() :System(), sensingWindow(nullptr),_timeTracer(info()){};

	MeasuringSystem(simulation_t *sim) :System(sim), sensingWindow(nullptr),_timeTracer(info()){};

};

class OverheadTestSystem : public System {
protected:
	static const int WINDOW_LENGTH_NO_TASK_SENSE_MS = 1000;
	static const int WINDOW_LENGTH_PER_TASK_SENSE_COARSE_MS = 100;
	static const int WINDOW_LENGTH_PER_TASK_SENSE_FINE_MS = 10;

	virtual void setup();
	virtual void report();

	static void window_handler_notasksense(int wid,System *owner);
	static void window_handler_tasksense(int wid,System *owner);

private:
	const std::string& _mode;
	const SensingWindowManager::WindowInfo *_sensingWindow;
	ExecutionTrace _execTrace;

public:
	OverheadTestSystem(const std::string& mode) :System(), _mode(mode), _sensingWindow(nullptr), _execTrace("trace"){};

};


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


class InterfaceTest : public System {
protected:
	static const int WINDOW_LENGTH_FINE_MS = 50;
	static const int WINDOW_LENGTH_COARSE_MS = 200;

	virtual void setup();
	virtual void report();

	const SensingWindowManager::WindowInfo *sensingWindow_fine;
	const SensingWindowManager::WindowInfo *sensingWindow_coarse;

	static void fine_window_handler(int wid,System *owner);
	static void coarse_window_handler(int wid,System *owner);

	ExecutionTrace _execTrace_fine;
	ExecutionTrace _execTrace_coarse;

	FrequencyActuator _freqAct;

	//is freq increassing or decreassing ?
	std::map<int,bool> _fd_state;

public:
	InterfaceTest() :System(),
		sensingWindow_fine(nullptr),sensingWindow_coarse(nullptr),
		_execTrace_fine("execTraceFine"),_execTrace_coarse("execTraceCoarse"),
		_freqAct(*info()){};

};

class IdlePowerChecker : public System {
protected:
    static const int WINDOW_LENGTH_MS = 200;

    static const int FREQ_STEPS_MHZ = 100;

    virtual void setup();

    const SensingWindowManager::WindowInfo *sensingWindow;

    static void window_handler(int wid,System *owner);

    FrequencyActuator _freqAct;

    enum state {
        INCREASING_1,
        DECREASING_1,
        INCREASING_2,
        DECREASING_2
    };

    state _state;

    std::unordered_map<void*,ExecutionTrace*> _execTraces;

    template<typename Rsc>
    ExecutionTrace::ExecutionTraceHandle& _getTraceHandle(const std::string &type, const Rsc &rsc, int wid)
    {
        auto iter = _execTraces.find((void*)&rsc);
        if(iter != _execTraces.end())
            return iter->second->getHandle(wid);
        else{
            ExecutionTrace *execTrace = new ExecutionTrace("idle_trace."+type+"."+std::to_string(rsc.domain_id));
            _execTraces[(void*)&rsc] = execTrace;
            return execTrace->getHandle(wid);
        }
    }

public:
    IdlePowerChecker() :System(),
        sensingWindow(nullptr),
        _freqAct(*info()),_state(INCREASING_1){};

    virtual ~IdlePowerChecker(){
        for(auto iter : _execTraces){
            delete iter.second;
        }
    }

};


#endif

