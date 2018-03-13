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

#ifndef __arm_rt_reports_deprecated_h
#define __arm_rt_reports_deprecated_h

#include <vector>
#include <map>
#include <ostream>
#include <fstream>
#include <sstream>

#include <runtime/common/rt_config_params.h>
#include <runtime/framework/sensing_interface.h>
#include <runtime/interfaces/performance_data.h>
#include <runtime/interfaces/sensing_module.h>

inline std::ostream& operator<< (std::ostream& os, const sys_info_t& obj){
	os << "sys"; return os;
}
inline std::ostream& operator<< (std::ostream& os, const freq_domain_info_t& obj){
	os << "fd" << obj.domain_id; return os;
}
inline std::ostream& operator<< (std::ostream& os, const power_domain_info_t& obj){
	os << "pd" << obj.domain_id; return os;
}
inline std::ostream& operator<< (std::ostream& os, const core_info_t& obj){
	os << "core" << obj.position; return os;
}
inline std::ostream& operator<< (std::ostream& os, const tracked_task_data_t& obj){
	os << "pid" << obj.this_task_pid << "." << obj.this_task_name; return os;
}

class SensingDataTracer {

protected:

	struct traced_data{
		const SensingDataTracer &_tracer;

		traced_data(const SensingDataTracer &tracer,const core_info_t &sd,int wid, bool isAgg,std::initializer_list<double> &a_args);
		traced_data(const SensingDataTracer &tracer,const tracked_task_data_t &sd,int wid, bool isAgg,std::initializer_list<double> &a_args);
		traced_data(const SensingDataTracer &tracer,const power_domain_info_t &sd, int wid, bool isAgg, std::initializer_list<double> &a_args);
		traced_data(const SensingDataTracer &tracer,const freq_domain_info_t &sd,int wid, bool isAgg,std::initializer_list<double> &a_args);
		traced_data(const SensingDataTracer &tracer,const tracked_task_data_t &sd,const freq_domain_info_t &sd_freq,const power_domain_info_t &sd_power, int wid, bool isAgg,std::initializer_list<double> &a_args);
		traced_data(const SensingDataTracer &tracer);

		template<typename Resource>
		inline double _total_time_s(const Resource &r, int wid, bool isAgg){
		    return isAgg ? SensingInterface::senseAgg<SEN_TOTALTIME_S>(&r,wid) : SensingInterface::sense<SEN_TOTALTIME_S>(&r,wid);
		}
		template<typename Resource>
		inline double _busy_time_s(const Resource &r, int wid, bool isAgg){
		    return isAgg ? SensingInterface::senseAgg<SEN_BUSYTIME_S>(&r,wid) : SensingInterface::sense<SEN_BUSYTIME_S>(&r,wid);
		}
		template<typename Resource>
		inline uint64_t _total_intr(const Resource &r, int wid, bool isAgg){
		    return isAgg ? SensingInterface::senseAgg<SEN_PERFCNT>(PERFCNT_INSTR_EXE,&r,wid) :  SensingInterface::sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,&r,wid);
		}
		template<typename Resource>
		inline double _total_ips(const Resource &r, int wid, bool isAgg){
			return (double) _total_intr(r,wid,isAgg) / _total_time_s(r,wid,isAgg);
		}
		template<typename Resource>
		inline double _busy_ips(const Resource &r, int wid, bool isAgg){
		    return (double) _total_intr(r,wid,isAgg) / _busy_time_s(r,wid,isAgg);
		}
		template<typename Resource>
		inline double _util(const Resource &r, int wid, bool isAgg){
			return _busy_time_s(r,wid,isAgg) / _total_time_s(r,wid,isAgg);
		}
		template<typename Resource>
		inline double _power_w(const Resource &r, int wid, bool isAgg){
			return isAgg ? SensingInterface::senseAgg<SEN_POWER_W>(&r,wid) : SensingInterface::sense<SEN_POWER_W>(&r,wid);
		}
		template<typename Resource>
		inline double _freq_mhz(const Resource &r, int wid, bool isAgg){
		    return isAgg ? SensingInterface::senseAgg<SEN_FREQ_MHZ>(&r,wid) : SensingInterface::sense<SEN_FREQ_MHZ>(&r,wid);
		}
		template<typename Resource>
		inline double _nivcsw(const Resource &r, int wid, bool isAgg){
		    return isAgg ? SensingInterface::senseAgg<SEN_NIVCSW>(&r,wid) : SensingInterface::sense<SEN_NIVCSW>(&r,wid);
		}
		template<typename Resource>
		inline double _nvcsw(const Resource &r, int wid, bool isAgg){
		    return isAgg ? SensingInterface::senseAgg<SEN_NVCSW>(&r,wid) : SensingInterface::sense<SEN_NVCSW>(&r,wid);
		}
		template<typename Resource>
		inline double _beats(const Resource &r, int domain, int wid, bool isAgg){
		    return isAgg ? SensingInterface::senseAgg<SEN_BEATS>(domain,&r,wid) : SensingInterface::sense<SEN_BEATS>(domain,&r,wid);
		}
		template<typename Resource>
		inline double _perfcnt(const Resource &r, int pindex, int wid, bool isAgg){
		    const PerformanceData &data = SensingModule::get().data();
		    perfcnt_t cnt = data.perfcntFromIdx(pindex);
		    return isAgg ? SensingInterface::senseAgg<SEN_PERFCNT>(cnt,&r,wid) : SensingInterface::sense<SEN_PERFCNT>(cnt,&r,wid);
		}

		typedef enum {
			AGG_SUM,
			AGG_MAX,
			AGG_NOPE,//when doesn't make sense to aggregate
		} agg_type;

