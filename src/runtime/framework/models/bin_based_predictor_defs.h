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

#ifndef __arm_rt_models_binbased_defs_h
#define __arm_rt_models_binbased_defs_h

#include <set>
#include <map>

#include <runtime/common/traceparser.h>
#include "bin_based_predictor_funcs.h"

namespace BinBasedPred {

typedef TraceParser::TaskName TaskName;
typedef TraceParser::ArchName ArchName;
typedef TraceParser::FrequencyMHz FrequencyMHz;

struct CoreFreq {
    CoreFreq(ArchName _a, FrequencyMHz _f):core(_a),freq(_f){}
    CoreFreq():core(""), freq(0){}
    ArchName core;
    FrequencyMHz freq;
    bool operator== (const CoreFreq &other) const {
        return (core == other.core) && (freq == other.freq);
    }
    bool operator!= (const CoreFreq &other) const {
        return !(*this == other);
    }
    bool operator< (const CoreFreq &other) const {
        return (core < other.core) || ((core == other.core) && (freq < other.freq));
    }
};

struct CoreFreqPair {
    CoreFreq src;
    CoreFreq tgt;
    CoreFreqPair(CoreFreq _a, CoreFreq _b):src(_a),tgt(_b){}


    bool operator== (const CoreFreqPair &other) const {
        return (src == other.src) && (tgt == other.tgt);
    }
    bool operator!= (const CoreFreqPair &other) const {
        return !(*this == other);
    }
    bool operator< (const CoreFreqPair &other) const {
        return (src < other.src) || ((src == other.src) && (tgt < other.tgt));
    }
};

struct Core_CoreFreq_Pair {
	ArchName src;
    CoreFreq tgt;
    Core_CoreFreq_Pair(ArchName _a, CoreFreq _b):src(_a),tgt(_b){}


    bool operator== (const Core_CoreFreq_Pair &other) const {
        return (src == other.src) && (tgt == other.tgt);
    }
    bool operator!= (const Core_CoreFreq_Pair &other) const {
        return !(*this == other);
    }
    bool operator< (const Core_CoreFreq_Pair &other) const {
        return (src < other.src) || ((src == other.src) && (tgt < other.tgt));
    }
};

struct CorePair {
	ArchName src;
	ArchName tgt;
    CorePair(ArchName _a, ArchName _b):src(_a),tgt(_b){}


    bool operator== (const CorePair &other) const {
        return (src == other.src) && (tgt == other.tgt);
    }
    bool operator!= (const CorePair &other) const {
        return !(*this == other);
    }
    bool operator< (const CorePair &other) const {
        return (src < other.src) || ((src == other.src) && (tgt < other.tgt));
    }
};

typedef std::vector<TraceParser::Sample>::iterator siter;

struct SampleWindow {
    CoreFreq owner;
    siter first;
    siter last;

    bool operator== (SampleWindow &other){
        return (owner == other.owner) && (first == other.first) && (last == other.last);
    }
    bool operator!= (SampleWindow &other){
        return !(*this == other);
    }

    int64_t numOfInstructions() {
    	if(first == last)//has only one sample
            return first->sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE);
    	else
            return (last->curr_instr - first->curr_instr) + first->sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE);
    }
};

struct MatchedWindow {
	TaskName task_name;
    SampleWindow bench1;
    SampleWindow bench2;

    bool operator== (MatchedWindow &other){
        return (bench1 == other.bench1) && (bench2 == other.bench2);
    }
    bool operator!= (MatchedWindow &other){
        return !(*this == other);
    }

    int64_t numOfInstructions() {
        return (bench1.numOfInstructions()+bench2.numOfInstructions())/2;
    }
    double numOfInstructionsDev() { return (bench1.numOfInstructions()-bench2.numOfInstructions())/(double)numOfInstructions(); }
    double numOfInstructionsDevStart() {
        return (bench2.first->curr_instr - bench1.first->curr_instr) / (double)bench1.first->curr_instr;
    }
    double numOfInstructionsDevEnd() {
        return (bench2.last->curr_instr - bench1.last->curr_instr) / (double)bench1.last->curr_instr;
    }
};

struct AveragedMatchedWindow {
    MatchedWindow window;
    TraceParser::CSVData bench1Avg;
    TraceParser::CSVData bench2Avg;

