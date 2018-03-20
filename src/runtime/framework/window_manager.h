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

	  private:

        WindowInfo(int p,int id,PolicyManager *o)
            :period_ms(p),wid(id),owner(o)
        {
            clear_list(handlers);
        }

		struct SensingWindowFunctor {
		    const Priority priority;
		    const SensingWindowFunction _func;

		    define_list_addable(SensingWindowFunctor,handler);

		    SensingWindowFunctor(SensingWindowFunction f, Priority p)
		        :priority(p),_func(f)
		    {
		        clear_object(this,handler);
		    }

		    void operator()(int wid,PolicyManager *owner) { _func(wid,owner); }
		};

		//Linked list of handlers ordered by priority
		define_vitamins_list(SensingWindowFunctor,handlers);

		//Also store them here so we don't bother destructing them
		std::vector<SensingWindowFunctor> handlerVector;

		static inline bool sensingWindowCmp(
		        const SensingWindowManager::WindowInfo::SensingWindowFunctor *a,
		        const SensingWindowManager::WindowInfo::SensingWindowFunctor *b)
		{
		    return a->priority > b->priority;
		}

		void addHandler(SensingWindowFunction func, Priority priority){
		    handlerVector.emplace_back(func,priority);
		    SensingWindowFunctor *iter;
		    add_to_priority_list(handlers,&(handlerVector.back()),handler,sensingWindowCmp,iter);
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
};


#endif

