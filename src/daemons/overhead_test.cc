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

#include <daemons/common/deamonizer.h>
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
    OverheadTestSystem(SensingModule *sm) :PolicyManager(sm), _mode(OptionParser::get<std::string>("mode")), _sensingWindow(nullptr), _execTrace("trace"){};

};

#include <runtime/common/reports_deprecated.h>

void OverheadTestSystem::setup()
{
    if(_mode == "overhead_test_notasksense"){
        _sensingWindow = windowManager()->addSensingWindowHandler(
                WINDOW_LENGTH_NO_TASK_SENSE_MS,this,
                window_handler_notasksense);
    }
    else if(_mode == "overhead_test_tasksense_coarse"){
        sensingModule()->enablePerTaskSensing();
        _sensingWindow = windowManager()->addSensingWindowHandler(
                WINDOW_LENGTH_PER_TASK_SENSE_COARSE_MS,this,
                window_handler_tasksense);

    }
    else if(_mode == "overhead_test_tasksense_fine"){
        sensingModule()->enablePerTaskSensing();
        _sensingWindow = windowManager()->addSensingWindowHandler(
                WINDOW_LENGTH_PER_TASK_SENSE_FINE_MS,this,
                window_handler_tasksense);
    }
    else arm_throw(OverheadTestSystemException,"Invalid mode = %s",_mode.c_str());
    pinfo("Overhead test with mode = %s\n",_mode.c_str());
}

void OverheadTestSystem::window_handler_notasksense(int wid,PolicyManager *owner)
{

}
void OverheadTestSystem::window_handler_tasksense(int wid,PolicyManager *owner)
{
    OverheadTestSystem *self =  dynamic_cast<OverheadTestSystem*>(owner);
    auto sensedData = self->sensingModule()->data();
    auto trace = self->_execTrace.getHandle(sensedData,wid);

    //sums up the number of instruction executed by the ubench task
    uint64_t instructions = 0;
    int tasks = 0;
    for(int t = 0; t < sensedData.numCreatedTasks(); ++t){
        auto task = sensedData.task(t);
        if((task.this_task_name[0] == 'u') && (task.this_task_name[1] == 'b') && (task.this_task_name[5] == 'h')){
            instructions += sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,&task,wid);
            tasks += 1;
        }
    }
    trace("ubench_instructions") = instructions;
    trace("ubench_tasks") = tasks;
}


void OverheadTestSystem::report()
{
    ExecutionSummary db(info());
    db.setWid(_sensingWindow->wid);
    db.record();
}

int main(int argc, char * argv[]){
	daemon_run<OverheadTestSystem>(argc,argv);
	return 0;
}
