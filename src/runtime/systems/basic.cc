#include "basic.h"
#include <runtime/interfaces/actuation_interface.h>

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


void OverheadTestSystem::setup()
{
	if(_mode == "overhead_test_notasksense"){
		_sensingWindow = _manager->addSensingWindowHandler(
				WINDOW_LENGTH_NO_TASK_SENSE_MS,this,
				window_handler_notasksense);
	}
	else if(_mode == "overhead_test_tasksense_coarse"){
		_manager->sensingModule()->enablePerTaskSensing();
		_sensingWindow = _manager->addSensingWindowHandler(
				WINDOW_LENGTH_PER_TASK_SENSE_COARSE_MS,this,
				window_handler_tasksense);

	}
	else if(_mode == "overhead_test_tasksense_fine"){
		_manager->sensingModule()->enablePerTaskSensing();
		_sensingWindow = _manager->addSensingWindowHandler(
				WINDOW_LENGTH_PER_TASK_SENSE_FINE_MS,this,
				window_handler_tasksense);
	}
	else arm_throw(OverheadTestSystemException,"Invalid mode = %s",_mode.c_str());
	pinfo("Overhead test with mode = %s\n",_mode.c_str());
}

void OverheadTestSystem::window_handler_notasksense(int wid,System *owner)
{

}
void OverheadTestSystem::window_handler_tasksense(int wid,System *owner)
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
			trace("total_ips") = sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,&task,wid) / sense<SEN_TOTALTIME_S>(&task,wid);

			if (sense<SEN_BUSYTIME_S>(&task,wid) == 0)
				trace("busy_ips") = 0;
			else
				trace("busy_ips") = sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,&task,wid) / sense<SEN_BUSYTIME_S>(&task,wid);

			trace("util") = sense<SEN_BUSYTIME_S>(&task,wid) / sense<SEN_TOTALTIME_S>(&task,wid);

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
		}
	}

}

void TracingSystem::report()
{
	ExecutionSummaryWithTracedTask db(info());
	db.setWid(sensingWindow->wid);
	db.record();
}

void InterfaceTest::setup()
{
	_manager->sensingModule()->enablePerTaskSensing();
	sensingWindow_fine = _manager->addSensingWindowHandler(WINDOW_LENGTH_FINE_MS,this,fine_window_handler);
	sensingWindow_coarse = _manager->addSensingWindowHandler(WINDOW_LENGTH_COARSE_MS,this,coarse_window_handler);

	_freqAct.setFrameworkMode();
	for(int domain_id = 0; domain_id < info()->power_domain_list_size; ++domain_id){
		_fd_state[domain_id] = false;
		actuate<ACT_FREQ_MHZ>(
							info()->freq_domain_list[domain_id],
							_freqAct.freqMax(info()->freq_domain_list[domain_id]));
	}
}

static char _formatstr_buff[64];
template<typename... Args>
static const char * formatstr(const char *s, Args... args){
	std::snprintf(_formatstr_buff,64,s,args...);
	return _formatstr_buff;
}

void InterfaceTest::fine_window_handler(int wid,System *owner)
{
	InterfaceTest *self =  dynamic_cast<InterfaceTest*>(owner);

	auto sensedData = self->sensingModule()->data();
	auto trace = self->_execTrace_fine.getHandle(sensedData,wid);

	//save total power
	double totalPowerW = 0;
	for(int domain_id = 0; domain_id < owner->info()->power_domain_list_size; ++domain_id){
		totalPowerW += sense<SEN_POWER_W>(&owner->info()->power_domain_list[domain_id],wid);
	}
	trace("total_power_w") = totalPowerW;

	uint64_t totalInsts = 0;
	double totalCPUTime = 0;

	for(int i = 0; i < owner->info()->core_list_size; ++i){
	    core_info_t &core = owner->info()->core_list[i];
	    totalInsts += sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,&core,wid);
		totalCPUTime += sense<SEN_BUSYTIME_S>(&core,wid);
	}
	trace("total_cpu_time_s") = totalCPUTime;
	trace("total_instr") = totalInsts;

	for(int i = 0; i < owner->info()->freq_domain_list_size; ++i){
		freq_domain_info_t &fd = owner->info()->freq_domain_list[i];

		trace(formatstr("freq_domain%d_sensed",i)) = sense<SEN_FREQ_MHZ>(&fd,wid);

		int curr = actuationVal<ACT_FREQ_MHZ>(fd);

		trace(formatstr("freq_domain%d_set",i)) = curr;

		//reached max and we were increassing freq
		if((curr >= self->_freqAct.freqMax(fd)) && self->_fd_state[i])
			self->_fd_state[i] = false;//now we decrease
		//reached min and we were decreassing freq
		else if((curr <= self->_freqAct.freqMin(fd)) && !self->_fd_state[i])
			self->_fd_state[i] = true;//now we increase

		if(self->_fd_state[i])
			actuate<ACT_FREQ_MHZ>(fd,curr+100);
		else
			actuate<ACT_FREQ_MHZ>(fd,curr-100);
	}
}

void InterfaceTest::coarse_window_handler(int wid,System *owner)
{
	InterfaceTest *self =  dynamic_cast<InterfaceTest*>(owner);

	auto sensedData = self->sensingModule()->data();
	auto trace = self->_execTrace_coarse.getHandle(sensedData,wid);

    //save total power
    double totalPowerW = 0;
    for(int domain_id = 0; domain_id < owner->info()->power_domain_list_size; ++domain_id){
        totalPowerW += sense<SEN_POWER_W>(&owner->info()->power_domain_list[domain_id],wid);
    }
    trace("total_power_w") = totalPowerW;

    uint64_t totalInsts = 0;
    double totalCPUTime = 0;

    for(int i = 0; i < owner->info()->core_list_size; ++i){
        core_info_t &core = owner->info()->core_list[i];
        totalInsts += sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,&core,wid);
        totalCPUTime += sense<SEN_BUSYTIME_S>(&core,wid);
    }
    trace("total_cpu_time_s") = totalCPUTime;
    trace("total_instr") = totalInsts;

    for(int i = 0; i < owner->info()->freq_domain_list_size; ++i){
        freq_domain_info_t &fd = owner->info()->freq_domain_list[i];

        trace(formatstr("freq_domain%d_sensed",i)) = sense<SEN_FREQ_MHZ>(&fd,wid);
	}
}

void InterfaceTest::report()
{
	ExecutionSummary db(info());
	db.setWid(sensingWindow_fine->wid);
	db.record();
}

