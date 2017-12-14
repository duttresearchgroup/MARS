
#include <iostream>
#include <fstream>
#include <cmath>
#include <set>
#include <sys/stat.h>
#include "bin_based_predictor_common.h"

task_csv_data_t AveragedMatchedWindow::average_sample(siter start, siter end)
{
    task_csv_data_t avg;

    avg.conf_arch = start->data.conf_arch;
    avg.conf_freq = start->data.conf_freq;

    avg.ipcActive = 0;
    avg.ipcTotal = 0;
    avg.avgDynPower = 0;
    avg.avgLeakPower = 0;
    avg.gatedSubThrLeakPower = 0;
    avg.gateLeakPower = 0;
    avg.l2TotalAvgPower = 0;
    avg.l2SubThrLeakPower = 0;
    avg.l2GateLeakPower = 0;
    avg.totalPower = 0;
    avg.commitedInsts = 0;
    avg.quiesceCycles = 0;
    avg.idleCycles = 0;
    avg.busyCycles = 0;
    avg.totalActiveCycles = 0;
    avg.commitedMemRefs = 0;
    avg.commitedFPInsts = 0;
    avg.commitedBranches = 0;
    avg.branchMispredicts = 0;
    avg.itlbAccesses = 0;
    avg.itlbMisses = 0;
    avg.dtlbAccesses = 0;
    avg.dtlbMisses = 0;
    avg.iCacheHits = 0;
    avg.iCacheMisses = 0;
    avg.dCacheHits = 0;
    avg.dCacheMisses = 0;
    avg.l2CacheHits = 0;
    avg.l2CacheMisses = 0;

    siter iter = start;
    double cnt = 0;
    while(true){
        avg.ipcActive += iter->data.ipcActive;
        avg.ipcTotal += iter->data.ipcTotal;
        avg.avgDynPower += iter->data.avgDynPower;
        avg.avgLeakPower += iter->data.avgLeakPower;
        avg.gatedSubThrLeakPower += iter->data.gatedSubThrLeakPower;
        avg.gateLeakPower += iter->data.gateLeakPower;
        avg.l2TotalAvgPower += iter->data.l2TotalAvgPower ;
        avg.l2SubThrLeakPower += iter->data.l2SubThrLeakPower ;
        avg.l2GateLeakPower += iter->data.l2GateLeakPower ;
        avg.totalPower += iter->data.totalPower ;
        avg.commitedInsts += iter->data.commitedInsts;
        avg.quiesceCycles  += iter->data.quiesceCycles;
        avg.idleCycles += iter->data.idleCycles;
        avg.busyCycles += iter->data.busyCycles;
        avg.totalActiveCycles += iter->data.totalActiveCycles;
        avg.commitedMemRefs += iter->data.commitedMemRefs ;
        avg.commitedFPInsts += iter->data.commitedFPInsts;
        avg.commitedBranches += iter->data.commitedBranches;
        avg.branchMispredicts += iter->data.branchMispredicts ;
        avg.itlbAccesses += iter->data.itlbAccesses ;
        avg.itlbMisses += iter->data.itlbMisses ;
        avg.dtlbAccesses += iter->data.dtlbAccesses ;
        avg.dtlbMisses += iter->data.dtlbMisses ;
        avg.iCacheHits += iter->data.iCacheHits ;
        avg.iCacheMisses += iter->data.iCacheMisses ;
        avg.dCacheHits += iter->data.dCacheHits ;
        avg.dCacheMisses += iter->data.dCacheMisses  ;
        avg.l2CacheHits += iter->data.l2CacheHits ;
        avg.l2CacheMisses  += iter->data.l2CacheMisses  ;

        ++cnt;
        if(iter == end) break;
        ++iter;
    }
    avg.ipcActive /= cnt;
    avg.ipcTotal /= cnt;
    avg.avgDynPower /= cnt;
    avg.avgLeakPower /= cnt;
    avg.gatedSubThrLeakPower /= cnt;
    avg.gateLeakPower /= cnt;
    avg.l2TotalAvgPower /= cnt;
    avg.l2SubThrLeakPower /= cnt;
    avg.l2GateLeakPower /= cnt;
    avg.totalPower /= cnt;
    avg.commitedInsts /= cnt;
    avg.quiesceCycles /= cnt;
    avg.idleCycles /= cnt;
    avg.busyCycles /= cnt;
    avg.totalActiveCycles /= cnt;
    avg.commitedMemRefs /= cnt;
    avg.commitedFPInsts /= cnt;
    avg.commitedBranches /= cnt;
    avg.branchMispredicts /= cnt;
    avg.itlbAccesses /= cnt;
    avg.itlbMisses /= cnt;
    avg.dtlbAccesses /= cnt;
    avg.dtlbMisses /= cnt;
    avg.iCacheHits /= cnt;
    avg.iCacheMisses /= cnt;
    avg.dCacheHits /= cnt;
    avg.dCacheMisses /= cnt;
    avg.l2CacheHits /= cnt;
    avg.l2CacheMisses /= cnt;


    return avg;
}


siter find_end_time(double duration, std::vector<task_data_t> &bench, siter start)
{
    siter sample = start;
    for(; sample != bench.end(); ++sample){
        if(sample->curr_time - start->curr_time > duration) break;
    }
    if (sample == start) return bench.end();
    return sample;
}

