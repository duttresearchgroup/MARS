#include <cmath>
#include <ostream>
#include <fstream>

#include <runtime/interfaces/common/sense_defs.h>
#include <core/base/base.h>

#include <runtime/common/rt_config_params.h>
#include <runtime/common/reports.h>
#include <runtime/interfaces/sensing_module.h>
#include <sstream>

#define IDENT_l0 "  "
#define IDENT_l1 "    "

/*
 * The counters are:
 * 		-total_time
 * 		-busy_time
 * 		-ips
 * 		-busy_ips
 * 		-util
 * 		-power
 * 		-freq
 * 		-perf_cnt0
 * 		-perf_cnt1
 * 		...
 * 		-perf_cntn
 * 		-nivcsw
 * 		-nvcsw
 * 		-beats
 * 		...
 */
const double SensingDataTracer::traced_data::NO_DATA = -1;
void SensingDataTracer::init_counters_names(std::initializer_list<std::string> a_args)
{
	assert_false(data_names.size() != 0);
	assert_false(data_agg_att.size() != 0);
	data_names.push_back("total_time_s"); data_agg_att.push_back(traced_data::AGG_MAX);
	data_names.push_back("busy_time_s"); data_agg_att.push_back(traced_data::AGG_SUM);
	data_names.push_back("total_ips"); data_agg_att.push_back(traced_data::AGG_SUM);
	data_names.push_back("busy_ips"); data_agg_att.push_back(traced_data::AGG_SUM);
	data_names.push_back("util"); data_agg_att.push_back(traced_data::AGG_SUM);
	data_names.push_back("power_w"); data_agg_att.push_back(traced_data::AGG_SUM);
	data_names.push_back("freq_mhz"); data_agg_att.push_back(traced_data::AGG_NOPE);
	for(int i = 0; i < _data.numMappedPerfcnts(); ++i) {
		data_names.push_back(perfcnt_str(_data.perfcntFromIdx(i)));
		data_agg_att.push_back(traced_data::AGG_SUM);
	}
	data_names.push_back("nivcsw"); data_agg_att.push_back(traced_data::AGG_SUM);
	data_names.push_back("nvcsw"); data_agg_att.push_back(traced_data::AGG_SUM);
	for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) {
		data_names.push_back("beats"+std::to_string(j)); data_agg_att.push_back(traced_data::AGG_SUM);
	}
	for (auto i: a_args) {
		data_names.push_back(i); data_agg_att.push_back(traced_data::AGG_NOPE);
	}
}
SensingDataTracer::traced_data::traced_data(const SensingDataTracer &tracer,const perf_data_cpu_t &sd,std::initializer_list<double> &a_args)
	:_tracer(tracer)
{
	data_vals.push_back(_total_time_s(sd.perfcnt));
	data_vals.push_back(_busy_time_s(sd.perfcnt));
	data_vals.push_back(_total_ips(tracer._data,sd.perfcnt));
	data_vals.push_back(_busy_ips(tracer._data,sd.perfcnt));
	data_vals.push_back(_util(sd.perfcnt));
	data_vals.push_back(traced_data::NO_DATA);//power
	data_vals.push_back(traced_data::NO_DATA);//freq
	for(int j = 0; j < tracer._data.numMappedPerfcnts(); ++j)
		data_vals.push_back(sd.perfcnt.perfcnts[j]);
	data_vals.push_back(sd.perfcnt.nivcsw);
	data_vals.push_back(sd.perfcnt.nvcsw);
	for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) data_vals.push_back(sd.beats[j]);
	for (auto i: a_args) data_vals.push_back(i);//additional data
	assert_false(data_vals.size()!=tracer.data_names.size());
}
SensingDataTracer::traced_data::traced_data(const SensingDataTracer &tracer,const perf_data_task_t &sd,std::initializer_list<double> &a_args)
	:_tracer(tracer)
{
	data_vals.push_back(_total_time_s(sd.perfcnt));
	data_vals.push_back(_busy_time_s(sd.perfcnt));
	data_vals.push_back(_total_ips(tracer._data,sd.perfcnt));
	data_vals.push_back(_busy_ips(tracer._data,sd.perfcnt));
	data_vals.push_back(_util(sd.perfcnt));
	data_vals.push_back(traced_data::NO_DATA);//power
	data_vals.push_back(traced_data::NO_DATA);//freq
	for(int j = 0; j < tracer._data.numMappedPerfcnts(); ++j)
		data_vals.push_back(sd.perfcnt.perfcnts[j]);
	data_vals.push_back(sd.perfcnt.nivcsw);
	data_vals.push_back(sd.perfcnt.nvcsw);
	for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) data_vals.push_back(sd.beats[j]);//beats
	for (auto i: a_args) data_vals.push_back(i);//additional data
	assert_false(data_vals.size()!=tracer.data_names.size());
}
SensingDataTracer::traced_data::traced_data(const SensingDataTracer &tracer,const power_domain_info_t &sd, int wid, bool isAgg, std::initializer_list<double> &a_args)
	:_tracer(tracer)
{
	data_vals.push_back(traced_data::NO_DATA);//total_time
	data_vals.push_back(traced_data::NO_DATA);//busy_time
	data_vals.push_back(traced_data::NO_DATA);//ips
	data_vals.push_back(traced_data::NO_DATA);//busy_ips
	data_vals.push_back(traced_data::NO_DATA);//util
	data_vals.push_back(_power_w(sd,wid,isAgg));//power
	data_vals.push_back(traced_data::NO_DATA);//freq
	for(int j = 0; j < tracer._data.numMappedPerfcnts(); ++j)
		data_vals.push_back(traced_data::NO_DATA);
	data_vals.push_back(traced_data::NO_DATA);//nivcsw
	data_vals.push_back(traced_data::NO_DATA);//nvcsw
	for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) data_vals.push_back(traced_data::NO_DATA);//beats
	for (auto i: a_args) data_vals.push_back(i);//additional data
	assert_false(data_vals.size()!=tracer.data_names.size());
}
SensingDataTracer::traced_data::traced_data(const SensingDataTracer &tracer,const perf_data_freq_domain_t &sd,std::initializer_list<double> &a_args)
	:_tracer(tracer)
{
	data_vals.push_back(traced_data::NO_DATA);//total_time
	data_vals.push_back(traced_data::NO_DATA);//busy_time
	data_vals.push_back(traced_data::NO_DATA);//ips
	data_vals.push_back(traced_data::NO_DATA);//busy_ips
	data_vals.push_back(traced_data::NO_DATA);//util
	data_vals.push_back(traced_data::NO_DATA);//power
	data_vals.push_back(_freq_mhz(sd));//freq
	for(int j = 0; j < tracer._data.numMappedPerfcnts(); ++j)
		data_vals.push_back(traced_data::NO_DATA);
	data_vals.push_back(traced_data::NO_DATA);//nivcsw
	data_vals.push_back(traced_data::NO_DATA);//nvcsw
	for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) data_vals.push_back(traced_data::NO_DATA);//beats
	for (auto i: a_args) data_vals.push_back(i);//additional data
	assert_false(data_vals.size()!=tracer.data_names.size());
}
//adds the core freq domain and power domain info to task
SensingDataTracer::traced_data::traced_data(const SensingDataTracer &tracer,const perf_data_task_t &sd,const perf_data_freq_domain_t &sd_freq,const power_domain_info_t &sd_power, int wid, bool isAgg, std::initializer_list<double> &a_args)
	:_tracer(tracer)
{
	data_vals.push_back(_total_time_s(sd.perfcnt));
	data_vals.push_back(_busy_time_s(sd.perfcnt));
	data_vals.push_back(_total_ips(tracer._data,sd.perfcnt));
	data_vals.push_back(_busy_ips(tracer._data,sd.perfcnt));
	data_vals.push_back(_util(sd.perfcnt));
	data_vals.push_back(_power_w(sd_power,wid,isAgg));
	data_vals.push_back(_freq_mhz(sd_freq));
	for(int j = 0; j < tracer._data.numMappedPerfcnts(); ++j)
		data_vals.push_back(sd.perfcnt.perfcnts[j]);
	data_vals.push_back(sd.perfcnt.nivcsw);
	data_vals.push_back(sd.perfcnt.nvcsw);
	for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) data_vals.push_back(sd.beats[j]);//beats
	for (auto i: a_args) data_vals.push_back(i);//additional data
	assert_false(data_vals.size()!=tracer.data_names.size());
}
SensingDataTracer::traced_data::traced_data(const SensingDataTracer &tracer)
	:_tracer(tracer)
{
	for(unsigned i = 0; i < tracer.data_names.size();++i) data_vals.push_back(traced_data::NO_DATA);
}


