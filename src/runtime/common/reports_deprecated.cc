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

#include <cmath>
#include <ostream>
#include <fstream>

#include <base/base.h>

#include <runtime/common/option_parser.h>
#include <runtime/common/reports_deprecated.h>
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
	const PerformanceData &data = SensingModule::get().data();
	for(int i = 0; i < data.numMappedPerfcnts(); ++i) {
		data_names.push_back(perfcnt_str(data.perfcntFromIdx(i)));
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
SensingDataTracer::traced_data::traced_data(const SensingDataTracer &tracer,const core_info_t &sd,int wid, bool isAgg,std::initializer_list<double> &a_args)
	:_tracer(tracer)
{
	data_vals.push_back(_total_time_s(sd,wid,isAgg));
	data_vals.push_back(_busy_time_s(sd,wid,isAgg));
	data_vals.push_back(_total_ips(sd,wid,isAgg));
	data_vals.push_back(_busy_ips(sd,wid,isAgg));
	data_vals.push_back(_util(sd,wid,isAgg));
	data_vals.push_back(traced_data::NO_DATA);//power
	data_vals.push_back(traced_data::NO_DATA);//freq
	const PerformanceData &data = SensingModule::get().data();
	for(int j = 0; j < data.numMappedPerfcnts(); ++j)
		data_vals.push_back(_perfcnt(sd,j,wid,isAgg));
	data_vals.push_back(_nivcsw(sd,wid,isAgg));
	data_vals.push_back(_nvcsw(sd,wid,isAgg));
	for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) data_vals.push_back(_beats(sd,j,wid,isAgg));
	for (auto i: a_args) data_vals.push_back(i);//additional data
	assert_false(data_vals.size()!=tracer.data_names.size());
}
SensingDataTracer::traced_data::traced_data(const SensingDataTracer &tracer,const tracked_task_data_t &sd,int wid, bool isAgg,std::initializer_list<double> &a_args)
	:_tracer(tracer)
{
    data_vals.push_back(_total_time_s(sd,wid,isAgg));
    data_vals.push_back(_busy_time_s(sd,wid,isAgg));
    data_vals.push_back(_total_ips(sd,wid,isAgg));
    data_vals.push_back(_busy_ips(sd,wid,isAgg));
    data_vals.push_back(_util(sd,wid,isAgg));
    data_vals.push_back(traced_data::NO_DATA);//power
    data_vals.push_back(traced_data::NO_DATA);//freq
    const PerformanceData &data = SensingModule::get().data();
    for(int j = 0; j < data.numMappedPerfcnts(); ++j)
        data_vals.push_back(_perfcnt(sd,j,wid,isAgg));
    data_vals.push_back(_nivcsw(sd,wid,isAgg));
    data_vals.push_back(_nvcsw(sd,wid,isAgg));
    for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) data_vals.push_back(_beats(sd,j,wid,isAgg));
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
	const PerformanceData &data = SensingModule::get().data();
	for(int j = 0; j < data.numMappedPerfcnts(); ++j)
		data_vals.push_back(traced_data::NO_DATA);
	data_vals.push_back(traced_data::NO_DATA);//nivcsw
	data_vals.push_back(traced_data::NO_DATA);//nvcsw
	for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) data_vals.push_back(traced_data::NO_DATA);//beats
	for (auto i: a_args) data_vals.push_back(i);//additional data
	assert_false(data_vals.size()!=tracer.data_names.size());
}
SensingDataTracer::traced_data::traced_data(const SensingDataTracer &tracer,const freq_domain_info_t &sd,int wid, bool isAgg,std::initializer_list<double> &a_args)
	:_tracer(tracer)
{
	data_vals.push_back(traced_data::NO_DATA);//total_time
	data_vals.push_back(traced_data::NO_DATA);//busy_time
	data_vals.push_back(traced_data::NO_DATA);//ips
	data_vals.push_back(traced_data::NO_DATA);//busy_ips
	data_vals.push_back(traced_data::NO_DATA);//util
	data_vals.push_back(traced_data::NO_DATA);//power
	data_vals.push_back(_freq_mhz(sd,wid,isAgg));//freq
	const PerformanceData &data = SensingModule::get().data();
	for(int j = 0; j < data.numMappedPerfcnts(); ++j)
		data_vals.push_back(traced_data::NO_DATA);
	data_vals.push_back(traced_data::NO_DATA);//nivcsw
	data_vals.push_back(traced_data::NO_DATA);//nvcsw
	for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) data_vals.push_back(traced_data::NO_DATA);//beats
	for (auto i: a_args) data_vals.push_back(i);//additional data
	assert_false(data_vals.size()!=tracer.data_names.size());
}
//adds the core freq domain and power domain info to task
SensingDataTracer::traced_data::traced_data(const SensingDataTracer &tracer,const tracked_task_data_t &sd,const freq_domain_info_t &sd_freq,const power_domain_info_t &sd_power, int wid, bool isAgg, std::initializer_list<double> &a_args)
	:_tracer(tracer)
{
    data_vals.push_back(_total_time_s(sd,wid,isAgg));
    data_vals.push_back(_busy_time_s(sd,wid,isAgg));
    data_vals.push_back(_total_ips(sd,wid,isAgg));
    data_vals.push_back(_busy_ips(sd,wid,isAgg));
    data_vals.push_back(_util(sd,wid,isAgg));
	data_vals.push_back(_power_w(sd_power,wid,isAgg));
	data_vals.push_back(_freq_mhz(sd_freq,wid,isAgg));
	const PerformanceData &data = SensingModule::get().data();
	for(int j = 0; j < data.numMappedPerfcnts(); ++j)
	    data_vals.push_back(_perfcnt(sd,j,wid,isAgg));
	data_vals.push_back(_nivcsw(sd,wid,isAgg));
	data_vals.push_back(_nvcsw(sd,wid,isAgg));
	for(int j = 0; j < MAX_BEAT_DOMAINS; ++j) data_vals.push_back(_beats(sd,j,wid,isAgg));
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

SensingDataTracer::SensingDataTracer(sys_info_t *sys)
	:_sys(sys),_time_series_size(0),_wid(-1),_doneCalled(false)
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

	const PerformanceData &data = SensingModule::get().data();

	assert_false(_wid < 0);
	_timestamps.push_back((double)(data.sensingStopTimeMS() - data.sensingStartTimeMS())/1000.0);

	for(int cpu = 0; cpu < _sys->core_list_size; ++cpu){
		_d_cpu[cpu].push_back(new traced_data(*this,_sys->core_list[cpu],_wid,true,a_args));
	}
	for(int power_domain = 0; power_domain < _sys->power_domain_list_size; ++power_domain){
		_d_pd[power_domain].push_back(new traced_data(*this,_sys->power_domain_list[power_domain],_wid,true,a_args));
	}
	for(int freq_domain = 0; freq_domain < _sys->freq_domain_list_size; ++freq_domain){
		_d_fd[freq_domain].push_back(new traced_data(*this,_sys->freq_domain_list[freq_domain],_wid,true,a_args));
	}
	for(int p = 0; p < data.numCreatedTasks(); ++p){
		_d_task.push_back(timeseries_data());
		_d_task[p].push_back(new traced_data(*this,data.task(p),_wid,true,a_args));
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
	const PerformanceData &data = SensingModule::get().data();

	for(int p = 0; p < data.numCreatedTasks(); ++p)
		_dumpTotalPrintLine(of,"",data.task(p),_d_task,p);

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

	const PerformanceData &data = SensingModule::get().data();

	assert_false(_wid < 0);
	_timestamps.push_back((double)(data.currWindowTimeMS(_wid) - data.sensingStartTimeMS())/1000.0);

	//in the time trace we copy the core's domain data to its own data to make it easier to analyse later
	for(int cpu = 0; cpu < _sys->core_list_size; ++cpu){
		//int power_domain = _sys->core_list[cpu].power->domain_id;
		//int freq_domain = _sys->core_list[cpu].freq->domain_id;
		//_d_cpu[cpu].push_back(new traced_data(_data,sw.cpus[cpu],sw.power_domains[power_domain],sw.freq_domains[freq_domain]));
		_d_cpu[cpu].push_back(new traced_data(*this,_sys->core_list[cpu],_wid,false,a_args));
	}
	for(int power_domain = 0; power_domain < _sys->power_domain_list_size; ++power_domain){
		_d_pd[power_domain].push_back(new traced_data(*this,_sys->power_domain_list[power_domain],_wid,false,a_args));
	}
	for(int freq_domain = 0; freq_domain < _sys->freq_domain_list_size; ++freq_domain){
		_d_fd[freq_domain].push_back(new traced_data(*this,_sys->freq_domain_list[freq_domain],_wid,false,a_args));
	}
	for(int p = 0; p < data.numCreatedTasks(); ++p){
		//has the task executed in this epoch ?
		const tracked_task_data_t &task = data.task(p);
		int last_cpu_used = SensingInterface::sense<SEN_LASTCPU>(&task,_wid);

		if(last_cpu_used == -1) continue; //task have not executed yet
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
			int fd = _sys->core_list[last_cpu_used].freq->domain_id;
			int pd = _sys->core_list[last_cpu_used].power->domain_id;
			traced_data *aux = new traced_data(*this,task,_sys->freq_domain_list[fd],_sys->power_domain_list[pd],_wid,false,a_args);
			_d_task[idx].push_back(aux);
		}
	}

	++_time_series_size;
}

