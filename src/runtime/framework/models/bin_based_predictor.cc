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

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <set>
#include <sys/stat.h>

#include <external/minijson_writer/minijson_writer.hpp>
#include <external/minijson_reader/minijson_reader.hpp>

#include <runtime/common/strings.h>

#include "bin_based_predictor.h"

namespace BinBasedPred {

TraceParser::CSVData AveragedMatchedWindow::average_sample(siter start, siter end)
{
    TraceSensingInterace::setTrace(start->_data.task, start->_data.arch, start->_data.freq,start,end);
    return TraceSensingInterace::senseRaw();
}

constexpr double ACCEPT_THR = 0.06;
static bool accept_window(MatchedWindow &w){
    return (std::fabs(w.numOfInstructionsDev()) < ACCEPT_THR) &&
            (std::fabs(w.numOfInstructionsDevStart()) < ACCEPT_THR) &&
            (std::fabs(w.numOfInstructionsDevEnd()) < ACCEPT_THR);
}
static bool accept_start(MatchedWindow &w){
    return std::fabs(w.numOfInstructionsDevStart()) < ACCEPT_THR;
}

static
bool
_find_best_window_aux(int minWindowSize, int maxWindowSize, MatchedWindow &window,
                 std::vector<TraceParser::Sample> &bench1,
                 std::vector<TraceParser::Sample> &bench2)
{
    assert_true(minWindowSize >= 1);
    assert_true((maxWindowSize == 0)||(maxWindowSize >= minWindowSize));

    if(window.bench1.first == bench1.end()) return false;
    if(window.bench2.first == bench2.end()) return false;

    //find a matching start point
    siter bench2start = window.bench2.first;
    while(!accept_start(window)){
        if(window.bench2.first->curr_instr > window.bench1.first->curr_instr){
            window.bench2.first = bench2start;
            window.bench1.first += 1;
            if(window.bench1.first == bench1.end()) return false;
            continue;
        }

        window.bench2.first += 1;

        if(bench2start == bench2.end()){
            window.bench2.first = bench2start;
            window.bench1.first += 1;
            if(window.bench1.first == bench1.end()) return false;
            continue;
        }
    }

    //find a matching end point
    // min/max window size applies to bench1, so:
    // 1) set bench1 window size = min
    // 2) find a bench1 window of the set size
    // 3) search for a window of bench2 that matches the one from 2)
    // 4) if not found, do bench1 window size += 1 and repeat from 2)
    //    until bench1 window size = max or end()
    int currWinSize = minWindowSize;
    while((maxWindowSize == 0) || (currWinSize <= maxWindowSize)){

        //ajusts bench1 ending to currWinSize
        window.bench1.last = window.bench1.first;
        for(int i = 1; i < currWinSize; ++i){
            window.bench1.last += 1;
            if(window.bench1.last == bench1.end()) return false;
        }

        window.bench2.last = window.bench2.first;
        while(window.bench2.last != bench2.end()){
            if(accept_window(window)){//finally
                return true;
            }
            if(window.bench2.last->curr_instr > window.bench1.last->curr_instr)
                break;//further increasing bench2 won't help
            window.bench2.last += 1;
        }

        currWinSize += 1;
    }

    //found nothing
    return false;
}

static
bool
find_best_window(int minWindowSize, int maxWindowSize, MatchedWindow &window,
                 std::vector<TraceParser::Sample> &bench1,
                 std::vector<TraceParser::Sample> &bench2)
{

    while(!_find_best_window_aux(minWindowSize,maxWindowSize,window,bench1,bench2)){
        if(window.bench1.first == bench1.end()) return false;
        //we may have found a good starting point but couldn't find a window afterwards,
        //so try again with the start pointer further up
        window.bench1.first += 1;
        if(window.bench1.first == bench1.end()) return false;
    }
    return true;
}

static void window(std::vector<MatchedWindow> &windows, TaskName task,
            ArchName srcArc, FrequencyMHz srcFreq, std::vector<TraceParser::Sample> *srcSamples,
            ArchName tgtArc, FrequencyMHz tgtFreq, std::vector<TraceParser::Sample> *tgtSamples,
            int minWindowSize, int maxWindowSize)
{
    // start searching from the last sample of the previously found window,
    // so we create an initial window with the inial sample == to begin()
    MatchedWindow window;
    window.task_name = task;
    window.bench1.owner = CoreFreq(srcArc,srcFreq);
    window.bench2.owner = CoreFreq(tgtArc,tgtFreq);
    window.bench1.first = srcSamples->begin();
    window.bench2.first = tgtSamples->begin();
    // find_best_window returns false if no more windows can be found
    while(find_best_window(minWindowSize, maxWindowSize, window, *srcSamples, *tgtSamples)){
        windows.push_back(window);
        window.bench1.first = window.bench1.last + 1;
        window.bench2.first = window.bench2.last + 1;
    }
}

std::ostream& operator<<(std::ostream &out, const CoreFreq &corefreq){
    out << corefreq.core << "@" << corefreq.freq;
    return out;
}
CoreFreq coreFreqFromStr(const std::string &s){
    std::vector<std::string> aux = splitstr<std::string>(s,"@");
    assert_true(aux.size()==2);
    return CoreFreq(aux[0],fromstr<FrequencyMHz>(aux[1]));
}
std::ostream& operator<<(std::ostream &out, const CoreFreqPair &corefreq){
    out << corefreq.src << ">" << corefreq.tgt;
    return out;
}
CoreFreqPair coreFreqPairFromStr(const std::string &s){
    std::vector<std::string> aux = splitstr<std::string>(s,">");
    assert_true(aux.size()==2);
    return CoreFreqPair(
            coreFreqFromStr(aux[0]),
            coreFreqFromStr(aux[1]));
}
std::ostream& operator<<(std::ostream &out, const Core_CoreFreq_Pair &corefreq){
    out << corefreq.src << ">" << corefreq.tgt;
    return out;
}
std::ostream& operator<<(std::ostream &out, const CorePair &corefreq){
    out << corefreq.src << ">" << corefreq.tgt;
    return out;
}


static void sumup_windows(std::string bench,
                   std::vector<MatchedWindow> &windows,
                   std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows){
    for (auto w : windows){
    	CoreFreqPair pair(w.bench1.owner, w.bench2.owner);
        if(averagedWindows.find(pair) == averagedWindows.end()) averagedWindows[pair] = new std::vector<AveragedMatchedWindow>();
        averagedWindows[pair]->push_back(AveragedMatchedWindow(w));
    }
}

struct TrainingBin : PredBin {
    //all sample between (PredBin::binBegin,PredBin::binEnd]
    std::vector<AveragedMatchedWindow> *samples;