/*SensingDataTracer::traced_data::traced_data(const SensedData &data,
		const sensed_data_perf_counters_t &sd_core,
		const sensed_data_power_domain_t &sd_pow,const sensed_data_freq_domain_t &sd_freq)
{
	traced_data::init_counters_names(data);
	data_vals.push_back(_total_time_s(sd_core));
	data_vals.push_back(_busy_time_s(sd_core));
	data_vals.push_back(_total_ips(data,sd_core));
	data_vals.push_back(_busy_ips(data,sd_core));
	data_vals.push_back(_util(sd_core));
	data_vals.push_back(_power_w(sd_pow));//power
	data_vals.push_back(_freq_mhz(sd_freq));//freq
	for(int j = 0; j < data.numMappedPerfcnts(); ++j)
		data_vals.push_back(sd_core.perfcnts[j]);
	data_vals.push_back(sd_core.nivcsw);
	data_vals.push_back(sd_core.nvcsw);
	for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) data_vals.push_back(traced_data::NO_DATA);//beats
	assert_false(data_vals.size()!=data_names.size());
}*/

void SensingDataTracer::traced_data::merge(traced_data &data, bool overwrite)
{
	assert_false(data_vals.size()!=data._tracer.data_names.size());
	assert_false(data_vals.size()!=data.data_vals.size());
	if(overwrite)
		for(unsigned i = 0; i < data_vals.size(); ++i) data_vals[i] = data.data_vals[i];
	else
		for(unsigned i = 0; i < data_vals.size(); ++i) data_vals[i] = _merge(data.data_vals,i);
}

