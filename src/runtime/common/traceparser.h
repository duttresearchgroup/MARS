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

#ifndef __arm_rt_traceparser_h
#define __arm_rt_traceparser_h

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdarg>
#include <thread>
#include <mutex>

#include <base/base.h>
#include <runtime/common/semaphore.h>
#include <runtime/interfaces/common/perfcnts.h>
#include <runtime/framework/types.h>

class TraceParser {

    friend class TraceSensingInterace;

  public:
    typedef std::string TaskName;
    typedef std::string ArchName;
    typedef int FrequencyMHz;

    struct CSVData {
        //gets a handle for the data using the string name
        double& operator()(const std::string &entry)
        {
            auto iter = parser._columnMap.find(entry);
            assert_true(iter!=parser._columnMap.end());
            assert_true(iter->second < (int)data.size());
            assert_true(iter->second >= 0);
            return data[iter->second];
        }
        //gets a handle using a "sense-like" interface
        template<SensingType S> double& sense() { return (*this)(sen_str<S>()); }
        template<SensingType S> double& sense(perfcnt_t perfcnt)
        { static_assert(S==SEN_PERFCNT,"S must be SEN_PERFCNT"); return (*this)(perfcnt_str(perfcnt)); }

        CSVData(TraceParser &p, const TaskName &t, const ArchName &a, const FrequencyMHz &f)
            :parser(p), task(t), arch(a), freq(f) {}
        TraceParser &parser;
        //refs to the task, core type and freq this sample was collected (used for checks)
        const TaskName &task;
        const ArchName &arch;
        const FrequencyMHz &freq;
        std::vector<double> data;

        CSVData& operator=(const CSVData &o){
            assert_true(&parser == &o.parser);
            assert_true(arch == o.arch);
            assert_true(freq == o.freq);
            data = o.data;
            return *this;
        }
    };

    struct Sample {
        double curr_time;  //time this sample started in seconds, first sampple starts at 0
        int64_t curr_instr; //num of instruction executed up to the time this sample was taken
        CSVData _data;  //stats for the period between curr_time and the curr_time of the previous sample
        //convenience funcs to access the stats. Just forwards to CSVData
        double& operator()(const std::string &entry) { return _data(entry); }
        template<SensingType S> double& sense() { return _data.sense<S>(); }
        template<SensingType S> double& sense(perfcnt_t perfnt) { return _data.sense<S>(perfnt); }

        Sample(TraceParser &trace, const TaskName &t, const ArchName &a, const FrequencyMHz &f)
            :curr_time(0),curr_instr(0),_data(trace,t,a,f){}
    };

    typedef std::unordered_map<
                TaskName,
                std::unordered_map<
                    ArchName,
                    std::unordered_map<
                        FrequencyMHz,
                        std::vector<Sample>*
                    >
                >
            >
    TraceSamples;

    struct Traces {
        TraceParser &parser;

        // Whether or not the sampling rate was fixed
        // If the sampling rate is fixed, the rate is
        // stored at sampleRate and this data can be
        // used for offline sim
        enum SampleType {
            SAMPLE_VARIABLE_RATE,
            SAMPLE_FIXED_RATE,
        };
        SampleType data_format;
        double sampleRate;

        // If the stddev of sampleRate div by sampleRate is
        // greater than TOLERANCE, then this is variable rate
        static constexpr double TOLERANCE = 0.1;

        TraceSamples samples;

        Traces(TraceParser &p)
            :parser(p),
             data_format(SAMPLE_VARIABLE_RATE),
             sampleRate(0)
        {}

        bool havePerfcnt(perfcnt_t perfcnt) const
        {
            return parser._columnMap.find(perfcnt_str(perfcnt)) != parser._columnMap.end();
        }
    };

    TraceParser(std::vector<std::string> dirs);


  private:
    Traces _traces;

    // Maps the name of a column in the trace to
    // the index in the parsed data structure.
    // All traces parsed by the same traced object
    // should have the same exact format.
    std::unordered_map<std::string,int> _columnMap;