    TrainingBin(double _binBegin, double _binEnd, double _binRef)
        :PredBin(_binBegin,_binEnd,_binRef), samples(nullptr){
        samples = new std::vector<AveragedMatchedWindow>;
        assert(samples != nullptr);
    }
    virtual ~TrainingBin(){
        if(samples != nullptr) delete samples;
    }
    void makeFinal(std::vector<BinFunc> &final_metric){
        for(auto &func : final_metric){
            double acc = 0;
            for(auto sample : *samples){
                acc += func(sample.bench2Avg);
            }
            binFinalPred.push_back(acc/samples->size());
        }
    }
};

std::ostream& operator<<(std::ostream &out, const PredBin &bin){
    const TrainingBin *_bin = dynamic_cast<const TrainingBin*>(&bin);
    if(_bin)
        out << "(from " << _bin->binBegin << " to " << _bin->binEnd << " " <<  _bin->samples->size() << " samples)";
    else
        out << "(from " << bin.binBegin << " to " << bin.binEnd << ")";

    return out;
}

struct Histogram{
    const int MAX_BINS = 100;

    double min;
    double max;
    BinFunc func;
    std::vector<PredBin*> bins;

    Histogram(double _min, double _max, int initialSize, BinFunc _func);

    void delete_bins()
    {
        for (auto b : bins) delete b;
    }

    void add_sample(AveragedMatchedWindow &s);