void SensingDataTracer::_init_counters(database_type &data,int num_of_components)
{
	for(int i = 0; i < num_of_components; ++i){
		data.push_back(timeseries_data());
	}
}

SensingDataTracer::SensingDataTracer(sys_info_t *sys,const PerformanceData &data)
	:_sys(sys),_data(data),_time_series_size(0),_wid(-1),_doneCalled(false)
{
	_init_counters(_d_sys,1);
	_init_counters(_d_cpu,_sys->core_list_size);
	_init_counters(_d_fd,_sys->freq_domain_list_size);
	_init_counters(_d_pd,_sys->power_domain_list_size);
	_init_counters(_d_task);
}


SensingDataTracer::~SensingDataTracer()
{
	for(unsigned i=0; i < _d_sys.size(); ++i) for(auto d : _d_sys[i]) delete d;
	for(unsigned i=0; i < _d_cpu.size(); ++i) for(auto d : _d_cpu[i]) delete d;
	for(unsigned i=0; i < _d_fd.size(); ++i) for(auto d : _d_fd[i]) delete d;
	for(unsigned i=0; i < _d_pd.size(); ++i) for(auto d : _d_pd[i]) delete d;
	for(unsigned i=0; i < _d_task.size(); ++i) for(auto d : _d_task[i]) delete d;
}

/*
 * Takes all the data from all components in src, aggregate, and then add to tgt
 * if create_new==false reuses the last data on tgt
 */
void SensingDataTracer::_aggregate(database_type &src, timeseries_data &tgt)
{
	//src must have at least one cmp
	assert_false(src.size()==0);
	for(unsigned comp = 0; comp < src.size(); ++comp) _aggregate_one(src[comp],tgt);

}