    //When using multiple threads to parse CSVs
    std::mutex _columnMapLock;
    std::vector<std::thread *> _threads;
    Semaphore *_threadDone;
    struct rate_check {
        double sampleRate;
        const char* csvFile;
    };
    std::vector<rate_check> _sampleRates;
    std::mutex _sampleRateLock;
    Semaphore _sampleDone;

  public:
    const Traces& traces() { return _traces; }

  private:
    static void _parse_csv_data_thread(TraceParser *self, const char *file, std::vector<Sample> *dataVector, const TaskName *confTask, const ArchName *confArch, const FrequencyMHz *confFreq);
    double _parse_csv_data(const char *csvFile, std::vector<Sample> &dataVector, const TaskName &confTask, const ArchName &confArch, const FrequencyMHz &confFreq);

    // Mandatory columns in the trace csv file
    std::vector<std::string> _requiredCols;

    static void _computeCurrTime(std::vector<Sample> &dataVector);
    static void _computeCurrInstr(std::vector<Sample> &dataVector);
    static void _correctInstr(std::vector<Sample> &dataVector, double targetTotal);

    void _normalizeInstructions();
};

class TraceSensingInterace {
  public:
    typedef TraceParser::Traces Traces;
    typedef std::vector<TraceParser::Sample>::iterator SampleIter;
    typedef TraceParser::TaskName TaskName;
    typedef TraceParser::ArchName ArchName;
    typedef TraceParser::FrequencyMHz FrequencyMHz;
    typedef TraceParser::CSVData CSVData;
    typedef TraceParser::Sample Sample;

  private:
    const Traces& _traces;
    TraceParser& _parser;

    TaskName _currTask;
    ArchName _currArch;
    FrequencyMHz _currFreq;
    CSVData  _currData;
    std::vector<SensingAggType> _agg_vector;

    void _makeAggVector();

    TraceSensingInterace(const Traces& traces) :
        _traces(traces),
        _parser(traces.samples.begin()->second.begin()->second.begin()->second->begin()->_data.parser),
        _currTask(traces.samples.begin()->first),
        _currArch(traces.samples.begin()->second.begin()->first),
        _currFreq(traces.samples.begin()->second.begin()->second.begin()->first),
        _currData(_parser,_currTask,_currArch,_currFreq)
    {
        _makeAggVector();
        for(unsigned i = 0; i < _agg_vector.size(); ++i) _currData.data.push_back(0);
    }

    static TraceSensingInterace* _instance;

  public:

    static void set(const Traces& traces)
    {
        if(_instance != nullptr) delete _instance;
        _instance = new TraceSensingInterace(traces);
    }

    static void setTrace(const TaskName &task, const ArchName &arch, const FrequencyMHz &freq,
                  const SampleIter begin, const SampleIter end);
    static void setTrace(const SampleIter begin, const SampleIter end);
    static void setTrace(const CSVData &sample);
    static void setTrace(const Sample &sample);


    /*
     * Same interface as SensingInterface
     * Notice the data returned depends on the previous call to setTrace(...),
     * so the 'rsc' and 'wid' params are irrelevant here.
     * Unlike the original SensingInterface::sense, these always returns double
     * since all trace data is stored as double.
     *
     */
    template<SensingType SEN_T,typename ResourceT>
    static double sense(const ResourceT *rsc, int wid)
    {
        assert_true(_instance != nullptr);
        return _instance->_currData.sense<SEN_T>();
    }

    template<SensingType SEN_T,typename ResourceT>
    static double sense(typename SensingTypeInfo<SEN_T>::ParamType p, const ResourceT *rsc, int wid)
    {
        assert_true(_instance != nullptr);
        return _instance->_currData.sense<SEN_T>(p);
    }

    static const CSVData & senseRaw()
    {
        assert_true(_instance != nullptr);
        return _instance->_currData;
    }

};


#endif
