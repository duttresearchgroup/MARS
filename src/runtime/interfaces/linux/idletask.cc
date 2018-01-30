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

#include <atomic>
#include <cmath>
#include <unistd.h>
#include <signal.h>
#include "idletask.h"

#include <external/cpulimit/src/cpulimit.h>


void* IdleTask::_thread_func(void*arg)
{
	try {
		IdleTask *idle_task = reinterpret_cast<IdleTask*>(arg);
		idle_task->__thread_func();
	} arm_catch(ARM_CATCH_NO_EXIT);
	pthread_exit(nullptr);
	return nullptr;
}

IdleTask::task_state IdleTask::IdleTaskInfo::updateState(int periodsElapsed)
{
	task_state prev = state;

	if(max_util_or_ratio == 1) {//corner case 1
		state = RUNNING;
	}
	else if(max_util_or_ratio == 0) {//corner case 2
		state = IDDLED;
	}
	else if(idle_periods==0){//never sleep case
		state = RUNNING;
	}
	else if(state == RUNNING){
		_running_periods_left -= periodsElapsed;
		if(_running_periods_left <= 0){
			state = IDDLED;
			_running_periods_left = running_periods;
		}
	}
	else {//state == IDDLED
		_idle_periods_left -= periodsElapsed;
		if(_idle_periods_left <= 0){
			state = RUNNING;
			_idle_periods_left = idle_periods;
		}
	}
	return prev;
}


const IdleTask::IdleTaskInfo::_period_ratio IdleTask::IdleTaskInfo::_period_table[_period_ratio::SIZE] =
{		{0,std::numeric_limits<int>::max(),1},
		{1,9,0.900000},
		{1,8,0.888889},
		{1,7,0.875000},
		{1,6,0.857143},
		{1,5,0.833333},
		{1,4,0.800000},
		{2,7,0.777778},
		{1,3,0.750000},
		{2,5,0.714286},
		{1,2,0.666667},
		{2,3,0.600000},
		{1,1,0.500000},
		{3,2,0.400000},
		{2,1,0.333333},
		{5,2,0.285714},
		{3,1,0.250000},
		{7,2,0.222222},
		{4,1,0.200000},
		{5,1,0.166667},
		{6,1,0.142857},
		{7,1,0.125000},
		{8,1,0.111111},
		{9,1,0.100000}};

void IdleTask::IdleTaskInfo::_updatePeriodsUtil()
{
	double curr_diff = _latestWindowUtil() - max_util_or_ratio ;

	int prev_period = _curr_period_idx;

	//increase/decrease the ratio until we converge to the util
	if(curr_diff > 0) _curr_period_idx += 1;
	else if(curr_diff < 0) _curr_period_idx -= 1;

	if(_curr_period_idx < 0) _curr_period_idx = 0;
	if(_curr_period_idx >= _period_ratio::SIZE) _curr_period_idx = _period_ratio::SIZE-1;

	if(std::abs(curr_diff) < (std::abs(_period_table[_curr_period_idx].ratio-_period_table[prev_period].ratio)/2))
		_curr_period_idx = prev_period;

	if(_curr_period_idx != prev_period){
		running_periods = _period_table[_curr_period_idx].running;
		_running_periods_left = running_periods;
		idle_periods = _period_table[_curr_period_idx].idle;
		_idle_periods_left = idle_periods;
		//pinfo("task %d rp=%d ip=%d currdiff=%f ratio=%f\n",task.this_task_pid,running_periods,idle_periods,curr_diff,std::abs(_period_table[_curr_period_idx].ratio-_period_table[prev_period].ratio));
	}
}