void SensingDataTracer::_aggregate_one(timeseries_data &src, timeseries_data &tgt)
{
	assert_false(src.size()!=_time_series_size);
	if(tgt.size() == 0){
		for(unsigned sample = 0; sample < _time_series_size; ++sample) tgt.push_back(new traced_data(*this));
	}
	assert_false(tgt.size()!=_time_series_size);
	for(unsigned sample = 0; sample < _time_series_size; ++sample) tgt[sample]->merge(*(src[sample]));
}

void SensingDataTracer::wrapUp()
{
	//aggregate the core data stuff into the power domain
	for(int power_domain = 0; power_domain < _sys->power_domain_list_size; ++power_domain){
		core_info_t *core;
		for_each_in_internal_list(&(_sys->power_domain_list[power_domain]),cores,core,power_domain){
			_aggregate_one(_d_cpu[core->position],_d_pd[power_domain]);
		}
	}
	//aggregate power stuf into freq domain
	for(int freq_domain = 0; freq_domain < _sys->freq_domain_list_size; ++freq_domain){
		power_domain_info_t *pd;
		for_each_in_internal_list(&(_sys->freq_domain_list[freq_domain]),power_domains,pd,freq_domain){
			_aggregate_one(_d_pd[pd->domain_id],_d_fd[freq_domain]);
		}
	}

	//sys-wide stuff by aggregating data from allpower domains (for power data)
	for(auto ts : _d_sys[0]) delete ts;//cleanup first
	_d_sys[0].clear();
	_aggregate(_d_fd,_d_sys[0]);
}

void ExecutionSummary::record(std::initializer_list<double> a_args)
{
	if(data_names.size() == 0)
		init_counters_names();

	assert_false(_wid < 0);
	_timestamps.push_back((double)(_data.sensingStopTimeMS() - _data.sensingStartTimeMS())/1000.0);

	const perf_window_data_t& sw = _data.swAggrData(_wid);

	for(int cpu = 0; cpu < _sys->core_list_size; ++cpu){
		_d_cpu[cpu].push_back(new traced_data(*this,sw.cpus[cpu],a_args));
	}
	for(int power_domain = 0; power_domain < _sys->power_domain_list_size; ++power_domain){
		_d_pd[power_domain].push_back(new traced_data(*this,_sys->power_domain_list[power_domain],_wid,true,a_args));
	}
	for(int freq_domain = 0; freq_domain < _sys->freq_domain_list_size; ++freq_domain){
		_d_fd[freq_domain].push_back(new traced_data(*this,sw.freq_domains[freq_domain],a_args));
	}
	for(int p = 0; p < _data.numCreatedTasks(); ++p){
		_d_task.push_back(timeseries_data());
		_d_task[p].push_back(new traced_data(*this,sw.tasks[p],a_args));
	}
	++_time_series_size;
}

void ExecutionSummary::dump()
{
	//this is for dumping the final summary so we must have only one sample
	assert_false(_timestamps.size() != 1);

	//counters in columns, components/tasks in rows

	std::string path = _pathNameTotal();

	pinfo("ExecutionSummary - dumping to %s\n",path.c_str());

	std::ofstream of(path);
	of.precision(17);

	of << "component";//first column is for sys components/tasks
	_appendDataHeader(of);
	of << "\n";

	//dump according to hierarchy
	_dumpTotalPrintLine(of,"",*_sys,_d_sys,0);
	for(int freq_domain = 0; freq_domain < _sys->freq_domain_list_size; ++freq_domain){
		_dumpTotalPrintLine(of,"  ",_sys->freq_domain_list[freq_domain],_d_fd,freq_domain);
		for(int power_domain = 0; power_domain < _sys->power_domain_list_size; ++power_domain){
			if(freq_domain != _sys->power_domain_list[power_domain].freq_domain->domain_id)
				continue;
			_dumpTotalPrintLine(of,"    ",_sys->power_domain_list[power_domain],_d_pd,power_domain);
			for(int cpu = 0; cpu < _sys->core_list_size; ++cpu){
				if(power_domain != _sys->core_list[cpu].power->domain_id)
					continue;
				_dumpTotalPrintLine(of,"      ",_sys->core_list[cpu],_d_cpu,cpu);
			}
		}
	}

	for(int p = 0; p < _data.numCreatedTasks(); ++p)
		_dumpTotalPrintLine(of,"",_data.task(p),_d_task,p);

	of.close();
}

