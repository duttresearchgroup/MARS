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

#include <runtime/interfaces/common/sense_defs.h>
#include <core/base/base.h>

#include <runtime/common/reports.h>
#include <runtime/interfaces/sensing_module.h>
#include <sstream>

#include <external/minijson_writer/minijson_writer.hpp>


ExecutionTrace::ExecutionTraceHandle& ExecutionTrace::getHandle(const PerformanceData &sensedData, int wid)
{
	uint64_t timestampMS = sensedData.currWindowTimeMS(wid) - sensedData.sensingStartTimeMS();

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

const std::string ExecutionTrace::COL_TIMESTAMP = "timestamp";
const std::string ExecutionTrace::COL_SAMPLEID = "sample_id";

void ExecutionTrace::__dumpNew()
{
	//dump the header / overwrite existing file

	assert_true(_lastSampleDumped == -1);//if dumping header then samples must've been reset

	std::ofstream of(_pathName);
	of << COL_SAMPLEID << ";" << COL_TIMESTAMP;
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

void SysInfoPrinter::_print(std::ostream &os)
{
    minijson::object_writer writer(os,
            minijson::writer_configuration().pretty_printing(true));

    writer.write("core_list_size", _sys_info.core_list_size);
    {
      minijson::array_writer list_writer = writer.nested_array("core_list");
      for(int i = 0; i < _sys_info.core_list_size; ++i){
          minijson::object_writer entry_writer = list_writer.nested_object();
          assert_true(i == _sys_info.core_list[i].position);
          entry_writer.write("arch",archToString(_sys_info.core_list[i].arch));
          entry_writer.write("core_id",_sys_info.core_list[i].position);
          entry_writer.write("freq_domain_id",_sys_info.core_list[i].freq->domain_id);
          entry_writer.write("power_domain_id",_sys_info.core_list[i].freq->domain_id);
          entry_writer.close();
      }
      list_writer.close();
    }

    writer.write("freq_domain_list_size", _sys_info.freq_domain_list_size);
    {
      minijson::array_writer list_writer = writer.nested_array("freq_domain_list");
      for(int i = 0; i < _sys_info.freq_domain_list_size; ++i){
          minijson::object_writer entry_writer = list_writer.nested_object();
          assert_true(i == _sys_info.freq_domain_list[i].domain_id);
          entry_writer.write("domain_id",_sys_info.freq_domain_list[i].domain_id);
          entry_writer.write("core_cnt",_sys_info.freq_domain_list[i].core_cnt);
          {
              minijson::array_writer core_writer = entry_writer.nested_array("cores");
              for(int j = 0; j < _sys_info.core_list_size; ++j)
                  if(_sys_info.core_list[j].freq->domain_id == i)
                      core_writer.write(j);
              core_writer.close();
          }
          entry_writer.write("power_domain_cnt",_sys_info.freq_domain_list[i].power_domain_cnt);
          {
              minijson::array_writer pd_writer = entry_writer.nested_array("power_domains");
              for(int j = 0; j < _sys_info.power_domain_list_size; ++j)
                  if(_sys_info.power_domain_list[j].freq_domain->domain_id == i)
                      pd_writer.write(j);
              pd_writer.close();
          }
          entry_writer.close();
      }
      list_writer.close();
    }

    writer.write("power_domain_list_size", _sys_info.power_domain_list_size);
    {
      minijson::array_writer list_writer = writer.nested_array("power_domain_list");
      for(int i = 0; i < _sys_info.power_domain_list_size; ++i){
          minijson::object_writer entry_writer = list_writer.nested_object();
          assert_true(i == _sys_info.power_domain_list[i].domain_id);
          entry_writer.write("domain_id",_sys_info.power_domain_list[i].domain_id);
          entry_writer.write("core_cnt",_sys_info.power_domain_list[i].core_cnt);
          {
              minijson::array_writer core_writer = entry_writer.nested_array("cores");
              for(int j = 0; j < _sys_info.core_list_size; ++j)
                  if(_sys_info.core_list[j].power->domain_id == i)
                      core_writer.write(j);
              core_writer.close();
          }
          entry_writer.close();
      }
      list_writer.close();
    }


    writer.close();
}
