#ifndef __arm_rt_reports_h
#define __arm_rt_reports_h

#include <vector>
#include <map>
#include <ostream>
#include <fstream>
#include <sstream>

#include <runtime/common/rt_config_params.h>
#include <runtime/interfaces/sensed_data.h>

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

		traced_data(const SensingDataTracer &tracer,const sensed_data_cpu_t &sd,std::initializer_list<double> &a_args);
		traced_data(const SensingDataTracer &tracer,const sensed_data_task_t &sd,std::initializer_list<double> &a_args);
		traced_data(const SensingDataTracer &tracer,const sensed_data_power_domain_t &sd,std::initializer_list<double> &a_args);
		traced_data(const SensingDataTracer &tracer,const sensed_data_freq_domain_t &sd,std::initializer_list<double> &a_args);
		traced_data(const SensingDataTracer &tracer,const sensed_data_task_t &sd,const sensed_data_freq_domain_t &sd_freq,const sensed_data_power_domain_t &sd_power,std::initializer_list<double> &a_args);
		traced_data(const SensingDataTracer &tracer);

		inline double _total_time_s(const sensed_data_perf_counters_t &counters){
			return (double)counters.time_total_ms / (double)1000;
		}
		inline double _busy_time_s(const sensed_data_perf_counters_t &counters){
			return (double)counters.time_busy_ms / (double)1000;
		}
		inline double _total_ips(const SensedData &data,const sensed_data_perf_counters_t &counters){
			return (double) data.getPerfcntVal(counters,PERFCNT_INSTR_EXE) / _total_time_s(counters);
		}
		inline double _busy_ips(const SensedData &data,const sensed_data_perf_counters_t &counters){
			return (counters.time_busy_ms == 0) ? 0 : (double) data.getPerfcntVal(counters,PERFCNT_INSTR_EXE) / _busy_time_s(counters);
		}
		inline double _util(const sensed_data_perf_counters_t &counters){
			return (double)counters.time_busy_ms / (double) counters.time_total_ms;
		}
		inline double _power_w(const sensed_data_power_domain_t &counters){
			return ((double)counters.avg_power_uW_acc / (double) counters.time_ms_acc)/1000000;
		}
		inline double _freq_mhz(const sensed_data_freq_domain_t &counters){
			return ((double)counters.avg_freq_mhz_acc / (double) counters.time_ms_acc);
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
	const SensedData &_data;

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
	SensingDataTracer(sys_info_t *sys,const SensedData &data);
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
	ExecutionSummary(sys_info_t *sys,const SensedData &data) :SensingDataTracer(sys,data){}

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
	TimeTracer(sys_info_t *sys,const SensedData &data) :SensingDataTracer(sys,data)
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

class ExecutionSummaryWithTracedTask : public ExecutionSummary {

	const tracked_task_data_t *_traced_task;

public:
	ExecutionSummaryWithTracedTask(sys_info_t *sys,const SensedData &data)
		:ExecutionSummary(sys,data), _traced_task(nullptr){}

	virtual ~ExecutionSummaryWithTracedTask(){ if(!_doneCalled) done(); }

protected:
	virtual void showReport();
	virtual void dump();
	virtual void wrapUp();
};

class ExecutionTrace {

public:
	class ExecutionTraceHandle {
		friend class ExecutionTrace;

	public:
		double& operator()(const std::string &entry);

	private:
		ExecutionTraceHandle(ExecutionTrace &trace)
			:_trace(trace),_sampleIdx(0)
		{}
		ExecutionTrace &_trace;
		int _sampleIdx;
	};

private:

	inline std::string _pathNameTrace(const std::string &traceName){
		std::stringstream ss; ss << rt_param_outdir() << "/" << traceName <<".csv"; return ss.str();
	}

	const std::string _traceName;
	const std::string _pathName;

	bool _headerDumped;
	int _lastSampleDumped;
	int _currSample;
	bool _headerModified;

	ExecutionTraceHandle _traceHandle;

	std::map<std::string,std::map<int,double>> _data;
	std::vector<uint64_t> _timestampsMS;

	void __dumpNew();
	void _dump();



public:

	ExecutionTrace(const std::string &traceName)
		:_traceName(traceName),_pathName(_pathNameTrace(traceName)),
		_headerDumped(false),_lastSampleDumped(-1),_currSample(-1),_headerModified(false),
		_traceHandle(*this)
	{}

	~ExecutionTrace()
	{
		//pinfo("%s called\n",__PRETTY_FUNCTION__);
		dump();
	}

	ExecutionTraceHandle& getHandle(const SensedData &sensedData, int wid);

	void dump()
	{
		if(_headerDumped && _headerModified){
			pinfo("ExecutionTrace %s - Header modified after dump. Creating new file !\n",_traceName.c_str());
			forceCleanDump();
		}
		else
			_dump();
	}

	void forceCleanDump()
	{
		_headerDumped = false;
		_lastSampleDumped = -1;
		_dump();
	}

};


#endif