    bool empty_bins()
    {
        for (auto b : bins){
            TrainingBin *_b = dynamic_cast<TrainingBin*>(b);
            if (_b && _b->samples->size() == 0)
                return true;
        }
        return false;
    }
    bool make_bin();
};

Histogram::Histogram(double _min, double _max, int initialSize, BinFunc _func)
:min(_min), max(_max), func(_func)
{
    assert(initialSize <= MAX_BINS);
    assert(initialSize > 0);

    double binSize = (max - min) / initialSize;

    int binIdx;
    double binEnd;
    for(binEnd = min + binSize, binIdx = 1; binIdx <= initialSize ; binEnd += binSize, ++binIdx){
        double _binStart = binEnd - binSize;
        double _binEnd = binEnd;
        double _binRef = (_binStart + _binEnd)/2;
        if(binIdx == 1) _binStart = std::numeric_limits<double>::lowest();
        if(binIdx == initialSize) _binEnd = std::numeric_limits<double>::max();
        bins.push_back(new TrainingBin(_binStart,_binEnd,_binRef));
    }
}

void Histogram::add_sample(AveragedMatchedWindow &s){
    bool ok = false;
    double val = (func)(s.bench1Avg);
    for(auto _bin : bins){
        TrainingBin *bin = dynamic_cast<TrainingBin*>(_bin); assert_true(bin!=nullptr);
        if(val <= bin->binEnd){
            bin->samples->push_back(s);
            ok = true;
            assert(val >= bin->binBegin);
            break;
        }
    }
    if(!ok){
        arm_throw(BinBasedPredException,"Failed to bin sample with val=%f",val);
    }
}


bool Histogram::make_bin()
{
    //it is invalid to call if we have empty bins
    //if size is 1, we know its impossible to break up
    if(empty_bins() || (bins.size()==1)) return false;

    auto maxBin = bins.size();
    unsigned int nsamples = 0;
    for(unsigned int i = 0 ; i < bins.size(); ++i){
        TrainingBin *bin = dynamic_cast<TrainingBin*>(bins[i]); assert_true(bin!=nullptr);
        unsigned int aux = bin->samples->size();
        if((aux >= 2) && (aux >= nsamples)){
            nsamples = aux;
            maxBin = i;
        }
    }
    if(maxBin == bins.size()) return false;

    //the new bin is to be inserted before bins[maxBin]

    double oldBinBegin =  bins[maxBin]->binBegin;
    double oldBinEnd =  bins[maxBin]->binEnd;
    double oldBinRef = bins[maxBin]->binRef;
    if(maxBin == bins.size()-1) oldBinEnd = max;
    if(maxBin == 0) oldBinBegin = min;

    double newBinBegin = oldBinBegin;
    double newBinEnd = oldBinRef;
    double newBinRef = (newBinBegin + newBinEnd)/2;

    oldBinBegin = newBinEnd;
    oldBinRef = (oldBinBegin + oldBinEnd)/2;

    if(maxBin == bins.size()-1) oldBinEnd = std::numeric_limits<double>::max();
    if(maxBin == 0) newBinBegin = std::numeric_limits<double>::lowest();

    //bkp the original bin
    TrainingBin *oldBinMod = dynamic_cast<TrainingBin*>(bins[maxBin]); assert_true(oldBinMod!=nullptr);
    TrainingBin oldBin = *oldBinMod;

    //give a new set of samples to bins[maxBin] and update its bounds
    oldBinMod->samples = new std::vector<AveragedMatchedWindow>;
    oldBinMod->binBegin = oldBinBegin;
    oldBinMod->binEnd = oldBinEnd;
    oldBinMod->binRef = oldBinRef;

    //insert a new bin before
    TrainingBin *newBin = new TrainingBin(newBinBegin,newBinEnd,newBinRef);
    auto insertPoint = bins.insert(bins.begin()+maxBin,newBin);

    //reinsert the samples
    for(auto s : *(oldBin.samples)) add_sample(s);

    //both bin must have samples
    if(empty_bins()){
        //revert and return false

        delete oldBinMod->samples;
        *oldBinMod = oldBin;
        oldBin.samples = nullptr;//avoid sample deletion when oldBin goes out of scope

        bins.erase(insertPoint);
        delete newBin;

        return false;
    }

    //all fine; delete the old sample vector
    delete oldBin.samples;

    oldBin.samples = nullptr;

    return true;
}

static
void make_final_vals(LayerConf &funcs, MappingBin *predictor){
    if(predictor->hasNext()){
        for(unsigned i = 0 ; i < predictor->bins->size(); ++i)
            make_final_vals(funcs,predictor->next(i));
    }
    else{
        for(auto _bin : *(predictor->bins)){
            TrainingBin *bin = dynamic_cast<TrainingBin*>(_bin); assert_true(bin!=nullptr);
            bin->makeFinal(funcs.final_metric);
        }
    }
}

MappingBin* MappingBin::create_bins(std::vector<AveragedMatchedWindow> *samples, LayerConf funcs){
    MappingBin* firstBin = _find_bins(samples,funcs.binning_metrics.begin());
    if(firstBin != nullptr){
        MappingBin *ret = firstBin->_make_hierachy(0, funcs.binning_metrics.begin()+1, funcs.binning_metrics.end());
        if(ret != 0){
            pinfo("WARNING: _make_hierachy (%s:%d) failed at layer %d!\n",__FILE__,__LINE__,ret->layer);
            pinfo("layer %d has %lu bins\n",ret->layer,ret->bins->size());
            pinfo("reducing and try again\n");
            if(tryToReduce(ret->layer, funcs)){
                delete firstBin;
                return create_bins(samples,funcs);
            }
            else {
                pinfo("ERROR: cannot reduce further. It failed!\n");
                delete firstBin;
                return nullptr;
            }
        }
        make_final_vals(funcs,firstBin);
        return firstBin;
    }
    else{
        pinfo("ERROR: Unnable to create first level bins !\n");
        return firstBin;
    }
}

bool MappingBin::tryToReduce(int layer, LayerConf &funcs){
    for(int i = layer; i >= 0; --i){
        if(funcs.binning_metrics[i].max_bins > 2){
            funcs.binning_metrics[i].max_bins -= 1;
            return true;
        }
    }
    return false;
}

std::ostream& MappingBin::print(std::ostream &out, const MappingBin &bin, std::string ident){
    out << ident << "(min=" << bin.min << ",max=" << bin.max << ", metric=" << *(metric.str) << ", samples=[\n";
    for(auto b : *(bin.bins)){
        out << ident << *b << "\n";
        if(hasNext()){
            nextHierarchy[b]->print(out,*(nextHierarchy[b]),ident+"\t");
        }
    }
    out << ident << "]\n";
    return out;
}

typedef std::vector<LayerConf::PredictionLayer>::iterator LayerIter;

MappingBin* MappingBin::_make_hierachy(int layerIdx, LayerIter curr, LayerIter end){

    if(curr == end) return 0;

    this->layer = layerIdx;

    for(auto _b : *bins){
        TrainingBin *b = dynamic_cast<TrainingBin*>(_b); assert_true(b!=nullptr);
        MappingBin *next = _find_bins(b->samples, curr);
        if(next == nullptr) return this;
        nextHierarchy[b] = next;
    }

    //create the next layer
    for(auto b : *bins) {
        MappingBin *ret = nextHierarchy[b]->_make_hierachy(layerIdx+1,curr+1,end);
        if(ret != 0) return ret;
    }

    return 0;
}

//always for the src benchmark (bench1)
MappingBin* MappingBin::_find_bins(std::vector<AveragedMatchedWindow> *samples, LayerIter curr){
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();

    //find min/max
    for(auto s : *samples){
        double val = curr->metric(s.bench1Avg);
        assert(val>=0);
        if(val < min) min = val;
        if(val > max) max = val;
    }

    //find the largest number of bins such that each bin has
    //at least one sample
    Histogram *currHistogram = nullptr;
    for(int currBinCnt = MIN_BIN_CNT; currBinCnt <= curr->max_bins; ++currBinCnt){
        Histogram *hist = new Histogram(min,max,currBinCnt,curr->metric);

        for(auto s : *samples) hist->add_sample(s);

        if(hist->empty_bins()){
            //we keep the previous one
            hist->delete_bins();
            delete hist;
        }
        else{
            //drop the previous and keep this one
            if(currHistogram != nullptr){
                currHistogram->delete_bins();
                delete currHistogram;
            }
            currHistogram = hist;
        }
    }

    if(currHistogram == nullptr)
        return nullptr;

    if(currHistogram->empty_bins()){
        currHistogram->delete_bins();
        delete currHistogram;
        return nullptr;
    }

    for(int currBinCnt = currHistogram->bins.size(); currBinCnt < curr->max_bins; ++currBinCnt){
        if(!currHistogram->make_bin()) break;
    }
    assert(!currHistogram->empty_bins());

    MappingBin *mappingBins = new MappingBin(min,max,curr->metric);
    for (auto b : currHistogram->bins) mappingBins->bins->push_back(b);

    delete currHistogram;

    return mappingBins;
}

template<typename... Args>
static
void bin_based_predict(std::vector<double> &result, MappingBin *predictor, Args... args){
    MappingBin *currLevel = predictor;
    while(true){
        double val = currLevel->metric(args...);
        PredBin *currBin = nullptr;
        for(auto bin : *(currLevel->bins)){
            if(val <= bin->binEnd){
                currBin = bin;
                break;
            }
        }
        assert_true(currBin != nullptr);

        if(currLevel->hasNext())
            currLevel = currLevel->next(currBin);
        else{
            assert_true(currBin->isFinal());
            result = currBin->binFinalPred;
            break;
        }
    }
    assert_true(result.size() > 0);
}

static
void bin_based_predict_test(std::vector<double> &result, MappingBin *predictor, AveragedMatchedWindow &window){
    bin_based_predict(result,predictor,window.bench1Avg);
}


static
double bin_based_predict_test(LayerConf funcs, BinFunc finalMetric, int finalMetricIdx,
                         MappingBin *predictor,
                         AveragedMatchedWindow &window){
    double tgt = finalMetric(window.bench2Avg);
    double pred = -1234;

    std::vector<double> result;
    bin_based_predict_test(result,predictor,window);
    pred = result[finalMetricIdx];

    double error = 1 - (tgt/pred);
    return error;
}

struct AvgError{
    double acc;
    double accSqr;
    unsigned int cnt;
    double min;
    double max;
    AvgError()
    :acc(0),accSqr(0), cnt(0),
     min(std::numeric_limits<double>::max()),
     max(std::numeric_limits<double>::lowest())
    {}
    void add(double error){
        acc += error;
        accSqr += error*error;
        cnt += 1;
        if(error < min) min = error;
        if(error > max) max = error;
    }
    void add(AvgError &other){
        acc += other.avg();
        accSqr += other.stddev();
        cnt += 1;
        if(other.min < min) min = other.min;
        if(other.max > max) max = other.max;
    }