siter find_end_instr(int64_t instr, std::vector<task_data_t> &bench, siter start)
{
    siter sample = start;
    siter samplePrev = start;
    for(; sample != bench.end(); ++sample){
        if(sample->curr_instr > instr){
            if((sample->curr_instr - instr) < (instr - samplePrev->curr_instr)) return sample;
            else return samplePrev;
        }
        samplePrev = sample;
    }
    if (sample == start) return bench.end();
    return sample;
}


int64_t intruction_diff(siter a, siter b){
    if(a->curr_instr >= b->curr_instr) return a->curr_instr - b->curr_instr;
    else return b->curr_instr - a->curr_instr;
}

MatchedWindow
find_best_window(double duration, MatchedWindow prevWindow,
                 std::vector<task_data_t> &bench1, CoreFreq bench1Info,
                 std::vector<task_data_t> &bench2, CoreFreq bench2Info)
{
    prevWindow.bench1.owner = bench1Info;
    prevWindow.bench2.owner = bench2Info;
    siter B1startCandidateLast = prevWindow.bench1.last;
    siter B1startCandidateNext = prevWindow.bench1.last + 1;
    siter B2startCandidateLast = prevWindow.bench2.last;
    siter B2startCandidateNext = prevWindow.bench2.last + 1;

    if(B1startCandidateLast == bench1.end()) return prevWindow;
    if(B1startCandidateNext == bench1.end()) return prevWindow;
    if(B2startCandidateLast == bench2.end()) return prevWindow;
    if(B2startCandidateNext == bench2.end()) return prevWindow;


    MatchedWindow window;
    window.bench1.owner = bench1Info;
    window.bench2.owner = bench2Info;


    int64_t diff = intruction_diff(B1startCandidateLast, B2startCandidateLast);
    window.bench1.first = B1startCandidateLast;
    window.bench2.first = B2startCandidateLast;

    if(intruction_diff(B1startCandidateLast, B2startCandidateNext) <= diff){
        window.bench1.first = B1startCandidateLast;
        window.bench2.first = B2startCandidateNext;
    }

    if(intruction_diff(B1startCandidateNext, B2startCandidateLast) <= diff){
        window.bench1.first = B1startCandidateNext;
        window.bench2.first = B2startCandidateLast;
    }

    if(intruction_diff(B1startCandidateNext, B2startCandidateNext) <= diff){
        window.bench1.first = B1startCandidateNext;
        window.bench2.first = B2startCandidateNext;
    }


    siter B1asMasterEnd = find_end_time(duration, bench1, window.bench1.first);
    siter B2asMasterEnd = find_end_time(duration, bench2, window.bench2.first);

    siter B1asSlaveEnd = find_end_instr(B2asMasterEnd->curr_instr, bench1, window.bench1.first);
    siter B2asSlaveEnd = find_end_instr(B1asMasterEnd->curr_instr, bench2, window.bench2.first);

    if(B1asMasterEnd == bench1.end()) return prevWindow;
    if(B1asSlaveEnd == bench1.end()) return prevWindow;
    if(B2asMasterEnd == bench2.end()) return prevWindow;
    if(B2asSlaveEnd == bench2.end()) return prevWindow;


    diff = intruction_diff(B1asMasterEnd, B2asMasterEnd);
    window.bench1.last = B1asMasterEnd;
    window.bench2.last = B2asMasterEnd;


    if(intruction_diff(B1asMasterEnd, B2asSlaveEnd) <= diff){
        window.bench1.last = B1asMasterEnd;
        window.bench2.last = B2asSlaveEnd;
    }

    if(intruction_diff(B1asSlaveEnd, B2asMasterEnd) <= diff){
        window.bench1.last = B1asSlaveEnd;
        window.bench2.last = B2asMasterEnd;
    }

    if(intruction_diff(B1asSlaveEnd, B2asSlaveEnd) <= diff){
        window.bench1.last = B1asSlaveEnd;
        window.bench2.last = B2asSlaveEnd;
    }

    return window;

}

static inline bool not_idle(SampleWindow &w){
	return (w.first->data.ipcActive >= w.first->data.ipcTotal) &&
		   ((w.first->data.ipcTotal / w.first->data.ipcActive) >= 0.95) &&
		   (w.last->data.ipcActive >= w.last->data.ipcTotal) &&
		   ((w.last->data.ipcTotal / w.last->data.ipcActive) >= 0.95);
}

bool accept_window(MatchedWindow &w){
    return (std::fabs(w.numOfInstructionsDev()) < 0.06) &&
            (std::fabs(w.numOfInstructionsDevStart()) < 0.06) &&
            not_idle(w.bench1) && not_idle(w.bench2) &&
            (std::fabs(w.numOfInstructionsDevEnd()) < 0.06);
}

bool _window_min_sample_cnt(std::vector<task_data_t> *samples, double duration){
    if(samples->empty())
        return true;
    else
        return (samples->back().curr_time - samples->front().curr_time) < (duration*2);
}