void ExecutionSummary::showReport()
{
	pinfo("execution report\n");
	for(int cpu = 0; cpu < _sys->core_list_size; ++cpu){
		assert_false(_d_cpu[cpu].size() != 1);
		pinfo(IDENT_l0"Core %d (fd%d/pd%d) util: %f \n",
			cpu,_sys->core_list[cpu].freq->domain_id,_sys->core_list[cpu].power->domain_id,_d_cpu[cpu][0]->data_vals[D_IDX_UTIL]);
	}
	for(int power_domain = 0; power_domain < _sys->power_domain_list_size; ++power_domain){
		assert_false(_d_pd[power_domain].size() != 1);
		pinfo(IDENT_l0"Power domain %d power: %fW\n",power_domain,_d_pd[power_domain][0]->data_vals[D_IDX_POWER]);
	}
	for(int freq_domain = 0; freq_domain < _sys->freq_domain_list_size; ++freq_domain){
		assert_false(_d_fd[freq_domain].size() != 1);
		pinfo(IDENT_l0"Freq domain %d freq: %fMHz\n",freq_domain,_d_fd[freq_domain][0]->data_vals[D_IDX_FREQ]);
	}
	assert_false(_d_sys[0].size() != 1);
	pinfo(IDENT_l0"Total sys time: %fs power: %fW \n",_d_sys[0][0]->data_vals[D_IDX_TOTALT],_d_sys[0][0]->data_vals[D_IDX_POWER]);
}

void TimeTracer::record(std::initializer_list<double> a_args)
{
	if(data_names.size() == 0)
		init_counters_names();

	assert_false(_wid < 0);
	_timestamps.push_back((double)(_data.swCurrSampleTimeMS(_wid) - _data.sensingStartTimeMS())/1000.0);

	const perf_window_data_t& sw = _data.swCurrData(_wid);

	//in the time trace we copy the core's domain data to its own data to make it easier to analyse later
	for(int cpu = 0; cpu < _sys->core_list_size; ++cpu){
		//int power_domain = _sys->core_list[cpu].power->domain_id;
		//int freq_domain = _sys->core_list[cpu].freq->domain_id;
		//_d_cpu[cpu].push_back(new traced_data(_data,sw.cpus[cpu],sw.power_domains[power_domain],sw.freq_domains[freq_domain]));
		_d_cpu[cpu].push_back(new traced_data(*this,sw.cpus[cpu],a_args));
	}
	for(int power_domain = 0; power_domain < _sys->power_domain_list_size; ++power_domain){
		_d_pd[power_domain].push_back(new traced_data(*this,_sys->power_domain_list[power_domain],_wid,false,a_args));
	}
	for(int freq_domain = 0; freq_domain < _sys->freq_domain_list_size; ++freq_domain){
		_d_fd[freq_domain].push_back(new traced_data(*this,sw.freq_domains[freq_domain],a_args));
	}
	for(int p = 0; p < _data.numCreatedTasks(); ++p){
		//has the task executed in this epoch ?
		const tracked_task_data_t &task = _data.task(p);
		if(sw.tasks[p].last_cpu_used == -1) continue; //task have not executed yet
		{
			//check we have data for this task
			if(_task_id2idx.find(task.this_task_pid)==_task_id2idx.end()){
				_task_id2idx[task.this_task_pid] = _d_task.size();
				_task_idx2id[_d_task.size()] = task.this_task_pid;
				_d_task.push_back(timeseries_data());
			}
			int idx = _task_id2idx[task.this_task_pid];

			//add empty data for all the epochs this one didn't have any data
			while(_d_task[idx].size() < _time_series_size) _d_task[idx].push_back(new traced_data(*this));
			assert_false(_d_task[idx].size() > _time_series_size);

			//data for this epoch
			int fd = _sys->core_list[sw.tasks[p].last_cpu_used].freq->domain_id;
			int pd = _sys->core_list[sw.tasks[p].last_cpu_used].power->domain_id;
			traced_data *aux = new traced_data(*this,sw.tasks[p],sw.freq_domains[fd],_sys->power_domain_list[pd],_wid,false,a_args);
			_d_task[idx].push_back(aux);
		}
	}

	++_time_series_size;
}

