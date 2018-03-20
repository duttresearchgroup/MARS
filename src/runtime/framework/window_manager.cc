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

#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <base/base.h>
#include <runtime/interfaces/common/sense_data_shared.h>
#include <runtime/interfaces/common/sensing_window_defs.h>
#include <runtime/interfaces/common/user_if_shared.h>

#include "window_manager.h"


void* SensingWindowManager::sen_win_dispatcher(void*arg){
	try {
		SensingWindowManager *wm = reinterpret_cast<SensingWindowManager*>(arg);
		SensingModule *sm = wm->sensingModule();

		sm->resgisterAsDaemonProc();

		while(sm->isSensing()){
			int wid = sm->nextSensingWindow();

			if(wid == WINDOW_EXIT) {
				break;
			}

			auto winfo = wm->_windowHandlers_idmap.find(wid);
			if(winfo == wm->_windowHandlers_idmap.end()) arm_throw(SensingWindowManagerException,"Sensing module returned unknown wid");

			for(auto &functor : winfo->second->handlers)
			    (functor)(wid,winfo->second->owner);
		}

		sm->unresgisterAsDaemonProc();

	} arm_catch(exit,EXIT_FAILURE);

	pthread_exit(nullptr);
	return nullptr;
}

SensingWindowManager::SensingWindowManager()
	:_sensingRunning(false),
	 _sen_win_dispatcher_thread(0)
{
	_sm = new SensingModule();
}

SensingWindowManager::~SensingWindowManager()
{
	cleanupWindows();

	for(auto winfo : _windowInfos) delete winfo;

	delete _sm;
}

void SensingWindowManager::startSensing()
{
	//tell module to start sensing
	_sm->sensingStart();

	int rc = pthread_create(&_sen_win_dispatcher_thread, NULL, sen_win_dispatcher, this);
	if (rc) arm_throw(SensingModuleException,"Error %d code returned from pthread_create()",rc);

	_sensingRunning = true;
}

void SensingWindowManager::stopSensing()
{
	_sm->sensingStop();
	pthread_join(_sen_win_dispatcher_thread,nullptr);
	pinfo("SensingWindowManager::stopSensing: _sen_win_dispatcher_thread joined\n");

	_sensingRunning = false;
}

const SensingWindowManager::WindowInfo* SensingWindowManager::addSensingWindowHandler(
        int period_ms,
        PolicyManager *owner,
        SensingWindowFunction func,
        Priority priority)
{
    //check if there is a window for this period
	//if there is, then append the handler, otherwise creates the window first
	if(_windowHandlers_periodmap.find(period_ms) == _windowHandlers_periodmap.end()){
		int wid = _sm->createSensingWindow(period_ms);
		WindowInfo *winfo = new WindowInfo(period_ms,wid,owner);
		_windowInfos.push_back(winfo);

		_windowHandlers_periodmap[period_ms] = winfo;
		_windowHandlers_idmap[wid] = winfo;
	}
	WindowInfo *winfo = _windowHandlers_periodmap[period_ms];
	if(winfo->owner != owner) arm_throw(SensingWindowManagerException,"Multiple systems using same window");

	winfo->addHandler(func,priority);

	return winfo;
}

void SensingWindowManager::cleanupWindows() {
	if(isSensing()) arm_throw(SensingWindowManagerException,"destroy called when sensing windows were running");
	_sm->cleanUpCreatedTasks();
}