void window(std::vector<MatchedWindow> &windows, task_name_t task,
            core_arch_t srcArc, core_freq_t srcFreq, std::vector<task_data_t> *srcSamples,
            core_arch_t tgtArc, core_freq_t tgtFreq, std::vector<task_data_t> *tgtSamples)
{
    //std::cout << "Windows for " << archToString(srcArc) << "_" << freqToString(srcFreq)
    //          << " -> " << archToString(tgtArc) << "_" << freqToString(tgtFreq) << "\n";

    double duration = 15/1000.0;

    if(_window_min_sample_cnt(srcSamples,duration)) return;
    if(_window_min_sample_cnt(tgtSamples,duration)) return;

    MatchedWindow prev;
    prev.bench1.last = srcSamples->begin();
    prev.bench2.last = tgtSamples->begin();

    MatchedWindow next = find_best_window(duration, prev,
               *srcSamples, CoreFreq(srcArc,srcFreq),
               *tgtSamples, CoreFreq(tgtArc,tgtFreq));

    while(next != prev){
        if(accept_window(next)){
            /*std::cout << "\t" << next.bench1.first->curr_instr << "\n";
            std::cout << "\t" << next.bench2.first->curr_instr << "\n";
            std::cout << "\t\t" << next.numOfInstructions() << " / " << next.numOfInstructionsDev() << " / " << next.numOfInstructionsDevStart() << " / " << next.numOfInstructionsDevEnd() << "\n";
            std::cout << "\t" << next.bench1.last->curr_instr << "\n";
            std::cout << "\t" << next.bench2.last->curr_instr << "\n\n\n";*/
            next.task_name = task;
        	windows.push_back(next);
        }

        prev = next;
        next = find_best_window(duration, prev,
                       *srcSamples, CoreFreq(srcArc,srcFreq),
                       *tgtSamples, CoreFreq(tgtArc,tgtFreq));
    }
}

bool window_single_sample(std::vector<MatchedWindow> &windows, task_name_t task,
            core_arch_t srcArc, core_freq_t srcFreq, std::vector<task_data_t> *srcSamples,
            core_arch_t tgtArc, core_freq_t tgtFreq, std::vector<task_data_t> *tgtSamples)
{
    if(srcSamples->size() != 1) return false;
    if(tgtSamples->size() != 1) return false;

    MatchedWindow window;
    window.task_name = task;
    window.bench1.owner = CoreFreq(srcArc,srcFreq);
    window.bench1.first = srcSamples->begin();
    window.bench1.last = srcSamples->begin();
    window.bench2.owner = CoreFreq(tgtArc,tgtFreq);
    window.bench2.first = tgtSamples->begin();
    window.bench2.last = tgtSamples->begin();

    if(std::fabs(window.numOfInstructionsDev()) > 0.05) return false;

    //TODO workaround. drop is ipc is > in a slower core
    if((tgtArc > srcArc) && (window.bench2.first->data.ipcActive > window.bench1.first->data.ipcActive)) return false;

    windows.push_back(window);
    return true;
}

std::ostream& operator<<(std::ostream &out, const CoreFreq &corefreq){
    out << archToString(corefreq.core) << "_" << freqToString(corefreq.freq);
    return out;
}
std::ostream& operator<<(std::ostream &out, const CoreFreqPair &corefreq){
    out << corefreq.src << " -> " << corefreq.tgt;
    return out;
}
std::ostream& operator<<(std::ostream &out, const Core_CoreFreq_Pair &corefreq){
    out << archToString(corefreq.src) << " -> " << corefreq.tgt;
    return out;
}
std::ostream& operator<<(std::ostream &out, const CorePair &corefreq){
    out << archToString(corefreq.src) << " -> " << archToString(corefreq.tgt);
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


std::ostream& operator<<(std::ostream &out, const Bin &bin){
    out << "(from " << bin.binBegin << " to " << bin.binEnd << " " <<  bin.samples->size() << " samples)";
    return out;
}



Histogram::Histogram(double _min, double _max, int initialSize, bin_func _func)
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
        if(binIdx == 1) _binStart = -1;
        if(binIdx == initialSize) _binEnd = std::numeric_limits<double>::max();
        bins.push_back(new Bin(_binStart,_binEnd,_binRef));
    }
}

void Histogram::add_sample(AveragedMatchedWindow &s){
    bool ok = false;
    double val = (func)(s.bench1Avg);
    for(auto bin : bins){
        if(val <= bin->binEnd){
            bin->samples->push_back(s);
            ok = true;
            assert(val >= bin->binBegin);
            break;
        }
    }
    if(!ok){
        std::cerr << "Failed to bin sample with val=" << val << " at bins = \n";
        throw 0;
    }
}


bool Histogram::make_bin()
{
    //it is invalid to call if we have empty bins
    //if size is 1, we know its impossible to break up
    if(empty_bins() || (bins.size()==1)) return false;

    //std::cerr << "Make bin\n";
    auto maxBin = bins.size();
    unsigned int nsamples = 0;
    for(unsigned int i = 0 ; i < bins.size(); ++i){
        unsigned int aux = bins[i]->samples->size();
        if((aux >= 2) && (aux >= nsamples)){
            nsamples = aux;
            maxBin = i;
        }
    }
    if(maxBin == bins.size()) return false;

    //std::cerr << "Make bin will split bin "<<maxBin<< "\n";
    //std::cerr << "Bins before:\n";
    //for(auto b : bins){
    //    std::cerr << *b << "\n";
    //}

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
    if(maxBin == 0) newBinBegin = -1;


    //bkp the original bin
    Bin *oldBinMod = bins[maxBin];
    Bin oldBin = *oldBinMod;

    //std::cerr << "Svector in origBin:" << oldBin.samples << "\n";


    //give a new set of samples to bins[maxBin] and update its bounds
    oldBinMod->samples = new std::vector<AveragedMatchedWindow>;
    oldBinMod->binBegin = oldBinBegin;
    oldBinMod->binEnd = oldBinEnd;
    oldBinMod->binRef = oldBinRef;

    //std::cerr << "new Svector in origBin:" << oldBinMod->samples << "\n";

    //insert a new bin before
    Bin *newBin = new Bin(newBinBegin,newBinEnd,newBinRef);
    auto insertPoint = bins.insert(bins.begin()+maxBin,newBin);

    //std::cerr << "new Svector in newBin:" << newBin->samples << "\n";

    //reinsert the samples
    for(auto s : *(oldBin.samples)) add_sample(s);

    //both bin must have samples
    if(empty_bins()){
        //std::cerr << "Failed, reverting\n";
        //revert and return false

        delete oldBinMod->samples;
        *oldBinMod = oldBin;
        oldBin.samples = nullptr;//avoid sample deletion when oldBin goes out of scope

        bins.erase(insertPoint);
        delete newBin;

        //std::cerr << "Bins after:\n";
        //for(auto b : bins){
        //    std::cerr << *b << "\n";
        //}

        return false;
    }

    //all fine; delete the old sample vector
    delete oldBin.samples;
    //std::cerr << "deleted Svector in oldBin:" << oldBin.samples << "\n";
    oldBin.samples = nullptr;

    //std::cerr << "Bins after:\n";
    //for(auto b : bins){
    //    std::cerr << *b << "\n";
    //}

    return true;
}