void TimeTracer::dump()
{
	//one file per component

    const PerformanceData &data = SensingModule::get().data();

    assert_false(_timestamps.size() != _time_series_size);

	pinfo("TimeTracer - dumping to %s\n", Options::get<OPT_OUTDIR>().c_str());

	_dumpComponentTimeSeries(*_sys,_d_sys[0]);

	for(int freq_domain = 0; freq_domain < _sys->freq_domain_list_size; ++freq_domain)
		_dumpComponentTimeSeries(_sys->freq_domain_list[freq_domain],_d_fd[freq_domain]);

	for(int power_domain = 0; power_domain < _sys->power_domain_list_size; ++power_domain)
		_dumpComponentTimeSeries(_sys->power_domain_list[power_domain],_d_pd[power_domain]);

	for(int cpu = 0; cpu < _sys->core_list_size; ++cpu)
		_dumpComponentTimeSeries(_sys->core_list[cpu],_d_cpu[cpu]);

	for(int p = 0; p < data.numCreatedTasks(); ++p){
		if(_task_id2idx.find(data.task(p).this_task_pid)==_task_id2idx.end()){
			//pinfo("No data for dumping trace of task %d\n",_data.created_tasks[p].this_task_pid);
			continue;
		}
		_dumpComponentTimeSeries(data.task(p),_d_task[_task_id2idx[data.task(p).this_task_pid]]);
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