void TimeTracer::dump()
{
	//one file per component

	assert_false(_timestamps.size() != _time_series_size);

	pinfo("TimeTracer - dumping to %s\n", rt_param_outdir().c_str());

	_dumpComponentTimeSeries(*_sys,_d_sys[0]);

	for(int freq_domain = 0; freq_domain < _sys->freq_domain_list_size; ++freq_domain)
		_dumpComponentTimeSeries(_sys->freq_domain_list[freq_domain],_d_fd[freq_domain]);

	for(int power_domain = 0; power_domain < _sys->power_domain_list_size; ++power_domain)
		_dumpComponentTimeSeries(_sys->power_domain_list[power_domain],_d_pd[power_domain]);

	for(int cpu = 0; cpu < _sys->core_list_size; ++cpu)
		_dumpComponentTimeSeries(_sys->core_list[cpu],_d_cpu[cpu]);

	for(int p = 0; p < _data.numCreatedTasks(); ++p){
		if(_task_id2idx.find(_data.task(p).this_task_pid)==_task_id2idx.end()){
			//pinfo("No data for dumping trace of task %d\n",_data.created_tasks[p].this_task_pid);
			continue;
		}
		_dumpComponentTimeSeries(_data.task(p),_d_task[_task_id2idx[_data.task(p).this_task_pid]]);
	}

}

void TimeTracer::_dumpTimeSeries(std::string filename, timeseries_data &data)
{
	//counters in columns, values for each timestamp in rows
	//pinfo("Dumping %s\n",filename.c_str());

	std::ofstream of(filename);
	of.precision(17);

	of << "timestamp";
	_appendDataHeader(of);
	of << "\n";

	//dump rest
	_dumpTimeSeriesData(of,data);

	of.close();
}