MappingBin* MappingBin::create_bins(std::vector<AveragedMatchedWindow> *samples, LayerConf funcs){
    MappingBin* firstBin = _find_bins(samples,funcs.binning_metrics.begin());
    if(firstBin != nullptr){
        MappingBin *ret = firstBin->_make_hierachy(0, funcs.binning_metrics.begin()+1, funcs.binning_metrics.end());
        if(ret != 0){
            std::cerr << "_make_hierachy failed at layer "<<ret->layer<<"!\n";
            std::cerr << "layer " << ret->layer << " has " << ret->bins->size() << " bins\n";
            std::cerr << "reducing and try again\n";
            if(tryToReduce(ret->layer, funcs)){
                delete firstBin;
                return create_bins(samples,funcs);
            }
            else {
                std::cerr << "cannot reduce further. It failed!\n";
                delete firstBin;
                return nullptr;
            }
        }
    }
    else{
        std::cerr << "Unnable to create first level bins !\n";
    }
    return firstBin;
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
    out << ident << "(min=" << bin.min << ",max=" << bin.max << ", metric=" << metric.name << ", samples=[\n";
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

    for(auto b : *bins){
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
    double max = -1;

    //find min/max
    //std::cerr << "Svector:" << samples << "\n";
    for(auto s : *samples){
        double val = (curr->metric.func)(s.bench1Avg);
        assert(val>=0);
        if(val < min) min = val;
        if(val > max) max = val;
    }

    //find the largest number of bins such that each bin has
    //at least one sample
    Histogram *currHistogram = nullptr;
    for(int currBinCnt = MIN_BIN_CNT; currBinCnt <= curr->max_bins; ++currBinCnt){
        Histogram *hist = new Histogram(min,max,currBinCnt,curr->metric.func);

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

    //std::cerr << "#### Start bin spliting. numbins = "<< currHistogram->bins.size() <<"\n";
    for(int currBinCnt = currHistogram->bins.size(); currBinCnt < curr->max_bins; ++currBinCnt){
        if(!currHistogram->make_bin()) break;
        //std::cerr << "#### Split successfull. numbins = "<< currHistogram->bins.size() <<"\n";
    }
    assert(!currHistogram->empty_bins());

    MappingBin *mappingBins = new MappingBin(min,max,curr->metric);
    for (auto b : currHistogram->bins) mappingBins->bins->push_back(b);

    delete currHistogram;

    return mappingBins;
}


void print_sumup_windows(std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows){
    for(auto corefreq : averagedWindows){
        std::cout << "Windows for " << corefreq.first << "\n";
        for(auto w : *(corefreq.second)){
            std::cout << "\t" << w.window.bench1.first->curr_instr << " --- " << w.window.bench1.last->curr_instr
                      << "\n";//" AVG IPS: " << perf_metric(w.bench1Avg)  << "   AVG d$ miss:" << mem_metric(w.bench1Avg)  <<  "\n";
            std::cout << "\t" << w.window.bench2.first->curr_instr << " --- " << w.window.bench2.last->curr_instr
                    << "\n";//<< " AVG IPS: " << perf_metric(w.bench2Avg)  << "   AVG d$ miss:" << mem_metric(w.bench2Avg)  <<  "\n";
            std::cout << "\n";
        }
    }
}


double bin_based_predict(LayerConf funcs, Metric finalMetric,
                         MappingBin *predictor,
                         AveragedMatchedWindow window){
    //double srcIPC = (finalMetric.func)(window.bench1Avg);
    double tgtIPC = (finalMetric.func)(window.bench2Avg);
    double predIPC = -1234;

    MappingBin *currLevel = predictor;
    unsigned i = 0;
    for(i = 0; i < funcs.binning_metrics.size(); ++i){
        double val = (funcs.binning_metrics[i].metric.func)(window.bench1Avg);

        Bin *currBin = nullptr;
        for(auto bin : *(currLevel->bins)){
            if(val <= bin->binEnd){
                currBin = bin;
                break;
            }
        }
        assert(currBin != nullptr);

        //last level; time to predict
        if(!(currLevel->hasNext())){
            //this must be the last iteration
            assert((i+1) == funcs.binning_metrics.size());
            predIPC = currBin->binAvgDest(finalMetric.func);
            //predIPC = currBin->binInterpDest(srcIPC,finalMetric.func);
            break;
        }
        assert(currLevel->nextHierarchy.find(currBin) != currLevel->nextHierarchy.end());
        currLevel = currLevel->nextHierarchy[currBin];
    }
    assert(i != funcs.binning_metrics.size());

    double error = 1 - (tgtIPC/predIPC);
    //if(std::fabs(error*100) > 100) {
    //	std::cout << " tgtIPC= " << tgtIPC
    //			<< " predIPC= " << predIPC
    //			<< " error=" << error << "\n";
    //	abort();
    //}
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
     max(std::numeric_limits<double>::min())
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

AvgError bin_based_predict(LayerConf funcs, Metric finalMetric,
                         MappingBin *predictor,
                         std::vector<AveragedMatchedWindow>* averagedWindows){
    AvgError error;
    for(auto sample : *averagedWindows){
    	error.add(std::fabs(bin_based_predict(funcs, finalMetric, predictor, sample)));
    }
    return error;
}

std::vector<AvgError> bin_based_predict(LayerConf funcs,
                         std::map<CoreFreqPair, MappingBin *> &predictors,
                         std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows){
    std::vector<AvgError> overall;
    for(unsigned i = 0; i < funcs.final_metric.size(); ++i) overall.push_back(AvgError());
    for(auto corefreq : averagedWindows){
        std::cout << "Predicting for " << corefreq.first << "\n";
        assert(funcs.final_metric.size() == 2);
        for(unsigned i = 0; i < funcs.final_metric.size(); ++i){
        	AvgError avgError = bin_based_predict(funcs,funcs.final_metric[i],predictors[corefreq.first],corefreq.second);
        	std::cout << "\tAvg error for "<<funcs.final_metric[i].name<<"= " << avgError.avg() * 100 << " %  " << " StDev = " << avgError.stddev()*100 << "% Min = " << avgError.min*100 << "% Max = " << avgError.max*100 << "%\n";
        	overall[i].add(avgError);
        }
    }
    return overall;
}

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

void print_pred_info(LayerConf funcs,
                     std::map<CoreFreqPair, MappingBin *> &predictors){
    std::cout << "Predictor info:\n";
    std::cout << "\tnum layers = "<<funcs.binning_metrics.size()<<"\n";
    for(unsigned i = 0; i < funcs.binning_metrics.size(); ++i){
        std::cout << "\tLayer " << i << " number of bins\n";
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
        std::cout << "\t\tavg " << avg/acc << "\n";
        std::cout << "\t\tmin " << min << "\n";
        std::cout << "\t\tminPair " << minPair << "\n";
        std::cout << "\t\tmax " << max << "\n";
    }
    std::cout << std::endl;

}


static bin_pred_func_id get_func_id(bin_func f)
{
    if      (f == BinFuncs::ipcActive)            return BIN_PRED_FUNC_ID_ipcActive;
    else if (f == BinFuncs::L1DL1IL2sum)          return BIN_PRED_FUNC_ID_sumL1ILIDL2misses;
    else if (f == BinFuncs::l1DmissPerInstr)    return BIN_PRED_FUNC_ID_LIDmissesPerInstr;
    else if (f == BinFuncs::L2missPerInstr)    return BIN_PRED_FUNC_ID_L2missesPerInstr;
    else if (f == BinFuncs::L1DL2missPerInstr)    return BIN_PRED_FUNC_ID_sumLIDL2missesPerInstr;
    else if (f == BinFuncs::brMisspredPerInstr)   return BIN_PRED_FUNC_ID_brMisspredrate;
    else if (f == BinFuncs::procTimeShare)        return BIN_PRED_FUNC_ID_procTimeShare;
    else if (f == BinFuncs::powerActive)        return BIN_PRED_FUNC_ID_powerActive;
    else{
        assert("Wrong metric"&&false);
        return SIZE_BIN_PRED_FUNC_ID;
    }
}



bin_pred_ptr_t gen_cpred(LayerConf funcs, std::map<CoreFreqPair, MappingBin *> &predictors);
void gen_cpred_final(bin_pred_layer_t *layer, LayerConf funcs,MappingBin *curr_predictor);
void gen_cpred_aux(bin_pred_layer_t *layer, LayerConf funcs, MappingBin *currLayer);


template<typename FilterType, typename ElemType>
inline bool filter_helper(FilterType &filter, ElemType &elem, bool include){
    if(include)
        return !filter.empty() && (filter.find(elem) == filter.end());
    else
        return !filter.empty() && (filter.find(elem) != filter.end());
}

void obtain_windows(
        std::vector<std::string> traceDirs,
        std::set<core_arch_t> archFilter, bool archFilterInclude,
        std::set<core_freq_t> freqFilter, bool freqFilterInclude,
        std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows){

    input_data_t *inputs = parse_csvs(traceDirs);


    for(auto task : inputs->samples){
        //std::cout << "Finding windows for " << task.first << "\n";

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
                        		archTgt.first, freqTgt.first, freqTgt.second);
                    }
                }
            }
        }
        //std::cout << "\t Obtained " << windows.size() << " samples\n";
        sumup_windows(task.first,windows,averagedWindows);
    }

    delete inputs;
}

