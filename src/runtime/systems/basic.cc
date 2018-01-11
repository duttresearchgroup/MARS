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
	ExecutionSummary db(info(),_manager->sensingModule()->data());
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
			instructions += sensedData.getPerfcntVal(
					sensedData.swCurrData(wid).tasks[t].perfcnt,
					PERFCNT_INSTR_EXE);
			tasks += 1;
		}
	}
	trace("ubench_instructions") = instructions;
	trace("ubench_tasks") = tasks;
}


void OverheadTestSystem::report()
{
	ExecutionSummary db(info(),_manager->sensingModule()->data());
	db.setWid(_sensingWindow->wid);
	db.record();
}


void TracingSystem::_init()
{
	if(rt_param_trace_core() == -1){
		arm_throw(DaemonSystemException,"tracing core not set");
	}
}


void TracingSystem::setup()
{
	_manager->sensingModule()->enablePerTaskSensing();
	_manager->sensingModule()->pinAllTasksToCPU(rt_param_trace_core());
	sensingWindow = _manager->addSensingWindowHandler(WINDOW_LENGTH_MS,this,window_handler);
//	_timeTracer.setWid(sensingWindow->wid);
}

void TracingSystem::window_handler(int wid,System *owner)
{
	TracingSystem* self = dynamic_cast<TracingSystem*>(owner);
	const PerformanceData& data = self->sensedData();
	auto trace = self->_execTrace.getHandle(data,wid);
	const perf_window_data_t& sw = data.swCurrData(wid);

///////////////////////////// this is what we record //////////////////////
//	data_names.push_back("total_time_s"); data_agg_att.push_back(traced_data::AGG_MAX);
//	data_names.push_back("busy_time_s"); data_agg_att.push_back(traced_data::AGG_SUM);
//	data_names.push_back("total_ips"); data_agg_att.push_back(traced_data::AGG_SUM);
//	data_names.push_back("busy_ips"); data_agg_att.push_back(traced_data::AGG_SUM);
//	data_names.push_back("util"); data_agg_att.push_back(traced_data::AGG_SUM);
//	data_names.push_back("power_w"); data_agg_att.push_back(traced_data::AGG_SUM);
//	data_names.push_back("freq_mhz"); data_agg_att.push_back(traced_data::AGG_NOPE);
//	for(int i = 0; i < _data.numMappedPerfcnts(); ++i) {
//		data_names.push_back(perfcnt_str(_data.perfcntFromIdx(i)));
//		data_agg_att.push_back(traced_data::AGG_SUM);
//	}
//	data_names.push_back("nivcsw"); data_agg_att.push_back(traced_data::AGG_SUM);
//	data_names.push_back("nvcsw"); data_agg_att.push_back(traced_data::AGG_SUM);
//	for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) {
//		data_names.push_back("beats"+std::to_string(j)); data_agg_att.push_back(traced_data::AGG_SUM);
//	}
//	for (auto i: a_args) {
//		data_names.push_back(i); data_agg_att.push_back(traced_data::AGG_NOPE);
//	}
//////////////////////////


//	for(int cpu = 0; cpu < self->info()->core_list_size; ++cpu){
//		trace("total_time_s") = ((double)sw.cpus[cpu].perfcnt.time_total_ms/1000.0);
//		trace("busy_time_s") = ((double)sw.cpus[cpu].perfcnt.time_busy_ms/1000.0);
//		trace("total_ips") = ((double)sw.cpus[cpu].perfcnt.perfcnts[self->sensingModule()->vitsData().perfcnt_to_idx_map[PERFCNT_INSTR_EXE]] / (double)sw.cpus[cpu].perfcnt.time_total_ms * 1000.0);
//		if (sw.cpus[cpu].perfcnt.time_busy_ms == 0)
//			trace("busy_ips") = 0;
//		else
//			trace("busy_ips") = ((double)sw.cpus[cpu].perfcnt.perfcnts[self->sensingModule()->vitsData().perfcnt_to_idx_map[PERFCNT_INSTR_EXE]] / (double)sw.cpus[cpu].perfcnt.time_busy_ms * 1000.0);
//		trace("util") = ((double)sw.cpus[cpu].perfcnt.time_busy_ms / (double)sw.cpus[cpu].perfcnt.time_total_ms);
//		trace("power_w") = ((double)sw.power_domains[self->info()->core_list[cpu].power->domain_id].avg_power_uW_acc / (double)sw.power_domains[self->info()->core_list[cpu].power->domain_id].time_ms_acc);
//		trace("freq_mhz") = sw.freq_domains[self->info()->core_list[cpu].freq->domain_id].avg_freq_mhz_acc;
//		for(int i = 0; i < data.numMappedPerfcnts(); ++i) {
//			trace(perfcnt_str(data.perfcntFromIdx(i))) = sw.cpus[cpu].perfcnt.perfcnts[i];
//		}
//		trace("nivcsw") = sw.cpus[cpu].perfcnt.nivcsw;
//		trace("nvcsw") = sw.cpus[cpu].perfcnt.nvcsw;
//		for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) {
//			trace("beats"+std::to_string(j)) = sw.cpus[cpu].beats[j];
//		}
//	}
//	for(int power_domain = 0; power_domain < _sys->power_domain_list_size; ++power_domain){
//		_d_pd[power_domain].push_back(new traced_data(*this,sw.power_domains[power_domain],a_args));
//	}
//	for(int freq_domain = 0; freq_domain < _sys->freq_domain_list_size; ++freq_domain){
//		_d_fd[freq_domain].push_back(new traced_data(*this,sw.freq_domains[freq_domain],a_args));
//	}
	for(int p = 0; p < data.numCreatedTasks(); ++p){
		assert(p==0);
		//has the task executed in this epoch ?
		if(sw.tasks[p].last_cpu_used == -1) continue; //task have not executed yet
		{
			//data for this epoch
			trace("total_time_s") = ((double)sw.tasks[p].perfcnt.time_total_ms/1000.0);
			trace("busy_time_s") = ((double)sw.tasks[p].perfcnt.time_busy_ms/1000.0);
			trace("total_ips") = ((double)data.getPerfcntVal(sw.tasks[p].perfcnt, PERFCNT_INSTR_EXE) / (double)sw.tasks[p].perfcnt.time_total_ms * 1000.0);
			if (sw.tasks[p].perfcnt.time_busy_ms == 0)
				trace("busy_ips") = 0;
			else
				trace("busy_ips") = ((double)data.getPerfcntVal(sw.tasks[p].perfcnt, PERFCNT_INSTR_EXE) / (double)sw.tasks[p].perfcnt.time_busy_ms * 1000.0);
			trace("util") = ((double)sw.tasks[p].perfcnt.time_busy_ms / (double)sw.tasks[p].perfcnt.time_total_ms);
//			trace("power_w") = ((double)sw.power_domains[self->info()->core_list[sw.tasks[p].last_cpu_used].power->domain_id].avg_power_uW_acc / (double)sw.power_domains[self->info()->core_list[sw.tasks[p].last_cpu_used].power->domain_id].time_ms_acc / 1000.0);
			trace("power_w") = sense<SEN_POWER_W>(self->info()->core_list[sw.tasks[p].last_cpu_used].power,wid);
			trace("freq_mhz") = sw.freq_domains[self->info()->core_list[sw.tasks[p].last_cpu_used].freq->domain_id].avg_freq_mhz_acc;
			for(int i = 0; i < data.numMappedPerfcnts(); ++i) {
				trace(perfcnt_str(data.perfcntFromIdx(i))) = sw.tasks[p].perfcnt.perfcnts[i];
			}
			trace("nivcsw") = sw.tasks[p].perfcnt.nivcsw;
			trace("nvcsw") = sw.tasks[p].perfcnt.nvcsw;
			for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) {
				trace("beats"+std::to_string(j)) = sw.tasks[p].beats[j];
			}
		}
	}

}

