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

#include "tracing.h"

#include <runtime/common/reports.h>
#include <runtime/common/reports_deprecated.h>

class ExecutionSummaryWithTracedTask : public ExecutionSummary {

    const tracked_task_data_t *_traced_task;

public:
    ExecutionSummaryWithTracedTask(sys_info_t *sys)
        :ExecutionSummary(sys), _traced_task(nullptr){}

    virtual ~ExecutionSummaryWithTracedTask(){ if(!_doneCalled) done(); }

protected:
    virtual void showReport();
    virtual void dump();
    virtual void wrapUp();
};

void ExecutionSummaryWithTracedTask::dump()
{
    ExecutionSummary::dump();

    if(_traced_task != nullptr){
        const PerformanceData &data = SensingModule::get().data();
        //reappend the traced task to the output file
        std::string path = _pathNameTotal();
        std::ofstream of(path,std::ios::app);
        of.precision(17);

        for(int p = 0; p < data.numCreatedTasks(); ++p)
            if(data.task(p).this_task_pid == _traced_task->this_task_pid){
                //adds domain power and freq to the traced task only
                int pd = _sys->core_list[rt_param_trace_core()].power->domain_id;
                int fd = _sys->core_list[rt_param_trace_core()].freq->domain_id;
                _d_task[p][0]->data_vals[D_IDX_POWER] = _d_pd[pd][0]->data_vals[D_IDX_POWER];
                _d_task[p][0]->data_vals[D_IDX_FREQ] = _d_fd[fd][0]->data_vals[D_IDX_FREQ];

                of << "traced.";
                _dumpTotalPrintLine(of,"",data.task(p),_d_task,p);
                break;
            }

        of.close();
    }
}

void ExecutionSummaryWithTracedTask::wrapUp()
{
    ExecutionSummary::wrapUp();

    pinfo("Wrapping up assuming tracing enabled on core %d\n",rt_param_trace_core());

    const tracked_task_data_t *traced_task = nullptr;
    _traced_task = nullptr;

    const PerformanceData &data = SensingModule::get().data();

    for(int i = 0; i < data.numCreatedTasks(); ++i){
        const tracked_task_data_t &task = data.task(i);
        int last_cpu_used = SensingInterface::senseAgg<SEN_LASTCPU>(&task,_wid);
        auto instr = SensingInterface::senseAgg<SEN_PERFCNT>(PERFCNT_INSTR_EXE,&task,_wid);
        auto busycy = SensingInterface::senseAgg<SEN_PERFCNT>(PERFCNT_BUSY_CY,&task,_wid);
        auto busytime = SensingInterface::senseAgg<SEN_BUSYTIME_S>(&task,_wid);
        if((last_cpu_used == rt_param_trace_core()) &&
           (instr > 0) && (busycy > 0) && (busytime > 0)){
            if(traced_task == nullptr) traced_task = &task;
            else if(instr > SensingInterface::senseAgg<SEN_PERFCNT>(PERFCNT_INSTR_EXE,traced_task,_wid)){
                traced_task = &task;
            }
        }
    }

    if(traced_task == nullptr){
        pinfo("Couldn't identify the traced task on core %d\n",rt_param_trace_core());
        pinfo("Retrying might fix this\n");
    }
    else {
        uint64_t instr_sum = 0;
        core_info_t *core;
        int last_cpu_used = SensingInterface::senseAgg<SEN_LASTCPU>(traced_task,_wid);
        //sums up the total ammount of instructions executed by all cores in this core's freq/power domain
        for_each_in_internal_list(_sys->core_list[last_cpu_used].power,cores,core,power_domain){
            instr_sum += SensingInterface::senseAgg<SEN_PERFCNT>(PERFCNT_INSTR_EXE,core,_wid);
        }
        double rate = SensingInterface::senseAgg<SEN_PERFCNT>(PERFCNT_INSTR_EXE,traced_task,_wid)
                      / (double) instr_sum;
        if(rate < 0.5){
            pinfo("Task %d is the traced task, but apparently too many other tasks\n",traced_task->this_task_pid);
            pinfo("executed in core %d's cluster (rate = %f). Too much interference!\n",rt_param_trace_core(),rate);
            pinfo("Retrying might fix this\n");
        }
        else if(last_cpu_used != rt_param_trace_core()){
            pinfo("Task %d is the traced task, but apparently it ran on core %d. It should run in the traced core %d.\n",traced_task->this_task_pid,last_cpu_used,rt_param_trace_core());
            pinfo("Retrying might fix this\n");
        }
        else{
            _traced_task = traced_task;
            pinfo("Task %d is the traced task\n",_traced_task->this_task_pid);
        }
    }

}

void ExecutionSummaryWithTracedTask::showReport()
{
    //Nothing else to print
}

void TracingSystem::_init()
{
	if(rt_param_trace_core() == -1){
		arm_throw(DaemonSystemException,"tracing core not set");
	}
}

TracingSystem::~TracingSystem()
{
    //deletes all the execution tracing objects
    for(auto iter : _execTraces){
        delete iter.second;
    }
}

void TracingSystem::setup()
{
	_manager->sensingModule()->enablePerTaskSensing();
	_manager->sensingModule()->pinAllTasksToCPU(rt_param_trace_core());
	sensingWindow = _manager->addSensingWindowHandler(WINDOW_LENGTH_MS,this,window_handler);
}

void TracingSystem::window_handler(int wid,System *owner)
{
	TracingSystem* self = dynamic_cast<TracingSystem*>(owner);
	const PerformanceData& data = self->sensedData();

	for(int p = 0; p < data.numCreatedTasks(); ++p){
		//has the task executed in this epoch ?
		auto task = data.task(p);
		auto last_cpu_used = sense<SEN_LASTCPU>(&task,wid);
		if(last_cpu_used == -1) continue; //task have not executed yet
		{
		    auto trace = self->getHandleForTask(task,wid);

		    //data for this epoch
			trace("total_time_s") = sense<SEN_TOTALTIME_S>(&task,wid);
			trace("busy_time_s") = sense<SEN_BUSYTIME_S>(&task,wid);

			trace("power_w") = sense<SEN_POWER_W>(self->info()->core_list[last_cpu_used].power,wid);

			trace("freq_mhz") = sense<SEN_FREQ_MHZ>(self->info()->core_list[last_cpu_used].freq,wid);
			for(int i = 0; i < data.numMappedPerfcnts(); ++i) {
				trace(perfcnt_str(data.perfcntFromIdx(i))) = sense<SEN_PERFCNT>(data.perfcntFromIdx(i),&task,wid);
			}

			trace("nivcsw") = sense<SEN_NIVCSW>(&task,wid);
			trace("nvcsw") = sense<SEN_NVCSW>(&task,wid);

			for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) {
				trace("beats"+std::to_string(j)) = sense<SEN_BEATS>(j,&task,wid);
			}

			trace("core") = last_cpu_used;
		}
	}

}

void TracingSystem::report()
{
	ExecutionSummaryWithTracedTask db(info());
	db.setWid(sensingWindow->wid);
	db.record();
}