void obtain_windows_single_sample(
        std::vector<std::string> traceDirs,
        std::set<core_arch_t> archFilter, bool archFilterInclude,
        std::set<core_freq_t> freqFilter, bool freqFilterInclude,
        std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows){

    input_data_t *inputs = parse_csvs(traceDirs);
    //check_inputs(*inputs);

    int totalSamples = 0;

    for(auto task : inputs->samples){
        //std::cout << "Finding windows for " << task.first << "\n";

        std::vector<MatchedWindow> windows;

        for(auto archSrc : task.second){
            if(filter_helper(archFilter,archSrc.first,archFilterInclude)) continue;

            for(auto freqSrc : archSrc.second){
                if(filter_helper(freqFilter,freqSrc.first,freqFilterInclude)) continue;

                for(auto archTgt : task.second){
                    if(filter_helper(archFilter,archTgt.first,archFilterInclude)) continue;

                    for(auto freqTgt : archTgt.second){
                        if(filter_helper(freqFilter,freqTgt.first,freqFilterInclude)) continue;

                        if(window_single_sample(windows, task.first,
                        		archSrc.first, freqSrc.first, freqSrc.second,
                        		archTgt.first, freqTgt.first, freqTgt.second))
                        	totalSamples += 1;
                    }
                }
            }
        }
        //std::cout << "\t Obtained " << windows.size() << " samples\n";
        sumup_windows(task.first,windows,averagedWindows);
    }
    std::cout << "Obtained a total of " << totalSamples << " samples\n";

    delete inputs;
}


