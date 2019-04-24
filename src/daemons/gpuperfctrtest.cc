/*******************************************************************************
 * Copyright (C) 2019 Saehanseul Yi <saehansy@uci.edu>
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
#include <runtime/framework/actuation_interface.h>
#include <runtime/common/strings.h>
#include <external/hookcuda/nvidia_counters.h>
#include <map>

class GpuPerfCtrTest : public PolicyManager {
protected:
    static const int WINDOW_LENGTH_FINE_MS = 50;
    static const int WINDOW_LENGTH_COARSE_MS = 200;

    virtual void setup();

    const SensingWindowManager::WindowInfo *sensingWindow_fine;
    const SensingWindowManager::WindowInfo *sensingWindow_coarse;

    static void fine_window_handler(int wid,PolicyManager *owner);
    static void coarse_window_handler(int wid,PolicyManager *owner);

    ExecutionTrace _execTrace_fine;
    ExecutionTrace _execTrace_coarse;

    //is freq increassing or decreassing ?
    bool _freq_state;

public:
    GpuPerfCtrTest(SensingModule *sm) :PolicyManager(sm),
        sensingWindow_fine(nullptr),sensingWindow_coarse(nullptr),
        _execTrace_fine("execTraceFine"),_execTrace_coarse("execTraceCoarse")
        {
	};
};

void GpuPerfCtrTest::setup()
{
    sensingModule()->enablePerTaskSensing();
    sensingWindow_fine = windowManager()->addSensingWindowHandler(WINDOW_LENGTH_FINE_MS,this,fine_window_handler);
    sensingWindow_coarse = windowManager()->addSensingWindowHandler(WINDOW_LENGTH_COARSE_MS,this,coarse_window_handler);

    enableSensor<SEN_NV_GPU_PERFCNT>(INST_EXECUTED, nullResource(), true);
}

void GpuPerfCtrTest::fine_window_handler(int wid,PolicyManager *owner)
{
    static int count = 1;
    GpuPerfCtrTest *self =  dynamic_cast<GpuPerfCtrTest*>(owner);

    auto sensedData = self->sensingModule()->data();
    auto trace = self->_execTrace_fine.getHandle(sensedData,wid);

    GpuPerfCtrRes inst_executed = sense<SEN_NV_GPU_PERFCNT>(INST_EXECUTED, nullResource(), wid);
    for (int i = 0 ; i < inst_executed.num_kernels ; ++i)
    {
	char name[128];
	if (inst_executed.kernels[i].num_launched > 0)
	{
	    sprintf(name, "num_launched#%d", i);
	    trace(name) = inst_executed.kernels[i].num_launched;
	    sprintf(name, "instr#%d", i);
	    trace(name) = inst_executed.kernels[i].mres[INST_EXECUTED];
	    sprintf(name, "duration#%d", i);
	    trace(name) = inst_executed.kernels[i].pure_kernel_duration/1000000.0;
//	    printf("#%d window: kernel#%d name=%s\n", count, i, inst_executed.kernels[i].name);
	}
    }
    count++;
}

void GpuPerfCtrTest::coarse_window_handler(int wid,PolicyManager *owner)
{
    static int count = 1;
    GpuPerfCtrTest *self =  dynamic_cast<GpuPerfCtrTest*>(owner);

    auto sensedData = self->sensingModule()->data();
    auto trace = self->_execTrace_coarse.getHandle(sensedData,wid);

    GpuPerfCtrRes inst_executed = senseAgg<SEN_NV_GPU_PERFCNT>(INST_EXECUTED, nullResource(), wid);
    for (int i = 0 ; i < inst_executed.num_kernels ; ++i)
    {
	char name[128];
	if (inst_executed.kernels[i].num_launched > 0)
	{
	    sprintf(name, "num_launched#%d", i);
	    trace(name) = inst_executed.kernels[i].num_launched;
	    sprintf(name, "instr#%d", i);
	    trace(name) = inst_executed.kernels[i].mres[INST_EXECUTED];
	    sprintf(name, "duration#%d", i);
	    trace(name) = inst_executed.kernels[i].pure_kernel_duration/1000000.0;
//	    printf("#%d window: kernel#%d name=%s\n", count, i, inst_executed.kernels[i].name);
	}
    }
    count++;
}

int main(int argc, char * argv[]){
	daemon_run<GpuPerfCtrTest>(argc,argv);
	return 0;
}