void ExecutionSummaryWithTracedTask::dump()
{
	ExecutionSummary::dump();

	if(_traced_task != nullptr){
		//reappend the traced task to the output file
		std::string path = _pathNameTotal();
		std::ofstream of(path,std::ios::app);
		of.precision(17);

		for(int p = 0; p < _data.numCreatedTasks(); ++p)
			if(_data.task(p).this_task_pid == _traced_task->this_task_pid){
				//adds domain power and freq to the traced task only
				int pd = _sys->core_list[rt_param_trace_core()].power->domain_id;
				int fd = _sys->core_list[rt_param_trace_core()].freq->domain_id;
				_d_task[p][0]->data_vals[D_IDX_POWER] = _d_pd[pd][0]->data_vals[D_IDX_POWER];
				_d_task[p][0]->data_vals[D_IDX_FREQ] = _d_fd[fd][0]->data_vals[D_IDX_FREQ];

				of << "traced.";
				_dumpTotalPrintLine(of,"",_data.task(p),_d_task,p);
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

	const perf_window_data_t& sw = _data.swAggrData(_wid);

	for(int i = 0; i < _data.numCreatedTasks(); ++i){
		const tracked_task_data_t &task = _data.task(i);
		if((sw.tasks[i].last_cpu_used == rt_param_trace_core()) &&
		   (_data.getPerfcntVal(sw.tasks[i].perfcnt,PERFCNT_INSTR_EXE) > 0) &&
		   (_data.getPerfcntVal(sw.tasks[i].perfcnt,PERFCNT_BUSY_CY) > 0) &&
		   (sw.tasks[i].perfcnt.time_busy_ms > 0)){

			if(traced_task == nullptr) traced_task = &task;
			else if(_data.getPerfcntVal(sw.tasks[i].perfcnt,PERFCNT_INSTR_EXE)
			     > _data.getPerfcntVal(sw.tasks[traced_task->task_idx].perfcnt,PERFCNT_INSTR_EXE)){
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
		//sums up the total ammount of instructions executed by all cores in this core's freq/power domain
		for_each_in_internal_list(_sys->core_list[sw.tasks[traced_task->task_idx].last_cpu_used].power,cores,core,power_domain){
			instr_sum += _data.getPerfcntVal(sw.cpus[core->position].perfcnt,PERFCNT_INSTR_EXE);
		}
		double rate = _data.getPerfcntVal(sw.tasks[traced_task->task_idx].perfcnt,PERFCNT_INSTR_EXE)
		              / (double) instr_sum;
		if(rate < 0.7){
			pinfo("Task %d is the traced task, but apparently too many other tasks executed in core %d's cluster (rate = %f). Too much interference\n",traced_task->this_task_pid,rt_param_trace_core(),rate);
			pinfo("Retrying might fix this\n");
		}
		else if(sw.tasks[traced_task->task_idx].last_cpu_used != rt_param_trace_core()){
			pinfo("Task %d is the traced task, but apparently it ran on core %d. It should run in the traced core %d.",traced_task->this_task_pid,sw.tasks[traced_task->task_idx].last_cpu_used,rt_param_trace_core());
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

///////////////////////////////////////////////
///////////////////////////////////////////////

ExecutionTrace::ExecutionTraceHandle& ExecutionTrace::getHandle(const PerformanceData &sensedData, int wid)
{
	uint64_t timestampMS = sensedData.swCurrSampleTimeMS(wid) - sensedData.sensingStartTimeMS();

	//have we called getHandle for the same sample ?
	if(!_timestampsMS.empty() && (_timestampsMS.back()==timestampMS)){
		return _traceHandle;
	}

	_currSample += 1;
	assert_true(_timestampsMS.size() == (unsigned)_currSample);
	_timestampsMS.push_back(timestampMS);
	_traceHandle._sampleIdx = _currSample;
	return _traceHandle;
}

void ExecutionTrace::__dumpNew()
{
	//dump the header / overwrite existing file

	assert_true(_lastSampleDumped == -1);//if dumping header then samples must've been reset

	std::ofstream of(_pathName);
	of << "sample_id;timestamp";
	for(const auto &col : _data)
		of << ";" << col.first;
	of << "\n";

	_headerDumped = true;
	_headerModified = false;
}
void ExecutionTrace::_dump()
{
	//dump all undumped samples
	//cretes a new file with header if heade was changed and undumped

	if(!_headerDumped || _headerModified) __dumpNew();

	if(_lastSampleDumped == _currSample) return;

	pinfo("ExecutionTrace - dumping to %s\n", _pathName.c_str());

	std::ofstream of(_pathName,std::ios::app);
	of.precision(17);

	for(int i = _lastSampleDumped+1; i <= _currSample; ++i){
		//sample_id;timestamp
		of << i << ";" << _timestampsMS[i] / 1000.0;
		for(const auto &col : _data){
			const std::map<int,double> &entryData = col.second;
			of << ";";
			auto iter = entryData.find(i);
			if(iter != entryData.end())
				of << iter->second;
		}
		of << "\n";
	}
	_lastSampleDumped = _currSample;
}

double& ExecutionTrace::ExecutionTraceHandle::operator()(const std::string &entry)
{
	//TODO this functions does the same search multiple times
	//OPTIMIZE!
	if(_trace._data.find(entry)==_trace._data.end()){
		_trace._data[entry] = std::map<int,double>();
		_trace._headerModified = true;
	}
	std::map<int,double> &entryData = _trace._data[entry];
	entryData[_sampleIdx] = 0;
	return entryData[_sampleIdx];
}