void create_predictors(std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows,
        std::map<CoreFreqPair, MappingBin *> &predictors, LayerConf &funcs)
{
    for(auto corefreq : averagedWindows){
        //std::cout << "\nFinding bins " << archToString(corefreq.first.src.core) << "_" << freqToString(corefreq.first.src.freq)
        //                  << " -> " << archToString(corefreq.first.tgt.core) << "_" << freqToString(corefreq.first.tgt.freq) << "\n";
        MappingBin *bins = MappingBin::create_bins(corefreq.second, funcs);
        assert(bins != nullptr);
        predictors[corefreq.first] = bins;

        //bins->print(std::cout) << "\n";
    }

    std::vector<AvgError> errors = bin_based_predict(funcs, predictors, averagedWindows);
    for (auto err : errors){
        std::cout << "Overall avg error = " << err.avg() * 100 << " %  " << " StDev = " << err.avgStddev()*100 << "% Min = " << err.min*100 << "% Max = " << err.max*100 << "%\n";
    }
    print_pred_info(funcs, predictors);
}

/*
void predictor_example()
{
    std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> averagedWindows;
    obtain_windows(averagedWindows);


    std::map<CoreFreqPair, MappingBin *> predictorsIPCPower;
    LayerConf funcsIPCPower;
    //funcsIPCPower.final_metric.push_back({makeMetric(ipcActive)});
    funcsIPCPower.final_metric.push_back({makeMetric(ipsActive)});
    funcsIPCPower.final_metric.push_back({makeMetric(powerActive)});
    funcsIPCPower.final_metric.push_back({makeMetric(procTimeShare)});
    funcsIPCPower.final_metric.push_back({makeMetric(ipsWatt)});
    funcsIPCPower.addLayer(4, makeMetric(L1DL1IL2sum2));
    funcsIPCPower.addLayer(4, makeMetric(brMisspredPerInstr));
    //funcsIPCPower.addLayer(100, makeMetric(ipcActive));
    funcsIPCPower.addLayer(3, makeMetric(ipsActive));
    create_predictors(averagedWindows,predictorsIPCPower,funcsIPCPower);

    MappingBin* tmp = predictorsIPCPower[CoreFreqPair(CoreFreq(COREARCH_GEM5_BIG_LITTLE,COREFREQ_2000MHZ),CoreFreq(COREARCH_GEM5_LITTLE_BIG,COREFREQ_2000MHZ))];
    tmp->print(std::cout);
    Bin* tmp2 = (*(tmp->next(1)->next(0)->bins))[1];
    auto sample = tmp2->samples->begin()->bench1Avg;
    std::cout << "sample.l1ImissPerInstr " << (l1ImissPerInstr)(sample) << "\n";
    std::cout << "sample.l1DmissPerInstr " << (l1DmissPerInstr)(sample) << "\n";
    std::cout << "sample.L2missPerInstr " << (L2missPerInstr)(sample) << "\n";
    std::cout << "sample.ipsActive " << (ipsActive)(sample) << "\n";
    std::cout << "sample.brMisspredPerInstr " << (brMisspredPerInstr)(sample) << "\n";
    for(auto xx : funcsIPCPower.final_metric){
        std::cout << "func " << xx.name << ": " << tmp2->binAvgDest(xx.func) << "\n";
    }
}
*/

bin_pred_ptr_t gen_cpred(LayerConf funcs, std::map<CoreFreqPair, MappingBin *> &predictors)
{
    bin_pred_ptr_t cpred = vitamins_bin_predictor_alloc_new();

    for(auto pairs : predictors){
    	CoreFreqPair coreFreqPair = pairs.first;
        bin_pred_layer_t *layer = new bin_pred_layer_t;
        gen_cpred_aux(layer,funcs, predictors[coreFreqPair]);
        cpred[coreFreqPair.src.core][coreFreqPair.src.freq][coreFreqPair.tgt.core][coreFreqPair.tgt.freq] = layer;
    }

    return cpred;
}

