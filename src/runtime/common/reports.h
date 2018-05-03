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

#ifndef __arm_rt_reports_h
#define __arm_rt_reports_h

#include <vector>
#include <map>
#include <ostream>
#include <fstream>
#include <sstream>

#include <runtime/common/option_parser.h>
#include <runtime/framework/sensing_interface.h>
#include <runtime/interfaces/performance_data.h>
#include <runtime/interfaces/sensing_module.h>

class ExecutionTrace {

public:
	class ExecutionTraceHandle {
		friend class ExecutionTrace;

	public:
		double& operator()(const std::string &entry);

		int sampleIdx() { return _sampleIdx; }

	private:
		ExecutionTraceHandle(ExecutionTrace &trace)
			:_trace(trace),_sampleIdx(0)
		{}
		ExecutionTrace &_trace;
		int _sampleIdx;
	};

private:

	inline std::string _pathNameTrace(const std::string &traceName){
		std::stringstream ss; ss << Options::get<OPT_OUTDIR>() << "/" << traceName <<".csv"; return ss.str();
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

	ExecutionTraceHandle& getHandle(const PerformanceData &sensedData, int wid);

	ExecutionTraceHandle& getHandle(int wid)
	{
	    return getHandle(SensingModule::get().data(),wid);
	}

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

	static const std::string COL_TIMESTAMP;
	static const std::string COL_SAMPLEID;

};


class SysInfoPrinter {

private:

    inline std::string _pathName(){
        std::stringstream ss;
        ss << Options::get<OPT_OUTDIR>() << "/sys_info.json";
        return ss.str();
    }

    void _print(std::ostream &os);

    sys_info_t &_sys_info;

public:

    SysInfoPrinter(sys_info_t &sys_info) :_sys_info(sys_info){}

    void printToOutdirFile(){
        std::ofstream of(_pathName());
        _print(of);
    }

    std::string toString(){
        std::stringstream ss;
        _print(ss);
        return ss.str();
    }

};

#endif