void TracingSystem::report()
{
	ExecutionSummaryWithTracedTask db(info(),_manager->sensingModule()->data());
	db.setWid(sensingWindow->wid);
	db.record();

//	db.done();
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
		totalPowerW += sense<SEN_POWER_W>(owner->info()->power_domain_list[domain_id],wid);
	}
	trace("total_power_w") = totalPowerW;

	uint64_t totalInsts = 0;
	double totalCPUTime = 0;

	for(int core = 0; core < owner->info()->core_list_size; ++core){
		totalInsts += sensedData.getPerfcntVal(sensedData.swCurrData(wid).cpus[core].perfcnt,PERFCNT_INSTR_EXE);
		totalCPUTime += sensedData.swCurrData(wid).cpus[core].perfcnt.time_busy_ms / 1000.0;
	}
	trace("total_cpu_time_s") = totalCPUTime;
	trace("total_instr") = totalInsts;

	for(int i = 0; i < owner->info()->freq_domain_list_size; ++i){
		freq_domain_info_t &fd = owner->info()->freq_domain_list[i];

		trace(formatstr("freq_domain%d_sensed",i)) = ((double)sensedData.swCurrData(wid).freq_domains[i].avg_freq_mhz_acc
					/ (double) sensedData.swCurrData(wid).freq_domains[i].time_ms_acc);

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
		totalPowerW += sense<SEN_POWER_W>(owner->info()->power_domain_list[domain_id],wid);
	}
	trace("total_power_w") = totalPowerW;

	uint64_t totalInsts = 0;
	double totalCPUTime = 0;

	for(int core = 0; core < owner->info()->core_list_size; ++core){
		totalInsts += sensedData.getPerfcntVal(sensedData.swCurrData(wid).cpus[core].perfcnt,PERFCNT_INSTR_EXE);
		totalCPUTime += sensedData.swCurrData(wid).cpus[core].perfcnt.time_busy_ms / 1000.0;
	}
	trace("total_cpu_time_s") = totalCPUTime;
	trace("total_instr") = totalInsts;

	for(int i = 0; i < owner->info()->freq_domain_list_size; ++i){
		trace(formatstr("freq_domain%d_sensed",i)) = ((double)sensedData.swCurrData(wid).freq_domains[i].avg_freq_mhz_acc
				/ (double) sensedData.swCurrData(wid).freq_domains[i].time_ms_acc);
	}
}

void InterfaceTest::report()
{
	ExecutionSummary db(info(),_manager->sensingModule()->data());
	db.setWid(sensingWindow_fine->wid);
	db.record();
}

