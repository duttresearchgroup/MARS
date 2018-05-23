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

#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cassert>
#include <thread>
#include <glob.h>
#include <mutex>
#include <cmath>

#include "traceparser.h"
#include <runtime/common/strings.h>
#include <runtime/common/reports.h>
#include <runtime/common/statistics.h>
#include <runtime/managers/tracing.h>

struct glob_files {
    std::string file;
    std::string origDir;
};

static inline void glob(const std::string& pat, std::string& origDir, std::vector<glob_files> &ret){
    glob_t glob_result;
    glob(pat.c_str(),GLOB_TILDE,NULL,&glob_result);
    for(unsigned int i=0;i<glob_result.gl_pathc;++i){
        ret.push_back({std::string(glob_result.gl_pathv[i]),origDir});
    }
    globfree(&glob_result);
}

TraceParser::TraceParser(std::vector<std::string> dirs)
    :_threadDone(nullptr),_sampleDone(0)
{
    // The traces are generated by ExecutionTrace (see common/reports.sh)
    // in the TracingSystem (see systems/tracing.h), so the traces
    // should have the following columns in addition to
    // sample_id and timestamp columns
    _requiredCols = {
            ExecutionTrace::COL_SAMPLEID,ExecutionTrace::COL_TIMESTAMP,
            TracingSystem::T_TOTAL_TIME_S,
            TracingSystem::T_BUSY_TIME_S,
            TracingSystem::T_POWER_W,
            TracingSystem::T_FREQ_MHZ,
            TracingSystem::T_NIVCSW,
            TracingSystem::T_NVCSW,
            TracingSystem::T_CORE,
            //these perfcnts should always be included
            perfcnt_str(PERFCNT_INSTR_EXE),
            perfcnt_str(PERFCNT_BUSY_CY)
    };

    if(std::thread::hardware_concurrency() > 0) _threadDone = new Semaphore(std::thread::hardware_concurrency());
    else                                        _threadDone = new Semaphore(1);
    //_threadDone = new Semaphore(1);

    std::vector<glob_files> files;
    for(auto origDir : dirs){
        std::string srcDir = origDir;
        srcDir += "/*.csv";
        glob(srcDir,origDir,files);
    }

    pinfo("Parsing %d CSV files using %d threads\n",(int)files.size(),_threadDone->getCount());

    for(const auto &csv_file : files){

        _threadDone->wait();

        char *aux = const_cast<char*>(csv_file.file.c_str());

        //std::cout << "CSV file: " << csv_file.file << "  " << csv_file.origDir << "\n";

        // Filename should be in the format
        // task_name--arch_name--freq_mhz.csv
        std::vector<std::string> tokens = splitstr<std::string>(csv_file.file,"/");//get the basename
        tokens = splitstr<std::string>(tokens.back(),"--");
        if(tokens.size() != 3) arm_throw(TraceException,"Invalid trace file name: %s",aux);
        TaskName tname = tokens[0];
        ArchName arch = tokens[1];
        tokens = splitstr<std::string>(tokens[2],".");
        if(tokens.size() != 2) arm_throw(TraceException,"Invalid trace file name: %s",aux);
        FrequencyMHz freq = fromstr<FrequencyMHz>(tokens[0]);
        if(freq <= 0) arm_throw(TraceException,"Invalid trace file name: %s",aux);

        //std::cout << "task_name=" << tname << " arch=" << arch << " freq=" << freq << "\n";

        if(_traces.samples.find(tname) == _traces.samples.end()){
            _traces.samples[tname] = std::unordered_map<ArchName,std::unordered_map<FrequencyMHz, std::vector<Sample>* > >();
        }
        if(_traces.samples[tname].find(arch) == _traces.samples[tname].end()){
            _traces.samples[tname][arch] = std::unordered_map<FrequencyMHz, std::vector<Sample>* >();
        }
        //no duplicates
        assert(_traces.samples[tname][arch].find(freq) == _traces.samples[tname][arch].end());

        _traces.samples[tname][arch][freq] = new std::vector<Sample>();

        //std::cout << "Parsing data\n";

        _threads.push_back(
          new std::thread(
            _parse_csv_data_thread,
                this, aux,
                _traces.samples[tname][arch][freq],
                &(_traces.samples.find(tname)->first),
                &(_traces.samples[tname].find(arch)->first),
                &(_traces.samples[tname][arch].find(freq)->first)
          )
        );

        //std::cout << "Done\n\n";
    }


    //check the result and compute avg sampling rate
    double avgSum = 0;
    double avgSum2 = 0;
    double avgCnt = 0;
    for(unsigned i = 0; i < files.size(); ++i){
        _sampleDone.wait();

        _sampleRateLock.lock();
        rate_check sampleRate = _sampleRates.back();
        _sampleRates.pop_back();
        _sampleRateLock.unlock();

        if(sampleRate.sampleRate <= 0) continue;

        avgSum += sampleRate.sampleRate;
        avgSum2 += sampleRate.sampleRate*sampleRate.sampleRate;
        avgCnt += 1;
        //std::cout << "Rate for file " << sampleRate.csvFile << " = " << sampleRate.sampleRate << " sec  avgRate= "<<avgSum/avgCnt<<" sec\n";
    }

    for (auto t : _threads){
        if(t->joinable()) t->join();
        delete t;
    }
    _threads.clear();

    //std::cout << "rate= " << avgSum/avgCnt << " sec\n";

    _normalizeInstructions();

    _traces.sampleRate = avgSum/avgCnt;
    double stdev = std::sqrt((avgSum2 / avgCnt) - (_traces.sampleRate * _traces.sampleRate));

    if(stdev/_traces.sampleRate > Traces::TOLERANCE){
        pinfo("Traces are variable rate. Norm stdev = %f\n",stdev/_traces.sampleRate);
        _traces.data_format = Traces::SAMPLE_VARIABLE_RATE;
        _traces.sampleRate = 0;
    }
    else{
        _traces.data_format = Traces::SAMPLE_FIXED_RATE;
    }
}