		//merge the given data into this one. Data os combined according to the counter agg_att
		void merge(traced_data &data, bool overwrite=false);

		//-1 value for spots with no data
		static const double NO_DATA;
		std::vector<double> data_vals;

		inline double _merge(std::vector<double> &other_data, int data_idx){
			if(data_vals[data_idx]==NO_DATA) return other_data[data_idx];
			if(other_data[data_idx]==NO_DATA) return data_vals[data_idx];
			switch (_tracer.data_agg_att[data_idx]) {
				case AGG_SUM: return data_vals[data_idx] + other_data[data_idx];
				case AGG_MAX: return std::max(data_vals[data_idx],other_data[data_idx]);
				case AGG_NOPE: return data_vals[data_idx];
				default: return NO_DATA;
			}
			return NO_DATA;
		}
	};

	sys_info_t *_sys;

	//number of samples stored
	unsigned _time_series_size;

	//sensing window to record data from
	int _wid;

	//was this object data dumped ?
	bool _doneCalled;


	std::vector<std::string> data_names;
	std::vector<traced_data::agg_type> data_agg_att;

	/*
	 * 1) Component ID (static, dynamic for tasks)
	 * 3) Time series ()
	 */
	typedef std::vector<traced_data*> timeseries_data;
	typedef std::vector<timeseries_data> database_type; //the size of all timeseries_data vectors should always be _time_series_size
	database_type _d_sys;//this only holds aggregated data updated at the end by update sys
	database_type _d_cpu;
	database_type _d_pd;
	database_type _d_fd;
	database_type _d_task;
	std::vector<double> _timestamps;

	void _init_counters(database_type &data,int num_of_components=0);

	void _aggregate_one(timeseries_data &src, timeseries_data &tgt);
	void _aggregate(database_type &src, timeseries_data &tgt);

	inline void _appendData(std::ofstream &of, std::vector<double> &data_vals){
		for(unsigned c = 0; c < data_names.size(); ++c){
			of << ";";
			if(data_vals[c]!=traced_data::NO_DATA) of << data_vals[c];
		}
	}
	inline void _appendDataHeader(std::ofstream &of){
		for(unsigned c = 0; c < data_names.size(); ++c){
			of << ";" << data_names[c];
		}
	}

public:
	SensingDataTracer(sys_info_t *sys);
	virtual ~SensingDataTracer();

	void init_counters_names(std::initializer_list<std::string> a_args={});

protected:
	//dump all traced data to files
	virtual void dump() = 0;
	//show a report in the console
	virtual void showReport() = 0;

	//should be called once before dump and/or report
	virtual void wrapUp();

public:

	inline void setWid(int wid) { _wid = wid;}

	/*
	 * Record data from the window specified by setWid
	 *
	 * Optional param a_args is a list of additional data to record
	 * If a_args is used, all calls to record should specify a_args of the same size
	 */
	virtual void record(std::initializer_list<double> a_args={}) = 0;

	/*
	 * Should be called once only at the end of system execution (e.g. at System::report)
	 * Assument record was called n>=1 times before this call.
	 * It will be called by this object's destructor if not called manually
	 */
	virtual void done() {
		assert_false(_doneCalled);
		wrapUp();
		dump();
		showReport();
		_doneCalled = true;
	}
};

class ExecutionSummary : public SensingDataTracer {
protected:
	const int D_IDX_TOTALT=0;
	const int D_IDX_BUSYT=1;
	const int D_IDX_UTIL=4;
	const int D_IDX_POWER=5;
	const int D_IDX_FREQ=6;

public:
	ExecutionSummary(sys_info_t *sys) :SensingDataTracer(sys){}

	virtual ~ExecutionSummary(){ if(!_doneCalled) done(); }

	virtual void record(std::initializer_list<double> a_args={});


protected:
	virtual void dump();
	virtual void showReport();

protected:
	inline std::string _pathNameTotal(){
		std::stringstream ss; ss << rt_param_outdir() << "/total.csv"; return ss.str();
	}

	template<typename C>
	inline void _dumpTotalPrintLine(std::ofstream &of, const std::string& ident,C &comp, database_type &data, int idx){
		assert_false(data[idx].size() != 1);
		assert_false(data[idx].size() != _time_series_size);
		of << ident << comp;
		_appendData(of,data[idx][0]->data_vals);
		of << "\n";
	}
};

class TimeTracer : public SensingDataTracer {

public:
	TimeTracer(sys_info_t *sys) :SensingDataTracer(sys)
	{
		pinfo("WARNING: \"TimeTracer\" is deprecated. Use \"ExecutionTrace\" instead.\n");
	}

	virtual ~TimeTracer(){ if(!_doneCalled) done(); }

	virtual void record(std::initializer_list<double> a_args={});

protected:
	virtual void dump();
	virtual void showReport() {}

protected:
	//maps task id (pid) to the idx in _d_task
	std::map<int,int> _task_id2idx;
	std::map<int,int> _task_idx2id;

private:
	template<typename T>
	inline std::string _pathNameTrace(const T &t){
		std::stringstream ss; ss << rt_param_outdir() << "/trace."<< t <<".csv"; return ss.str();
	}

	template<typename C>
	inline void _dumpComponentTimeSeries(C &comp, timeseries_data &data){
		_dumpTimeSeries(_pathNameTrace(comp),data);
	}

	void _dumpTimeSeries(std::string filename, timeseries_data &data);

	inline void _dumpTimeSeriesData(std::ofstream &of, timeseries_data &data){
		for(unsigned i = 0; i < data.size(); ++i){
			of << _timestamps[i];
			_appendData(of,data[i]->data_vals);
			of << "\n";
		}
	}
};

#endif