    AveragedMatchedWindow(MatchedWindow _w) :window(_w),
        bench1Avg(window.bench1.first->_data.parser, window.bench1.first->_data.task, window.bench1.first->_data.arch, window.bench1.first->_data.freq),
        bench2Avg(window.bench2.first->_data.parser, window.bench2.first->_data.task, window.bench2.first->_data.arch, window.bench2.first->_data.freq)
    {
        if((window.bench1.first == window.bench1.last) && (window.bench2.first == window.bench2.last)){
            bench1Avg = window.bench1.first->_data;
            bench2Avg = window.bench2.first->_data;
        }
        else{
            bench1Avg = average_sample(window.bench1.first, window.bench1.last);
            bench2Avg = average_sample(window.bench2.first, window.bench2.last);
        }
    }

    TraceParser::CSVData average_sample(siter start, siter end);
};

struct LayerConf {
    struct PredictionLayer {
        BinFunc metric;
        int max_bins;
    };

    std::vector<BinFunc> final_metric;
    std::vector<PredictionLayer> binning_metrics;

    void addLayer(int max_bins, BinFunc func){
        binning_metrics.push_back({func,max_bins});
    }

};

struct PredBin {
    double binBegin;
    double binEnd;
    double binRef;
    std::vector<double> binFinalPred;

    PredBin(double _binBegin, double _binEnd, double _binRef)
        :binBegin(_binBegin),binEnd(_binEnd),binRef(_binRef)
         { }
    PredBin() :binBegin(0),binEnd(0),binRef(0){ }

    virtual ~PredBin() {}

    double predValue(unsigned i){
        assert_true(i < binFinalPred.size());
        return binFinalPred[i];
    }
    bool isFinal(){
        return binFinalPred.size() > 0;
    }
};

struct MappingBin{
    static const int MIN_BIN_CNT = 1;

    double min,max;
    BinFunc metric;
    std::vector<PredBin*> *bins;
    int layer;//set only when the hierarcy is created
    MappingBin* prevLayer;//set only when the hierarcy is created

    std::map<PredBin*,MappingBin*> nextHierarchy;

    MappingBin* next(int idx)
    {
        assert_true(hasNext());
        assert_true(idx < (int)bins->size());
        auto iter = nextHierarchy.find((*bins)[idx]);
        assert_true(iter != nextHierarchy.end());
        return iter->second;
    }
    MappingBin* next(PredBin* bin)
    {
        assert_true(hasNext());
        auto iter = nextHierarchy.find(bin);
        assert_true(iter!=nextHierarchy.end());
        return iter->second;
    }

    bool hasNext() { return nextHierarchy.size() > 0; }

    MappingBin(double _min, double _max, BinFunc _metric)
      :min(_min),max(_max),metric(_metric),bins(nullptr), layer(-1), prevLayer(nullptr){
        bins = new std::vector<PredBin*>;
        assert_true(bins != nullptr);
    }
    MappingBin()
    :min(0),max(0),metric((BinFuncID)0),bins(nullptr), layer(-1), prevLayer(nullptr){
        bins = new std::vector<PredBin*>;
        assert_true(bins != nullptr);
    }
    ~MappingBin(){
        for(auto b : nextHierarchy) delete b.second;
        for(auto b : *bins) delete b;
        delete bins;
    }

    std::ostream& print(std::ostream &out){
        return print(out,*this,"");
    }

    static MappingBin* create_bins(std::vector<AveragedMatchedWindow> *samples, LayerConf funcs);

    void checkConsistency()
    {
        assert_true(bins != nullptr);
        assert_true(bins->size() > 0);
        if(hasNext()){
            assert_true(nextHierarchy.size() > 0);
            for(auto b : *bins)
                assert_true(next(b) != nullptr);
            for(auto b : *bins)
                next(b)->checkConsistency();
        }
    }

private:

    static bool tryToReduce(int layer, LayerConf &funcs);

    std::ostream& print(std::ostream &out, const MappingBin &bin, std::string ident);

    typedef std::vector<LayerConf::PredictionLayer>::iterator LayerIter;

    MappingBin* _make_hierachy(int layerIdx, LayerIter curr, LayerIter end);

    //always for the src benchmark (bench1)
    static MappingBin* _find_bins(std::vector<AveragedMatchedWindow> *samples, LayerIter curr);
};

};

#endif