class CSVRow
{
  public:
    CSVRow() = default;

    CSVRow(const char* cstr) {
        std::stringstream ss(cstr);
        readNextRow(ss);
    }

    static const char separator = ';';

    std::string const& operator[](std::size_t index) const
    {
        assert(index < m_data.size());
        return m_data[index];
    }
    std::size_t size() const
    {
        return m_data.size();
    }
    void readNextRow(std::istream& str)
    {
        line.clear();
        std::getline(str,line);

        std::stringstream   lineStream(line);
        std::string         cell;

        m_data.clear();
        while(std::getline(lineStream,cell,separator))
        {
            //std::cout << "\t" << cell << "\n";
            m_data.push_back(cell);
        }
    }

    std::vector<std::string>    m_data;
    std::string         line;
};

inline
std::istream&
operator>>(std::istream& str,CSVRow& data)
{
    data.readNextRow(str);
    return str;
}

double
TraceParser::_parse_csv_data(const char *csvFile,
        std::vector<Sample> &dataVector,
        const TaskName &confTask, const ArchName &confArch, const FrequencyMHz &confFreq)
{
    std::ifstream file(csvFile);

    // Read and check header

    CSVRow row;
    file >> row;

    const std::string &timestamp_col = "timestamp";
    const std::string &sample_id_col = "sample_id";//check if has these

    if(row.size() < 3)
        arm_throw(TraceParserException,"Trace file header is invalid %s",csvFile);

    //initialize the header
    if(_columnMap.size() == 0){
        _columnMapLock.lock();
        if(_columnMap.size() == 0){
            for(unsigned i = 0; i < row.size(); ++i)
                _columnMap[row[i]] = i;
            for(auto col : _requiredCols){
                if(_columnMap.find(col) == _columnMap.end())
                    arm_throw(TraceParserException,"Trace file header %s does not have column %s",csvFile,col.c_str());
            }
        }
        _columnMapLock.unlock();
    }

    if(row.size() != _columnMap.size())
        arm_throw(TraceParserException,"File %s has a different trace header than expected",csvFile);

    for(unsigned i = 0; i < row.size(); ++i){
        auto iter = _columnMap.find(row[i]);
        if(iter == _columnMap.end())
            arm_throw(TraceParserException,"File %s has a different trace header than expected",csvFile);
        if(iter->second != (int)i)
            arm_throw(TraceParserException,"File %s has a different trace header than expected",csvFile);
    }

    // Now parse
    while(file >> row)
    {
        assert_true(row.size() == _columnMap.size());

        dataVector.emplace_back(*this,confTask,confArch,confFreq);
        Sample &data = dataVector.back();

        for(unsigned i = 0; i < row.size(); ++i){
            data._data.data.push_back(fromstr<double>(row[i]));
        }


        double timestamp = data(ExecutionTrace::COL_TIMESTAMP);
        double total_time = data(TracingSystem::T_TOTAL_TIME_S);
        double &freqmhz = data(TracingSystem::T_FREQ_MHZ);
        double &active_cycles = data(TracingSystem::T_BUSY_TIME_S);

        double total_cycles = total_time * freqmhz * 1000000;

        if(active_cycles > total_cycles){
            if(active_cycles > (total_cycles*1.1)){
                pinfo("FUCK: active_cycles > total_cycles\n");
                pinfo("\t file=%s\n)",csvFile);
                pinfo("\t timestamp = %f\n", timestamp);
                pinfo("\t total_time = %f\n", total_time);
                pinfo("\t total_cycles = %f\n", total_cycles);
                pinfo("\t activ_cycles = %f\n", active_cycles);
            }
            active_cycles = total_cycles;
        }

        if(((FrequencyMHz)freqmhz) != confFreq){
            if((freqmhz > (confFreq*1.1)) || (freqmhz < (confFreq*0.9))){
                pinfo("FUCK: freqmhz != confFreq\n");
                pinfo("\t timestamp = %f\n", timestamp);
                pinfo("\t file=%s\n)",csvFile);
                pinfo("\t freqmhz = %f\n", freqmhz);
                pinfo("\t confFreq = %d\n", confFreq);
            }
            freqmhz = confFreq;
        }
    }

    if(dataVector.size()==0) return 0;

    //obtain the avg distance between samples
    double skip = 0;
    int skipCnt = 0;
    for(unsigned int i = 1; i < dataVector.size(); ++i) {
        skip += dataVector[i](ExecutionTrace::COL_TIMESTAMP) - dataVector[i-1](ExecutionTrace::COL_TIMESTAMP);
        skipCnt += 1;
    }
    skip = skip/skipCnt;

    //ajust the time and the instruction acc
    _computeCurrTime(dataVector);
    _computeCurrInstr(dataVector);

    return skip;
}


