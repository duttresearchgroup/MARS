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

#ifndef __arm_rt_system_overheadtest_h
#define __arm_rt_system_overheadtest_h

#include <runtime/framework/policy.h>

class OverheadTestSystem : public PolicyManager {
protected:
	static const int WINDOW_LENGTH_NO_TASK_SENSE_MS = 1000;
	static const int WINDOW_LENGTH_PER_TASK_SENSE_COARSE_MS = 100;
	static const int WINDOW_LENGTH_PER_TASK_SENSE_FINE_MS = 10;

	virtual void setup();
	virtual void report();

	static void window_handler_notasksense(int wid,PolicyManager *owner);
	static void window_handler_tasksense(int wid,PolicyManager *owner);

private:
	const std::string& _mode;
	const SensingWindowManager::WindowInfo *_sensingWindow;
	ExecutionTrace _execTrace;

public:
	OverheadTestSystem(const std::string& mode) :PolicyManager(), _mode(mode), _sensingWindow(nullptr), _execTrace("trace"){};

};


#endif