    double avg() {return acc/cnt;}

    double avgStddev() {return accSqr/cnt;}

    double stddev() {return std::sqrt((accSqr/cnt)-((acc/cnt)*(acc/cnt)));}

};

static
AvgError bin_based_predict_test(LayerConf funcs, BinFunc finalMetric, int finalMetricIdx,
                         MappingBin *predictor,
                         std::vector<AveragedMatchedWindow>* averagedWindows){
    AvgError error;
    for(auto &sample : *averagedWindows){
    	error.add(std::fabs(bin_based_predict_test(funcs, finalMetric, finalMetricIdx, predictor, sample)));
    }
    return error;
}

static
std::vector<AvgError> bin_based_predict(LayerConf funcs,
                         std::map<CoreFreqPair, MappingBin *> &predictors,
                         std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows){
    std::vector<AvgError> overall;
    for(unsigned i = 0; i < funcs.final_metric.size(); ++i) overall.push_back(AvgError());
    for(auto corefreq : averagedWindows){
        pinfo("Testing prediction for %s@%d -> %s@%d\n",corefreq.first.src.core.c_str(),corefreq.first.src.freq,corefreq.first.tgt.core.c_str(), corefreq.first.tgt.freq);
        assert(funcs.final_metric.size() == 2);
        for(unsigned i = 0; i < funcs.final_metric.size(); ++i){
        	AvgError avgError = bin_based_predict_test(funcs,funcs.final_metric[i],i,predictors[corefreq.first],corefreq.second);
        	pinfo("\tAvg error for %s=%f%% StDev=%f%% Min=%f%% Max=%f%%\n",funcs.final_metric[i].str->c_str(),avgError.avg() * 100,avgError.stddev()*100,avgError.min*100,avgError.max*100);
        	overall[i].add(avgError);
        }
    }
    return overall;
}

static
double count_bin(MappingBin *bin, int layer, int currLayer){
    assert((currLayer == layer) || bin->hasNext());

    if(layer == currLayer) return bin->bins->size();

    double avg = 0;
    int acc = 0;
    for(auto b : bin->nextHierarchy){
        avg += count_bin(b.second,layer,currLayer+1);
        acc += 1;
    }

    return avg/acc;
}

static
void print_pred_info(LayerConf funcs,
                     std::map<CoreFreqPair, MappingBin *> &predictors){
    pinfo("Predictor info:\n");
    pinfo("\tnum layers = %lu\n",funcs.binning_metrics.size());
    for(unsigned i = 0; i < funcs.binning_metrics.size(); ++i){
        pinfo("\tLayer %u number of bins\n",i);
        double avg = 0;
        int acc = 0;
        int min = 999999999;
        CoreFreqPair minPair = predictors.begin()->first;
        int max = 0;
        for(auto pair : predictors){
            double val = count_bin(pair.second,i,0);
            avg += val;
            acc += 1;
            if(val < min) {
                min = val;
                minPair = pair.first;
            }
            if(val > max) max = val;
        }
        pinfo("\t\tavg %f\n",avg/acc);
        pinfo("\t\tmin %d\n",min);
        pinfo("\t\tminPair %s@%d -> %s@%d\n",minPair.src.core.c_str(),minPair.src.freq,minPair.tgt.core.c_str(),minPair.tgt.freq);
        pinfo("\t\tmax %d\n",max);
    }
}

template<typename FilterType, typename ElemType>
inline bool filter_helper(FilterType &filter, ElemType &elem, bool include){
    if(include)
        return !filter.empty() && (filter.find(elem) == filter.end());
    else
        return !filter.empty() && (filter.find(elem) != filter.end());
}

void TrainingData::obtain_windows(
        const TraceParser::Traces& traces,
        std::set<ArchName> archFilter, bool archFilterInclude,
        std::set<FrequencyMHz> freqFilter, bool freqFilterInclude,
        std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows,
        int minWindowSize, int maxWindowSize){

    TraceSensingInterace::set(traces);

    for(auto task : traces.samples){
        pinfo("Finding windows for %s\n",task.first.c_str());

        std::vector<MatchedWindow> windows;

        for(auto archSrc : task.second){
            if(filter_helper(archFilter,archSrc.first,archFilterInclude)) continue;

            for(auto freqSrc : archSrc.second){
                if(filter_helper(freqFilter,freqSrc.first,freqFilterInclude)) continue;

                for(auto archTgt : task.second){
                    if(filter_helper(archFilter,archTgt.first,archFilterInclude)) continue;

                    for(auto freqTgt : archTgt.second){
                        if(filter_helper(freqFilter,freqTgt.first,freqFilterInclude)) continue;

                        window(windows, task.first,
                        		archSrc.first, freqSrc.first, freqSrc.second,
                                archTgt.first, freqTgt.first, freqTgt.second,
                                minWindowSize,maxWindowSize);
                    }
                }
            }
        }
        pinfo("\tObtained %lu samples\n",windows.size());
        sumup_windows(task.first,windows,averagedWindows);
    }
}


void Predictor::_create_predictors(std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows,
        std::map<CoreFreqPair, MappingBin *> &predictors, LayerConf &funcs)
{
    for(auto corefreq : averagedWindows){
        pinfo("Finding bins %s@%d -> %s@%d\n",corefreq.first.src.core.c_str(),corefreq.first.src.freq,corefreq.first.tgt.core.c_str(), corefreq.first.tgt.freq);
        MappingBin *bins = MappingBin::create_bins(corefreq.second, funcs);
        assert(bins != nullptr);
        predictors[corefreq.first] = bins;
    }
    print_pred_info(funcs, predictors);
}

void Predictor::_test_predictors(std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows,
        std::map<CoreFreqPair, MappingBin *> &predictors, LayerConf &funcs)
{
    std::vector<AvgError> errors = bin_based_predict(funcs, predictors, averagedWindows);
    int f = 0;
    for (auto err : errors){
        pinfo("Overall %s avg error=%f%% StDev=%f%% Min=%f%% Max=%f%%\n", funcs.final_metric[f++].str->c_str(),err.avg() * 100,err.stddev()*100,err.min*100,err.max*100);
    }
}


static core_arch_t archFromStr(const ArchName &arch)
{
    for(int i = 0; i < SIZE_COREARCH; ++i){
        std::string aux(archToString((core_arch_t)i));
        if(aux == arch) return (core_arch_t)i;
    }
    assert_false("Couldn't find arch!");
    return SIZE_COREARCH;
}
template<typename Map, typename Key>
typename Map::iterator findInsertDefault(Map &map, const Key &key){
    typename Map::iterator lb = map.find(key);
    if(lb == map.end())
        lb = map.insert(lb, typename Map::value_type(key,typename Map::value_type::second_type()));
    return lb;
}
template<typename Map, typename Key, typename Val>
typename Map::iterator findInsert(Map &map, const Key &key, const Val &val){
    typename Map::iterator lb = map.find(key);
    if(lb == map.end())
        lb = map.insert(lb, typename Map::value_type(key,val));
    return lb;
}


void Predictor::_make_faster()
{
    for(auto corefreq : _predictors){
        core_arch_t srcArch = archFromStr(corefreq.first.src.core);
        FrequencyMHz srcFreq = corefreq.first.src.freq;
        core_arch_t tgtArch = archFromStr(corefreq.first.tgt.core);
        FrequencyMHz tgtFreq = corefreq.first.tgt.freq;

        auto iSrcArc = findInsertDefault(_predictors_faster,srcArch);
        auto iSrcFreq = findInsertDefault(iSrcArc->second,srcFreq);
        auto iTgtArch = findInsertDefault(iSrcFreq->second,tgtArch);
        auto iTgtFreq = findInsertDefault(iTgtArch->second,tgtFreq);

        //cannot have duplicates
        assert_true(iTgtFreq->second == nullptr);
        iTgtFreq->second = corefreq.second;
    }
    // double check and adds to available* maps
    for(auto corefreq : _predictors){
        core_arch_t srcArch = archFromStr(corefreq.first.src.core);
        FrequencyMHz srcFreq = corefreq.first.src.freq;
        core_arch_t tgtArch = archFromStr(corefreq.first.tgt.core);
        FrequencyMHz tgtFreq = corefreq.first.tgt.freq;
        assert_true(_predictors_faster[srcArch][srcFreq][tgtArch][tgtFreq] == corefreq.second);

        auto iSrcArc = findInsertDefault(_availableSrcFreqs,srcArch);
        iSrcArc->second.insert(srcFreq);
        auto iTgtArc = findInsertDefault(_availableTgtFreqs,tgtArch);
        iTgtArc->second.insert(tgtFreq);
    }
}




// If search_val is in the set, both upper_bound and lower_bound are set to
// search_val, otherwise lower_bound and upper_bound are set to the nearest
// value < search_val and to the nearest value > search_val.
// If val is larger than the maximum or smaller than the minimum value in the
// set, lower_bound and upper_bound are set to the maximum and minimum
static inline void _search_bounds(const std::set<FrequencyMHz> &set, int search_val, int &lower_bound, int &upper_bound){
    auto upper = set.lower_bound(search_val);
    if((upper == set.end()) || (*upper > search_val+10) || (*upper < search_val-10) ){//10 Mhz tolerance
        auto lower = upper;--lower;
        if((upper == set.end()) || (lower == set.end())){
            lower_bound = *(set.begin());
            upper_bound = *(set.rbegin());
        }
        else{
            lower_bound = *lower;
            upper_bound = *upper;
        }

    }
    else{
        lower_bound = *upper;
        upper_bound = *upper;
    }
}
// If search_val is in the set, both upper_bound and lower_bound are set to
// search_val, otherwise lower_bound and upper_bound are set to the nearest
// value < search_val and to the nearest value > search_val.
// If val is larger than the maximum or smaller than the minimum value in the
// set, lower_bound and upper_bound are set to the maximum and minimum
static inline int _search_nearest(const std::set<FrequencyMHz> &set, int search_val){
    if(search_val <= *(set.begin())) return *(set.begin());
    if(search_val >= *(set.rbegin())) return *(set.rbegin());
    auto upper = set.lower_bound(search_val);
    auto lower = upper;--lower;
    if((*upper-search_val) < (search_val-*lower))
        return *upper;
    else
        return *lower;
}

void Predictor::predict(std::vector<double> &result,
        const tracked_task_data_t *task, int wid,
        const core_info_t *target_core, int target_freq_mhz)
{
    assert_true(_sys_info!=nullptr);
    int srcCore = SensingInterface::sense<SEN_LASTCPU>(task,wid);
    core_arch_t srcArch = _sys_info->core_list[srcCore].arch;
    int srcFreqMhz = SensingInterface::sense<SEN_FREQ_MHZ>(_sys_info->core_list[srcCore].freq,wid);

    auto srcArchI = _availableSrcFreqs.find(srcArch);
    assert_true(srcArchI!=_availableSrcFreqs.end());
    int srcFreqMhzNearest = _search_nearest(srcArchI->second,srcFreqMhz);

    auto tgtArchI = _availableTgtFreqs.find(target_core->arch);
    assert_true(tgtArchI!=_availableSrcFreqs.end());
    int tgtFreqMhz0, tgtFreqMhz1;
    _search_bounds(tgtArchI->second,target_freq_mhz,tgtFreqMhz0,tgtFreqMhz1);

    if(tgtFreqMhz0 != tgtFreqMhz1)
        _predict_interpolate(result,task,wid,srcArch,srcFreqMhzNearest,target_core->arch,tgtFreqMhz0,tgtFreqMhz1,target_freq_mhz);
    else
        _predict_single(result,task,wid,srcArch,srcFreqMhzNearest,target_core->arch,tgtFreqMhz0);

    assert(result.size()==_funcs.final_metric.size());

}

static inline double lin_interp(double x, double x0, double y0, double x1, double y1){
    return y0 + ((y1-y0)*(x-x0))/(x1-x0);
}

// Used whe we have the exact predictors for the src and tgt frequencies
void Predictor::_predict_single(std::vector<double> &result, const tracked_task_data_t *task, int wid,
        core_arch_t srcArch, FrequencyMHz srcFreq, core_arch_t tgtArch, FrequencyMHz tgtFreq)
{
    bin_based_predict(result,_predictors_faster[srcArch][srcFreq][tgtArch][tgtFreq],task,wid);
}

// Used when we don't have the exact predictors. We use the two nearest src/tgt frequencies
// and use linear interpolation to get the final value
void Predictor::_predict_interpolate(std::vector<double> &result, const tracked_task_data_t *task, int wid,
        core_arch_t srcArch, FrequencyMHz srcFreq,
        core_arch_t tgtArch, FrequencyMHz tgtFreq0, FrequencyMHz tgtFreq1, FrequencyMHz origTgtFreq)
{
    bin_based_predict(_predict_interpolate_result0,_predictors_faster[srcArch][srcFreq][tgtArch][tgtFreq0],task,wid);
    bin_based_predict(_predict_interpolate_result1,_predictors_faster[srcArch][srcFreq][tgtArch][tgtFreq1],task,wid);
    result.clear();
    assert_true(_predict_interpolate_result0.size() == _predict_interpolate_result1.size());
    for(unsigned i = 0; i < _predict_interpolate_result0.size(); ++i){
        result.push_back(lin_interp(origTgtFreq,tgtFreq0,_predict_interpolate_result0[i],tgtFreq1,_predict_interpolate_result1[i]));
    }
}
// Aux vectors for predict interpolate
thread_local std::vector<double> Predictor::_predict_interpolate_result0;
thread_local std::vector<double> Predictor::_predict_interpolate_result1;

static void savePreds(MappingBin *pred, minijson::object_writer &predW)
{
    predW.write("min",pred->min);
    predW.write("max",pred->max);
    predW.write("func_id",(int)pred->metric.id);
    {
        minijson::array_writer bins = predW.nested_array("bins");
        for(auto b : *(pred->bins)){
            minijson::object_writer bin = bins.nested_object();
            bin.write("binBegin",b->binBegin);
            bin.write("binEnd",b->binEnd);
            bin.write("binRef",b->binRef);
            {
                minijson::array_writer binf = bin.nested_array("binFinalPred");
                for(auto f : b->binFinalPred){
                    binf.write(f);
                }
                binf.close();
            }
            {
                minijson::object_writer next = bin.nested_object("next");
                if(pred->hasNext())
                    savePreds(pred->next(b),next);
                next.close();
            }
            bin.close();
        }
        bins.close();
    }

}

void Predictor::saveToFile(const std::string& filepath)
{
    std::ofstream os(filepath);
    minijson::object_writer writer(os,
            minijson::writer_configuration().pretty_printing(true));

    {
        minijson::object_writer layers = writer.nested_object("layers");
        {
            minijson::array_writer finalmetrics = layers.nested_array("final_metrics");
            for(auto metric : _funcs.final_metric){
                minijson::object_writer fm = finalmetrics.nested_object();
                fm.write("func_id",(int)metric.id);
                fm.write("func_name",*(metric.str));
                fm.close();
            }
            finalmetrics.close();
        }
        {
            minijson::array_writer binningmetrics = layers.nested_array("binning_metrics");
            for(auto metric : _funcs.binning_metrics){
                minijson::object_writer fm = binningmetrics.nested_object();
                fm.write("func_id",(int)metric.metric.id);
                fm.write("func_name",*(metric.metric.str));
                fm.write("max_bins",metric.max_bins);
                fm.close();
            }
            binningmetrics.close();
        }
        layers.close();
    }
    {
        minijson::object_writer predictrs = writer.nested_object("predictors");
        for(auto pred : _predictors){
            std::stringstream ss; ss << pred.first;
            minijson::object_writer predW = predictrs.nested_object(ss.str().c_str());
            savePreds(pred.second,predW);
            predW.close();
        }

        predictrs.close();
    }
    writer.close();
    os.close();
}

static bool loadPreds(MappingBin *pred, minijson::istream_context &ctx)
{
    bool notEmpty = false;
    minijson::parse_object(ctx, [&](const char* name, minijson::value value)
    {
        notEmpty = true;
        if(streq(name, "max")) pred->max = value.as_double();
        if(streq(name, "min")) pred->min = value.as_double();
        if(streq(name, "func_id")) pred->metric = BinFunc((BinFuncID)value.as_long());
        if(streq(name, "bins")){
            minijson::parse_array(ctx, [&](minijson::value value)
            {
                PredBin *bin = new PredBin;
                minijson::parse_object(ctx, [&](const char* name, minijson::value value)
                {
                    if(streq(name, "binBegin")) bin->binBegin = value.as_double();
                    if(streq(name, "binEnd")) bin->binEnd = value.as_double();
                    if(streq(name, "binRef")) bin->binRef = value.as_double();
                    if(streq(name, "binFinalPred"))
                        minijson::parse_array(ctx, [&](minijson::value value)
                        {
                            bin->binFinalPred.push_back(value.as_double());
                        });
                    if(streq(name, "next")){
                        MappingBin *next = new MappingBin;
                        if(loadPreds(next,ctx))
                            pred->nextHierarchy[bin] = next;
                        else
                            delete next;
                    }

                });
                pred->bins->push_back(bin);
            });
        }
    });
    return notEmpty;
}

void Predictor::loadFromFile(const std::string& filepath)
{
    std::ifstream is(filepath);
    minijson::istream_context ctx(is);

    minijson::parse_object(ctx, [&](const char* name, minijson::value value)
    {
        if(streq(name, "layers"))
        minijson::parse_object(ctx, [&](const char* name, minijson::value value)
        {
            if(streq(name, "final_metrics"))
            minijson::parse_array(ctx, [&](minijson::value value)
            {
                minijson::parse_object(ctx, [&](const char* name, minijson::value value)
                {
                    if(streq(name, "func_id")) _funcs.final_metric.push_back(BinFunc((BinFuncID)value.as_long()));
                });
            });

            if(streq(name, "binning_metrics"))
            minijson::parse_array(ctx, [&](minijson::value value)
            {
                LayerConf::PredictionLayer aux{BinFunc((BinFuncID)0),0};
                minijson::parse_object(ctx, [&](const char* name, minijson::value value)
                {
                    if(streq(name, "func_id")) aux.metric = BinFunc((BinFuncID)value.as_long());
                    if(streq(name, "max_bins")) aux.max_bins = value.as_long();
                });
                _funcs.binning_metrics.push_back(aux);
            });
        });

        if(streq(name, "predictors"))
        minijson::parse_object(ctx, [&](const char* name, minijson::value value)
        {
            CoreFreqPair key = coreFreqPairFromStr(name);
            MappingBin *bin = new MappingBin;
            loadPreds(bin,ctx);
            _predictors[key] = bin;
        });
    });

    is.close();

    _make_faster();
}

};