void TraceParser::_computeCurrTime(std::vector<Sample> &dataVector){
    double offset = dataVector[0](ExecutionTrace::COL_TIMESTAMP);
    for(unsigned int i = 0; i < dataVector.size(); ++i)
        dataVector[i].curr_time = dataVector[i](ExecutionTrace::COL_TIMESTAMP) - offset;
}
void TraceParser::_computeCurrInstr(std::vector<Sample> &dataVector){
    double instrAcc = 0;
    for(unsigned int i = 0; i < dataVector.size(); ++i) {
        instrAcc += dataVector[i](perfcnt_str(PERFCNT_INSTR_EXE));
        dataVector[i].curr_instr = instrAcc;
    }
}


void TraceParser::_parse_csv_data_thread(TraceParser *self,
        const char *file,
        std::vector<Sample> *dataVector,
        const TaskName *confTask, const ArchName *confArch, const FrequencyMHz *confFreq)
{
    try {
        double sampleRate = self->_parse_csv_data(file,*dataVector,*confTask,*confArch,*confFreq);

        self->_sampleRateLock.lock();
        rate_check rc = {sampleRate,file};
        self->_sampleRates.push_back(rc);
        self->_sampleRateLock.unlock();

        self->_sampleDone.notify();

        self->_threadDone->notify();

    } arm_catch(exit,0)
}

void TraceParser::_normalizeInstructions()
{
    constexpr double NORM_STDEV_THR = 0.02;//print a warnign if stddev > 2%

    /*std::cout << "Summary pre normalization: \n";
    for(auto bench : _traces.samples){
        std::cout << "\t" << bench.first << "\n";
        for(auto arch : bench.second){
            std::cout << "\t\t" << arch.first << "\n";
            for(auto freq : arch.second){
                std::cout << "\t\t\t" << freq.first << ", " << freq.second->size() << " samples\n";
                for(auto s : *(freq.second)){
                    std::cout << "\t\t\t\t" << "curr_time=" <<  s.curr_time << "\tcurr_inst=" << s.curr_instr << "\ttimestamp=" << s(ExecutionTrace::COL_TIMESTAMP) << "\tinstrs=" << s.sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE)  <<  "\n";
                }
            }
        }
    }*/

    for(auto bench : _traces.samples){
        std::vector<double> instrs;
        for(auto arch : bench.second){
            for(auto freq : arch.second){
                instrs.push_back(freq.second->back().curr_instr);
            }
        }
        Statistics::Average avg = Statistics::average(instrs);
        double normstdev = avg.stddev/avg.average;
        if(normstdev > NORM_STDEV_THR)
            pinfo("WARNING: stdev of traces for app %s is %f\n",bench.first.c_str(),normstdev);

        //std::cout << "\t" << bench.first << " " << avg.average << " " << avg.stddev/avg.average << "\n";
        for(auto arch : bench.second){
            for(auto freq : arch.second){
                //std::cout << "\t\t" << (freq.second->back().curr_instr - avg.average)/avg.average << "\n";
                _correctInstr(*(freq.second),avg.average);
            }
        }

    }

    /*std::cout << "Summary post normalization: \n";
    for(auto bench : _traces.samples){
        std::cout << "\t" << bench.first << "\n";
        for(auto arch : bench.second){
            std::cout << "\t\t" << arch.first << "\n";
            for(auto freq : arch.second){
                std::cout << "\t\t\t" << freq.first << ", " << freq.second->size() << " samples\n";
                for(auto s : *(freq.second)){
                    std::cout << "\t\t\t\t" << "curr_time=" <<  s.curr_time << "\tcurr_inst=" << s.curr_instr << "\ttimestamp=" << s(ExecutionTrace::COL_TIMESTAMP) << "\tinstrs=" << s.sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE)  <<  "\n";
                }
            }
        }
    }*/
}