void gen_cpred_aux(bin_pred_layer_t *layer, LayerConf funcs, MappingBin *currLayer)
{
    gen_cpred_final(layer,funcs,currLayer);

    if(currLayer->hasNext()){
        int binIdx = 0;
        for(auto b : *(currLayer->bins)){
            layer->next_layer[binIdx] = new bin_pred_layer_t;
            gen_cpred_aux(layer->next_layer[binIdx],
                    funcs,
                    currLayer->nextHierarchy[b]);
            binIdx += 1;
        }
    }
}

void gen_cpred_final(bin_pred_layer_t *layer, LayerConf funcs,MappingBin *curr_predictor)
{
    layer->metric = vitamins_get_bin_pred_func(get_func_id(curr_predictor->metric.func));
    layer->num_of_bins = curr_predictor->bins->size();
    layer->bins = new uint32_t[curr_predictor->bins->size()];
    for(unsigned i = 0; i < curr_predictor->bins->size(); ++i){
        if(curr_predictor->bins->at(i)->binEnd > curr_predictor->max)
            layer->bins[i] = UINT32_MAX;
        else
            layer->bins[i] = CONV_DOUBLE_scaledUINT32(curr_predictor->bins->at(i)->binEnd);

    }
    if(!(curr_predictor->hasNext())){
        layer->bin_result = new bin_pred_result_t[curr_predictor->bins->size()];
        layer->next_layer = nullptr;
        for(unsigned i = 0; i < curr_predictor->bins->size(); ++i){
            //TODO this part is a workaround and should be generalized
            //maybe by generalizing or creating a mapping between the C/C++ functions used for binning
            //we always generate the result for both predictors assuming ipc is metric 0 and power is metric 1
            assert(funcs.final_metric.size() == 2);

            //std::cout << curr_predictor << " " <<  curr_predictor->bins << " " << curr_predictor->bins->at(i) << " " << funcs.final_metric[0].func << "\n";

            double ipcActive = curr_predictor->bins->at(i)->binAvgDest(funcs.final_metric[0].func);
            double activePower = curr_predictor->bins->at(i)->binAvgDest(funcs.final_metric[1].func);
            layer->bin_result[i].ipcActive=CONV_DOUBLE_scaledUINT32(ipcActive);
            layer->bin_result[i].powerActive=CONV_DOUBLE_scaledUINT32(activePower);
        }

    }
    else{
        layer->bin_result=nullptr;
        layer->next_layer = new bin_pred_layer_t*[curr_predictor->bins->size()];
        for(unsigned i = 0; i < curr_predictor->bins->size(); ++i){
            layer->next_layer[i]=nullptr;
        }
    }
}


double BinFuncs::procTimeShare(task_csv_data_t &data){return (data.busyCycles+data.idleCycles)/(double)(data.busyCycles+data.idleCycles+data.quiesceCycles);};
double BinFuncs::ipsTotal(task_csv_data_t &data){return data.ipcTotal*freqToValMHz_d(data.conf_freq);};
double BinFuncs::ipsActive(task_csv_data_t &data){return data.ipcActive*freqToValMHz_d(data.conf_freq);};
double BinFuncs::ipcActive(task_csv_data_t &data){return data.ipcActive;};
double BinFuncs::powerTotal(task_csv_data_t &data){return data.avgDynPower+data.avgLeakPower+data.l2TotalAvgPower;};
double BinFuncs::powerIdle(task_csv_data_t &data){return data.gateLeakPower+data.gatedSubThrLeakPower+data.l2GateLeakPower+data.l2SubThrLeakPower;};
double BinFuncs::powerActive(task_csv_data_t &data) {return (((powerTotal)(data)) - (((powerIdle)(data)) * (1-((procTimeShare)(data)))))/((procTimeShare)(data));};

double BinFuncs::powerActiveFromIdle(task_csv_data_t &data) {
	//available for the exynos SOC. totalPower here is cluster power
	//so we get the total core power by deducting the idle power of other cores
	//we assume that idle power is 0 so this is the same as active power
	assert((data.conf_arch == COREARCH_Exynos5422_BIG) || (data.conf_arch == COREARCH_Exynos5422_LITTLE));
	return data.totalPower-(CONV_scaledINTany_DOUBLE(arch_idle_power_scaled(data.conf_arch,data.conf_freq))*3);
};