void IdleTask::IdleTaskInfo::_updatePeriodsRatio()
{
	int prev_period = _curr_period_idx;

	//finds the idle/sleep count that gives the closest ratio
	int _curr_period_idx = 0;
	double err = std::abs(max_util_or_ratio-_period_table[_curr_period_idx].ratio);
	for(int i = 1; i < _period_ratio::SIZE; ++i){
        double newErr = std::abs(max_util_or_ratio-_period_table[i].ratio);
	    if(newErr < err){
		    err = newErr;
		    _curr_period_idx = i;
        }
	}

	if(_curr_period_idx != prev_period){
		running_periods = _period_table[_curr_period_idx].running;
		_running_periods_left = running_periods;
		idle_periods = _period_table[_curr_period_idx].idle;
		_idle_periods_left = idle_periods;
		//pinfo("task %d rp=%d ip=%d currdiff=%f ratio=%f\n",task.this_task_pid,running_periods,idle_periods,curr_diff,std::abs(_period_table[_curr_period_idx].ratio-_period_table[prev_period].ratio));
	}
}

void IdleTask::__thread_func()
{
	unsigned int sleep_periods = 1;
	//const unsigned int sleep_periods_max = 1;//10;

	pinfo("Idle task thread started\n");

	while(!_idle_thread_stop){
		usleep(sleep_periods*_idle_thread_period_us);

		//bool any_task_changed_state = false;

		pthread_mutex_lock(&_mutex);
		for(auto task : _tasks){
			if(task->skip) continue;
			task_state prev = task->updateState(sleep_periods);
			//implement the task state if something changes
			if(prev!=task->state){
				//pinfo("Task pid %d (%s) state changed %u ->%u, calc util %f, w util %f\n",task->task.this_task_pid,task->task.this_task_name,task->state,next,(double)(task->currTimeBusyMS() - task->last_time_busy_acc_ms)/(double)(_idle_thread_timer_ms - task->last_update_time_ms),(double)task->window.curr.tasks[task->task.task_idx].perfcnt.time_busy_ms / (double)task->window.curr.tasks[task->task.task_idx].perfcnt.time_total_ms);
				switch (task->state) {
					case RUNNING:
						if(kill(task->task.this_task_pid, SIGCONT)){
							//pinfo("kill(SIGCONT) failed for task pid %d (%s) \n",task->task.this_task_pid,task->task.this_task_name);
							task->skip = true;
						}
						break;
					case IDDLED:
						if(kill(task->task.this_task_pid,SIGSTOP)){
							//pinfo("kill(SIGSTOP) failed for task pid %d (%s) \n",task->task.this_task_pid,task->task.this_task_name);
							task->skip = true;
						}
						break;
					default: break;
				}
				//any_task_changed_state = true;
			}
		}
		pthread_mutex_unlock(&_mutex);

		//for every iteration that nothing changed we decrease the rate
		//we are checking
		//if(any_task_changed_state) sleep_periods = 1;
		//else sleep_periods += 1;
		//if(sleep_periods > sleep_periods_max) sleep_periods = sleep_periods_max;
	}
	pinfo("Idle task thread ended\n");
}


IdleTask::IdleTask(SensingWindowManager &manager,int idle_thread_period_ms)
	:_manager(manager), _idle_thread_period_us(idle_thread_period_ms*1000), _idle_thread(0), _idle_thread_stop(false)
{
	if(pthread_mutex_init(&_mutex,nullptr)) arm_throw(IdleTaskException,"pthread_mutex_init failed with errno %d",errno);

	int rc = pthread_create(&_idle_thread, NULL, _thread_func, this);
	if (rc) arm_throw(IdleTaskException,"Error code %d returned from pthread_create()",rc);
}

IdleTask::~IdleTask()
{
	pthread_mutex_lock(&_mutex);
	_idle_thread_stop = true;
	for(auto task : _tasks){
		//let all continue if stopped
		kill(task->task.this_task_pid, SIGCONT);
		delete task;
	}
	_tasks.clear();
	pthread_mutex_unlock(&_mutex);

	pthread_join(_idle_thread,nullptr);


}

