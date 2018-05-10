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

#include <runtime/daemon/deamonizer.h>
#include <runtime/common/reports_deprecated.h>

class MeasuringSystem : public PolicyManager {
protected:
    static const int WINDOW_LENGTH_MS = 50;

    virtual void setup();
    virtual void report();

    const SensingWindowManager::WindowInfo *sensingWindow;

    static void window_handler(int wid,PolicyManager *owner);

private:
    TimeTracer _timeTracer;

public:
    MeasuringSystem(SensingModule *sm) :PolicyManager(sm), sensingWindow(nullptr),_timeTracer(info()){};

#if defined(IS_OFFLINE_PLAT)
    MeasuringSystem(simulation_t *sim) :PolicyManager(sim), sensingWindow(nullptr),_timeTracer(info()){};
#endif

};

void MeasuringSystem::setup()
{
    sensingModule()->enablePerTaskSensing();
    sensingWindow = windowManager()->addSensingWindowHandler(WINDOW_LENGTH_MS,this,window_handler);
    _timeTracer.setWid(sensingWindow->wid);
}

void MeasuringSystem::window_handler(int wid,PolicyManager *owner)
{
    dynamic_cast<MeasuringSystem*>(owner)->_timeTracer.record();
}

void MeasuringSystem::report()
{
    ExecutionSummary db(info());
    db.setWid(sensingWindow->wid);
    db.record();

    db.done();

    _timeTracer.done();
}

int main(int argc, char * argv[]){
    daemon_run<MeasuringSystem>(argc,argv);
    return 0;
}
