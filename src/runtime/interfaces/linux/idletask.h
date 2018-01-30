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

#ifndef __arm_rt_idletask_h
#define __arm_rt_idletask_h

#include <pthread.h>
#include <semaphore.h>

#include <vector>
#include <map>
#include <limits>

#include <core/core.h>
#include <runtime/interfaces/common/perfcnts.h>
#include <runtime/interfaces/common/sense_data_shared.h>

#include <runtime/interfaces/window_manager.h>

class IdleTask
{
	enum task_state{
		RUNNING,
		IDDLED
	};

	struct IdleTaskInfo {
		IdleTaskInfo(const tracked_task_data_t& t, int w, double mu, bool util_is_ratio)
			:task(t),window(w),max_util_or_ratio(mu),
			 running_periods(0),_running_periods_left(0),idle_periods(0),_idle_periods_left(0),_curr_period_idx(0),
			 state(RUNNING),skip(false){
			_initialPeriods();
			if(util_is_ratio)_updatePeriodsRatio();
			else			_updatePeriodsUtil();
		}

		struct _period_ratio {
			const static int SIZE = 24;
			int idle;
			int running;
			double ratio;
		};
		//_period_table[0] == max ratio (1) _period_table[SIZE-1] == lowest ratio (0.1)
		static const _period_ratio _period_table[_period_ratio::SIZE];

		//removes idling from this task
		void _initialPeriods(){
			idle_periods = _period_table[0].idle;
			running_periods = _period_table[0].running;
			_idle_periods_left = idle_periods;
			_running_periods_left = running_periods;
			_curr_period_idx = 0;
		}

		const tracked_task_data_t& task;//task being idled
		int window;//sensing window to use
		double max_util_or_ratio;//idle until max util

		int running_periods;
		int _running_periods_left;
		int idle_periods;
		int _idle_periods_left;
		int _curr_period_idx;

		task_state state;//check to see if the task is currently suspended
		bool skip;//set to true if a signal fails for this task

		double _latestWindowUtil()
		{
		    return SensingInterface::sense<SEN_BUSYTIME_S>(&task,window) / SensingInterface::sense<SEN_TOTALTIME_S>(&task,window);
		}

		void _updatePeriodsUtil();

		void _updatePeriodsRatio();
		static const _period_ratio& _closestRatio(double ratio);

		void updateMaxUtil(double mu, bool util_is_ratio){
			max_util_or_ratio = mu;
			if(util_is_ratio)_updatePeriodsRatio();
			else			_updatePeriodsUtil();
		}

		//updates the task state based on the number of periods elapsed
		//returns the prevstate
		task_state updateState(int periodsElapsed);

		//max number of periods to next task state
		int toNextState() {
			if(state == RUNNING)
				return _running_periods_left;
			else
				return _idle_periods_left;
		}
	};


	SensingWindowManager &_manager;

	const int _idle_thread_period_us;

	pthread_t		_idle_thread;
	volatile bool	_idle_thread_stop;

	pthread_mutex_t _mutex;
	std::vector<IdleTaskInfo*> _tasks;
	std::map<int,IdleTaskInfo*> _tasksPidMap;



	static void* _thread_func(void*arg);
	void __thread_func();

	void _idleTask(const tracked_task_data_t& task, double max_util, int wid, bool util_is_ratio);

public:
	IdleTask(SensingWindowManager &manager,int idle_thread_period_ms);

	~IdleTask();

	//forcefully injects idle periods
	//ratio defines ratio between the forced idle periods and the
	//task normal running state
	//**ratio==1 means no idle periods,
	//**ratio==0 means task is always idle, ratio == 0.5
	//**ration==0.5 means the tasks is suspended after idle_thread_period_ms, then resumed again after idle_thread_period_ms
	void injectIdlePeriods(const tracked_task_data_t& task, double ratio, int wid);

	//start forcing a task to idle until it's utilization <= max_util
	//a task can idled using only one sensing window at a time.
	//This works by using choosing right ration for injectIdlePeriods
	//in order to maintain a certain utilization
	//(e.g. if the task normal util is already <= max_util, then a ration of 1 is applied)
	void forceMaxUtil(const tracked_task_data_t& task, double max_util, int wid);

	//equivalent to idleTask with max_util==1,
	//but allows idleTaskStart to be called again with a different window
	void idleTaskStop(const tracked_task_data_t& task);

};

class IdleTaskCPULimit
{

	struct IdleTaskInfo {
		int cpuinfoProcId;
		double maxUtil;
	};

	SensingWindowManager &_manager;
	std::map<int,IdleTaskInfo> _tasksPidMap;

	void _idleTask(const tracked_task_data_t& task, double maxUtil);
	void _idleTaskStop(int pid);

public:
	IdleTaskCPULimit(SensingWindowManager &manager):_manager(manager){}
	~IdleTaskCPULimit();

	//start forcing a task to idle until it's utilization <= max_util
	//a task can idled using only one sensing window at a time
	//does not affect task if max_util == 1
	void idleTask(const tracked_task_data_t& task, double maxUtil);

	//equivalent to idleTask with max_util==1,
	//but allows idleTaskStart to be called again with a different window
	void idleTaskStop(const tracked_task_data_t& task);

};


#endif