void IdleTask::_idleTask(const tracked_task_data_t& task, double max_util, int wid, bool util_is_ratio)
{
	if((max_util < 0) || (max_util>1)) pinfo("%f\n",max_util);
	assert_false((max_util < 0) || (max_util>1));

	if(_tasksPidMap.find(task.this_task_pid)==_tasksPidMap.end()){
		IdleTaskInfo *info = new IdleTaskInfo(task,wid,max_util,util_is_ratio);
		_tasksPidMap[task.this_task_pid] = info;
		//pinfo("Iddling tsk %d(%s) max_util=%f\n",task.this_task_pid,task.this_task_name,max_util);
		pthread_mutex_lock(&_mutex);
		_tasks.push_back(info);
		pthread_mutex_unlock(&_mutex);
	}
	else{
		assert_false(_tasksPidMap[task.this_task_pid]->window != wid);
		_tasksPidMap[task.this_task_pid]->updateMaxUtil(max_util,util_is_ratio);
	}
}

void IdleTask::forceMaxUtil(const tracked_task_data_t& task, double max_util, int wid)
{
	_idleTask(task,max_util,wid,false);
}

void IdleTask::injectIdlePeriods(const tracked_task_data_t& task, double max_util, int wid)
{
	_idleTask(task,max_util,wid,true);
}


//equivalent to idleTaskUpdate with max_util==1,
//but allows idleTaskStart to be called again with a different window
void IdleTask::idleTaskStop(const tracked_task_data_t& task)
{
	assert_false(_tasksPidMap.find(task.this_task_pid)==_tasksPidMap.end());

	_tasksPidMap.erase(_tasksPidMap.find(task.this_task_pid));

	pthread_mutex_lock(&_mutex);
		for(unsigned i = 0; i < _tasks.size();++i){
			if(_tasks[i]->task.this_task_pid == task.this_task_pid){
				_tasks.erase(_tasks.begin()+i);
				break;
			}
		}
	pthread_mutex_unlock(&_mutex);
}


void IdleTaskCPULimit::_idleTask(const tracked_task_data_t& task, double maxUtil)
{
	//crteate
	//_tasksPidMap[task.this_task_pid] = info;

	//fork a new proc and call cpulimit
	pid_t pid = fork();

	/* An error occurred */
	if (pid < 0){
		pinfo("Iddling tsk %d(%s) cpulimit fork error\n",task.this_task_pid,task.this_task_name);
		return;
	}

	/* That us*/
	if (pid > 0){
		_tasksPidMap[task.this_task_pid] = {pid,maxUtil};
		pinfo("Iddling tsk %d(%s) max_util=%f cpulimit=%d\n",task.this_task_pid,task.this_task_name,maxUtil,pid);
	}
	else{
		//cpulimit stuff
		signal(SIGQUIT, SIG_IGN);//disable the handler inherited from parent daemon
		//_manager.sensingModule()->forceDetach();//disconect this guy from the sensing module
		pinfo("cpulimit=%d exec\n",getpid());
		limit_process(task.this_task_pid,maxUtil,false);
		exit(0);
	}
}

void IdleTaskCPULimit::idleTask(const tracked_task_data_t& task, double maxUtil)
{
	const double error = 0.05;
	assert_false((maxUtil < 0) || (maxUtil>1));

	if(_tasksPidMap.find(task.this_task_pid)==_tasksPidMap.end()){
		_idleTask(task,maxUtil);
	}
	else{
		double currMaxUtil = _tasksPidMap[task.this_task_pid].maxUtil;
		if((maxUtil < (currMaxUtil-error) || (maxUtil > (currMaxUtil+error)))){
			idleTaskStop(task);
			_idleTask(task,maxUtil);
		}
	}
}

void IdleTaskCPULimit::_idleTaskStop(int pid)
{
	kill(_tasksPidMap[pid].cpuinfoProcId,SIGINT);
}

void IdleTaskCPULimit::idleTaskStop(const tracked_task_data_t& task)
{
	assert_false(_tasksPidMap.find(task.this_task_pid)==_tasksPidMap.end());
	_idleTaskStop(task.this_task_pid);
	_tasksPidMap.erase(_tasksPidMap.find(task.this_task_pid));
}

IdleTaskCPULimit::~IdleTaskCPULimit()
{
	for (auto i : _tasksPidMap) _idleTaskStop(i.first);
}
