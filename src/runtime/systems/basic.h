#ifndef __arm_rt_system_basic_h
#define __arm_rt_system_basic_h

#include <runtime/framework/system.h>
#include <runtime/framework/actuator.h>
#include <runtime/interfaces/actuation_interface.h>

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
	MeasuringSystem() :System(), sensingWindow(nullptr),_timeTracer(info(),sensingModule()->data()){};

	MeasuringSystem(simulation_t *sim) :System(sim), sensingWindow(nullptr),_timeTracer(info(),sensingModule()->data()){};

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
	ExecutionTrace _execTrace;

  	void _init();

public:
  	TracingSystem() :System(), sensingWindow(nullptr),_execTrace("trace.pid0"){
  		_init();
  	};

  	TracingSystem(simulation_t *sim) :System(sim), sensingWindow(nullptr),_execTrace("trace.pid0"){
  	  		_init();
  	  	};
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

#endif

