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

#ifndef __arm_rt_sensing_window_manager_h
#define __arm_rt_sensing_window_manager_h

#include <pthread.h>

#include <vector>
#include <map>
#include <set>
#include <limits>

#include <runtime/interfaces/sensing_module.h>

#include <base/base.h>
#include <runtime/interfaces/common/perfcnts.h>
#include <runtime/interfaces/common/sense_data_shared.h>

//forward system declaration
class PolicyManager;

class SensingWindowManager
{
    friend class PolicyManager;

  public:
	typedef void (*SensingWindowFunction)(int,PolicyManager*);

	typedef int Priority;

	static constexpr Priority PRIORITY_MIN = std::numeric_limits<Priority>::lowest();
	static constexpr Priority PRIORITY_DEFAULT = 0;
	static constexpr Priority PRIORITY_MAX = std::numeric_limits<Priority>::max();

	struct WindowInfo {
	    friend class SensingWindowManager;

		const int period_ms;
		const int wid;
		PolicyManager *const owner;

		int timestamp() { return _timestamp; }

	  private:

		// A virtual timestamp, incremented by period_ms everytime this
		// window is ready
		int _timestamp;

        WindowInfo(int p,int id,PolicyManager *o)
            :period_ms(p),wid(id),owner(o),_timestamp(0)
        {
        }

		struct SensingWindowFunctor {
		    const Priority priority;
		    const SensingWindowFunction _func;

		    SensingWindowFunctor(SensingWindowFunction f, Priority p)
		        :priority(p),_func(f)
		    {}

		    void operator()(int wid,PolicyManager *owner) const { _func(wid,owner); }
		};


		struct sensingWindowCmp {
		    bool operator()(const SensingWindowFunctor &a, const SensingWindowFunctor &b)
		    { return a.priority >= b.priority; }
		};

		// Handlers ordered by priority
		std::multiset<SensingWindowFunctor,sensingWindowCmp> handlers;


		void addHandler(SensingWindowFunction func, Priority priority){
		    handlers.emplace(func,priority);
		}
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

    void startSensing();
    void stopSensing();

  public:

	SensingWindowManager();

	~SensingWindowManager();

  public:

	const WindowInfo* addSensingWindowHandler(int period_ms, PolicyManager *owner, SensingWindowFunction func, Priority priority = PRIORITY_DEFAULT);

	bool isSensing() const { return _sensingRunning; }

	SensingModule *sensingModule() { return _sm; }

	const PerformanceData& sensingData() const { return _sm->data(); }

	const WindowInfo* winfo(int wid) {
	    auto i = _windowHandlers_idmap.find(wid);
	    if(i == _windowHandlers_idmap.end())
	        arm_throw(SensingWindowManagerException,"Invalid window id = %d",wid);
	    return i->second;
	}
};


#endif