double BinFuncs::memRate(task_csv_data_t &data){return data.commitedMemRefs/(double)data.commitedInsts;};
double BinFuncs::branchRate(task_csv_data_t &data){return data.commitedBranches/(double)data.commitedInsts;};
double BinFuncs::fpRate(task_csv_data_t &data){return data.commitedFPInsts/(double)data.commitedInsts;};
double BinFuncs::brMisspred(task_csv_data_t &data){return data.branchMispredicts/(double)data.commitedBranches;};
double BinFuncs::brMisspredPerInstr(task_csv_data_t &data){return data.branchMispredicts/(double)data.commitedInsts;};
double BinFuncs::l1Dmiss(task_csv_data_t &data){return data.dCacheMisses/(double)(data.dCacheMisses+data.dCacheHits);};
double BinFuncs::l1DmissPerInstr(task_csv_data_t &data){return data.dCacheMisses/(double)(data.commitedInsts);};
double BinFuncs::l1ImissPerInstr(task_csv_data_t &data){return data.iCacheMisses/(double)(data.commitedInsts);};
double BinFuncs::l1Imiss(task_csv_data_t &data){return data.iCacheMisses/(double)(data.iCacheMisses+data.iCacheHits);};
double BinFuncs::dTLBmiss(task_csv_data_t &data){return data.dtlbMisses/(double)data.dtlbAccesses;};
double BinFuncs::iTLBmiss(task_csv_data_t &data){return data.itlbMisses/(double)data.itlbAccesses;};
double BinFuncs::globalL2miss(task_csv_data_t &data){return data.l2CacheMisses / (double) data.commitedMemRefs;};
double BinFuncs::localL2miss(task_csv_data_t &data){return data.l2CacheMisses / (double) (data.l2CacheHits + data.l2CacheMisses);};
double BinFuncs::L2missPerInstr(task_csv_data_t &data){return data.l2CacheMisses / (double) data.commitedInsts;};
double BinFuncs::branchAndCacheSum(task_csv_data_t &data){return (brMisspredPerInstr)(data) + (L2missPerInstr)(data);};
double BinFuncs::branchAndCacheMult(task_csv_data_t &data){return (brMisspredPerInstr)(data) * (L2missPerInstr)(data);};
double BinFuncs::L2L1Dsum(task_csv_data_t &data){return (L2missPerInstr)(data) + (l1Dmiss)(data);};
double BinFuncs::L1DL1Isum(task_csv_data_t &data){return (l1Dmiss)(data)+(l1Imiss)(data);};
double BinFuncs::L1DL1IL2sum(task_csv_data_t &data){return (l1Dmiss)(data)+(l1Imiss)(data) + (L2missPerInstr)(data);};
double BinFuncs::L1DL1IL2sum2(task_csv_data_t &data){return (l1DmissPerInstr)(data)+(l1ImissPerInstr)(data) + (L2missPerInstr)(data);};
double BinFuncs::L1DL2missPerInstr(task_csv_data_t &data){return (l1DmissPerInstr)(data) + (L2missPerInstr)(data);};
double BinFuncs::ipsWatt(task_csv_data_t &data){return (ipsActive)(data)/(powerActive)(data);};


static inline void add_level(int level, std::ostream &os){
	for(int i = 0; i < level; ++i) os << "  ";
}

static void print_layer(LayerConf &funcs, MappingBin *layer, int level)
{
	for(unsigned int i = 0; i < layer->bins->size(); ++i){

		uint32_t binEndInt = CONV_DOUBLE_scaledUINT32(layer->bins->at(i)->binEnd);
		if(layer->bins->at(i)->binEnd >= (std::numeric_limits<double>::max()-1))
			binEndInt = UINT32_MAX;

		add_level(level,std::cout);
		std::cout << "bin[" << i << "]: "
				<< vitamins_get_bin_pred_func_name(get_func_id(layer->metric.func))
				<< " < " <<  binEndInt << "\n";//" (" << layer->bins->at(i)->binEnd << ")\n";

		if(layer->hasNext()){
			print_layer(funcs,layer->next(i),level+1);
		}
		else{

			//TODO this part is a workaround and should be generalized
			//maybe by generalizing or creating a mapping between the C/C++ functions used for binning
			//we always generate the result for both predictors assuming ipc is metric 0 and power is metric 1
			assert(funcs.final_metric.size() == 2);

			//add_level(level+1,std::cout);
			//std::cout << layer << " " <<  layer->bins << " " << layer->bins->at(i) << " " << funcs.final_metric[0].func << "\n";

			double ipcActive = layer->bins->at(i)->binAvgDest(funcs.final_metric[0].func);
			double activePower = layer->bins->at(i)->binAvgDest(funcs.final_metric[1].func);

			add_level(level+1,std::cout);
			std::cout << "result: " << vitamins_get_bin_pred_func_name(get_func_id(funcs.final_metric[0].func))
							  << "=" << CONV_DOUBLE_scaledUINT32(ipcActive) << "\n";// " (" << ipcActive <<  ")\n";
			add_level(level+1,std::cout);
			std::cout << "result: " << vitamins_get_bin_pred_func_name(get_func_id(funcs.final_metric[1].func))
							  << "=" << CONV_DOUBLE_scaledUINT32(activePower) << "\n";// " (" << activePower <<  ")\n";


			if(activePower > 50){
				add_level(level+1,std::cout);
				std::cout << "Bin has bug. Dumping "<<layer->bins->at(i)->samples->size()<<" samples:\n";

				for(auto sample : *(layer->bins->at(i)->samples)){
					add_level(level+2,std::cout);
					std::cout << sample.window.task_name << " "
					          << sample.window.bench1.owner << "(" << sample.window.bench1.first->curr_time_original << "-" << sample.window.bench1.last->curr_time_original << ")"
					          << " -> "
					          << sample.window.bench2.owner << "(instr " << sample.window.bench2.first->curr_time_original << "-" << sample.window.bench2.last->curr_time_original << ")\n";
					for(auto binMetric : funcs.binning_metrics){
						auto func = binMetric.metric.func;
						add_level(level+3,std::cout); std::cout << vitamins_get_bin_pred_func_name(get_func_id(func)) << ": "
						<< func(sample.bench1Avg) << " -> " << func(sample.bench2Avg) << "\n";
					}
					auto func = funcs.final_metric[1].func;
					add_level(level+3,std::cout); std::cout << vitamins_get_bin_pred_func_name(get_func_id(func)) << ": "
					<< func(sample.bench1Avg) << " -> " << func(sample.bench2Avg) << "\n";
				}
			}
		}
	}


}

void print_predictor(std::map<CoreFreqPair, MappingBin *> &predictors, LayerConf &funcs){
	for(auto corePair : predictors){
		std::cout << "Pred " << corePair.first << "\n";
		print_layer(funcs,corePair.second,1);
	}
}