void TraceParser::_correctInstr(std::vector<Sample> &dataVector, double targetTotal){
    double currTotal = dataVector.back().curr_instr;
    double ajust = targetTotal - currTotal;
    for(unsigned int i = 0; i < dataVector.size(); ++i) {
        double &instr = dataVector[i](perfcnt_str(PERFCNT_INSTR_EXE));
        instr += (instr/currTotal) * ajust;
    }
    _computeCurrInstr(dataVector);
}



TraceSensingInterace* TraceSensingInterace::_instance = nullptr;


void TraceSensingInterace::_makeAggVector()
{
    // initially set all to SIZE_SEN_AGG which means we dont know how
    // to aggregate
    for(unsigned i = 0; i < _parser._columnMap.size(); ++i){
        _agg_vector.push_back(SIZE_SEN_AGG);
    }
    //now sets the ones we know
    for(int j = 0; j < SIZE_SEN_TYPES; ++j ){
        SensingType type = (SensingType)j;
        const std::string &name = sen_str(type);
        SensingAggType agg = sen_agg(type);
        auto i =  _parser._columnMap.find(name);
        if(i != _parser._columnMap.end()){
            _agg_vector[i->second] = agg;
        }
    }
    for(int j = 0; j < SIZE_PERFCNT; ++j ){
        perfcnt_t type = (perfcnt_t)j;
        const std::string &name = perfcnt_str(type);
        auto i =  _parser._columnMap.find(name);
        if(i != _parser._columnMap.end()){
            _agg_vector[i->second] = SensingTypeInfo<SEN_PERFCNT>::agg;
        }
    }

    assert_true(_agg_vector.size() == _parser._columnMap.size());
}

void TraceSensingInterace::setTrace(const TaskName &task, const ArchName &arch, const FrequencyMHz &freq,
        const SampleIter begin, const SampleIter end)
{
    assert_true(_instance != nullptr);
    _instance->_currTask = task;
    _instance->_currArch = arch;
    _instance->_currFreq = freq;
    setTrace(begin,end);
}

void TraceSensingInterace::setTrace(const SampleIter begin, const SampleIter end)
{
    assert_true(_instance != nullptr);
    assert_true(&(begin->_data.parser) == &(_instance->_currData.parser));
    assert_true(begin->_data.task == _instance->_currTask);
    assert_true(begin->_data.arch == _instance->_currArch);
    assert_true(begin->_data.freq == _instance->_currFreq);
    assert_true(_instance->_agg_vector.size() == _instance->_currData.data.size());
    assert_true(begin->_data.data.size() == _instance->_currData.data.size());
    assert_true(end->_data.data.size() == _instance->_currData.data.size());

    // Currentlly we are not aggregating SEN_AGG_INT_COUNT and treat it as
    // SIZE_SEN_AGG (we keep the last value)

    const int nVals = _instance->_currData.data.size();
    for(int i = 0; i < nVals; ++i) _instance->_currData.data[i] = 0;
    SampleIter iter = begin;
    double cnt = 0;
    while(true){
        for(int i = 0; i < nVals; ++i){
            _instance->_currData.data[i] += iter->_data.data[i];
        }
        ++cnt;
        if(iter == end) break;
        ++iter;
    }
    for(int i = 0; i < nVals; ++i){
        if(_instance->_agg_vector[i] == SEN_AGG_MEAN)
            _instance->_currData.data[i] /= cnt;
        else if((_instance->_agg_vector[i] == SEN_AGG_INT_COUNT) || (_instance->_agg_vector[i] == SIZE_SEN_AGG))
            _instance->_currData.data[i] = end->_data.data[i];
        //else use the sum for SEN_AGG_SUM
    }
}


void TraceSensingInterace::setTrace(const CSVData &sample)
{
    assert_true(_instance != nullptr);
    assert_true(sample.data.size() == _instance->_currData.data.size());
    assert_true(&(sample.parser) == &(_instance->_currData.parser));
    for(unsigned i = 0; i < sample.data.size(); ++i){
        _instance->_currData.data[i] = sample.data[i];
    }
}

void TraceSensingInterace::setTrace(const Sample &sample)
{
    setTrace(sample._data);
}
