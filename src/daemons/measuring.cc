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

#if defined(IS_OFFLINE_PLAT)
    MeasuringSystem(simulation_t *sim) :System(sim), sensingWindow(nullptr),_timeTracer(info()){};
#endif

};

void MeasuringSystem::setup()
{
    _manager->sensingModule()->enablePerTaskSensing();
    sensingWindow = _manager->addSensingWindowHandler(WINDOW_LENGTH_MS,this,window_handler);
    _timeTracer.setWid(sensingWindow->wid);
}

void MeasuringSystem::window_handler(int wid,System *owner)
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
    daemon_setup(argc,argv);
    daemon_run_sys<MeasuringSystem>();
    return 0;
}
