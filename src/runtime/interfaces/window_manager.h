#ifndef __arm_rt_sensing_window_manager_h
#define __arm_rt_sensing_window_manager_h

#include <pthread.h>

#include <vector>
#include <map>

#include "sensing_module.h"

#include <core/core.h>
#include <runtime/interfaces/common/perfcnts.h>
#include <runtime/interfaces/common/sense_data_shared.h>

//forward system declaration
class System;

class SensingWindowManager
{
  public:
	typedef void (*SensingWindowFunction)(int,System*);

	struct WindowInfo {
		WindowInfo(int p,int id,System *o):period_ms(p),wid(id),owner(o){}
		int period_ms;
		int wid;
		System *owner;
		std::vector<SensingWindowFunction> handlers;
	};

  private:

	volatile bool _sensingRunning;

	pthread_t _sen_win_dispatcher_thread;

	std::vector<WindowInfo*>  _windowInfos;
	std::map<int,WindowInfo*> _windowHandlers_idmap;
	std::map<int,WindowInfo*> _windowHandlers_periodmap;

	SensingModule *_sm;

	static void* sen_win_dispatcher(void*arg);

	void cleanupWindows();

  public:

	SensingWindowManager();

	~SensingWindowManager();

  public:

	void startSensing();
	void stopSensing();

	const WindowInfo* addSensingWindowHandler(int period_ms, System *owner, SensingWindowFunction func);

	bool isSensing() { return _sensingRunning; }

	SensingModule *sensingModule() { return _sm; }

	const PerformanceData& sensingData() { return _sm->data(); }
};


#endif

