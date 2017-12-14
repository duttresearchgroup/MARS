
#ifndef __BIN_PRED_COMMON_H
#define __BIN_PRED_COMMON_H

#include <set>
#include "app_common.h"

struct CoreFreq {
    CoreFreq(core_arch_t _a, core_freq_t _f):core(_a),freq(_f){}
    CoreFreq():core(COREARCH_GEM5_BIG_BIG), freq(COREFREQ_1000MHZ){}
    core_arch_t core;
    core_freq_t freq;
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
	core_arch_t src;
    CoreFreq tgt;
    Core_CoreFreq_Pair(core_arch_t _a, CoreFreq _b):src(_a),tgt(_b){}


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
	core_arch_t src;
	core_arch_t tgt;
    CorePair(core_arch_t _a, core_arch_t _b):src(_a),tgt(_b){}


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

typedef std::vector<task_data_t>::iterator siter;

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
    		return first->data.commitedInsts;
    	else
    		return last->curr_instr - first->curr_instr;
    }
};

struct MatchedWindow {
	task_name_t task_name;
    SampleWindow bench1;
    SampleWindow bench2;

    bool operator== (MatchedWindow &other){
        return (bench1 == other.bench1) && (bench2 == other.bench2);
    }
    bool operator!= (MatchedWindow &other){
        return !(*this == other);
    }

    int64_t numOfInstructions() {
        //assert((bench1.last->curr_instr - bench2.first->curr_instr) > 0);
       // assert((bench2.last->curr_instr - bench1.first->curr_instr) > 0);
        return (bench1.numOfInstructions()+bench2.numOfInstructions())/2;
    }
    double numOfInstructionsDev() { return (bench1.numOfInstructions()-bench2.numOfInstructions())/(double)numOfInstructions(); }
    double numOfInstructionsDevStart() {
        return (bench1.first->curr_instr - bench2.first->curr_instr)/(double)numOfInstructions();
    }
    double numOfInstructionsDevEnd() {
        return (bench1.last->curr_instr - bench2.last->curr_instr)/(double)numOfInstructions();
    }
};

struct AveragedMatchedWindow {
    MatchedWindow window;
    task_csv_data_t bench1Avg;
    task_csv_data_t bench2Avg;

    AveragedMatchedWindow(MatchedWindow _w) :window(_w)
    {
        if((window.bench1.first == window.bench1.last) && (window.bench2.first == window.bench2.last)){
            bench1Avg = window.bench1.first->data;
            bench2Avg = window.bench2.first->data;
        }
        else{
            bench1Avg = average_sample(window.bench1.first, window.bench1.last);
            bench2Avg = average_sample(window.bench2.first, window.bench2.last);
        }
    }

    task_csv_data_t average_sample(siter start, siter end);
};

typedef double (*bin_func)(task_csv_data_t &data);

struct Metric {
    bin_func func;
    std::string name;
    bool operator== (const Metric &other) const {
        return (name == other.name);
    }
    bool operator!= (const Metric &other) const {
        return !(*this == other);
    }
    bool operator< (const Metric &other) const {
        return (name < other.name);
    }
};


struct LayerConf {
    struct PredictionLayer {
        Metric metric;
        int max_bins;
    };

    std::vector<Metric> final_metric;
    std::vector<PredictionLayer> binning_metrics;

    void addLayer(int max_bins, bin_func func, std::string name){
        Metric m = {func,name};
        binning_metrics.push_back({m,max_bins});
    }

};

struct Bin {
    double binBegin;
    double binEnd;
    double binRef;
    //all sample between (binBegin,binEnd]
    std::vector<AveragedMatchedWindow> *samples;

    Bin(double _binBegin, double _binEnd, double _binRef) :binBegin(_binBegin),binEnd(_binEnd),binRef(_binRef),samples(nullptr){
        samples = new std::vector<AveragedMatchedWindow>;
        assert(samples != nullptr);
    }
    ~Bin(){
        if(samples != nullptr) delete samples;
    }

    double binAvgSrc(bin_func func){
        double acc = 0;
        for(auto sample : *samples){
            acc += (func)(sample.bench1Avg);
        }
        return acc/samples->size();
    }
    double lin_inter(double x, double x0, double y0, double x1, double y1){
    	double result = y0 + ((y1-y0)*(x-x0))/(x1-x0);
    	return result;
    }
    double binAvgDest(bin_func func){
        double acc = 0;
        for(auto sample : *samples){
            acc += (func)(sample.bench2Avg);
        }
        return acc/samples->size();
    }
    double binInterpDest(double src_val,bin_func func){
        double min_src = std::numeric_limits<double>::max();
        double max_src = 0;
        double min_tgt = std::numeric_limits<double>::max();
        double max_tgt = 0;
        for(auto sample : *samples){
            double val_src = (func)(sample.bench1Avg);
            double val_tgt = (func)(sample.bench2Avg);
            if(val_tgt <= min_tgt){
            	min_tgt = val_tgt;
            	min_src = val_src;
            }
            if(val_tgt >= max_tgt){
            	max_tgt = val_tgt;
            	max_src = val_src;
            }
        }
        double result = lin_inter(src_val,min_src,min_tgt,max_src,max_tgt);
        if(result > 0)  return result;
        else 			return binAvgDest(func);
    }

    double binAvgDest(bin_func func, core_freq_t freq){
        double acc = 0;
        int cnt = 0;
        for(auto sample : *samples){
        	assert(sample.bench2Avg.conf_freq == sample.window.bench2.owner.freq);
        	if(sample.bench2Avg.conf_freq != freq) continue;

            acc += (func)(sample.bench2Avg);
            cnt += 1;
        }
        if(cnt == 0) return 0;
        else 		 return acc/cnt;
    }
};

struct Histogram{
    const int MAX_BINS = 100;

    double min;
    double max;
    bin_func func;
    std::vector<Bin*> bins;

    Histogram(double _min, double _max, int initialSize, bin_func _func);

    void delete_bins()
    {
        for (auto b : bins) delete b;
    }

    void add_sample(AveragedMatchedWindow &s);

    bool empty_bins()
    {
        for (auto b : bins){
            if (b->samples->size() == 0)
                return true;
        }
        return false;
    }
    bool make_bin();
};

struct MappingBin{
    static const int MIN_BIN_CNT = 1;

    double min,max;
    Metric metric;
    std::vector<Bin*> *bins;
    int layer;//set only when the hierarcy is created
    MappingBin* prevLayer;//set only when the hierarcy is created

    std::map<Bin*,MappingBin*> nextHierarchy;

    MappingBin* next(int idx)
    {
        assert(hasNext());
        assert(idx < (int)bins->size());
        return nextHierarchy[(*bins)[idx]];
    }

    bool hasNext() { return nextHierarchy.size() > 0; }

    MappingBin(double _min, double _max, Metric _metric)
      :min(_min),max(_max),metric(_metric),bins(nullptr), layer(-1), prevLayer(nullptr){
        bins = new std::vector<Bin*>;
        assert(bins != nullptr);
    }
    ~MappingBin(){
        for(auto b : *bins) delete b;
        for(auto b : nextHierarchy) delete b.second;
        delete bins;
    }

    std::ostream& print(std::ostream &out){
        return print(out,*this,"");
    }


    static MappingBin* create_bins(std::vector<AveragedMatchedWindow> *samples, LayerConf funcs);

private:

    static bool tryToReduce(int layer, LayerConf &funcs);

    std::ostream& print(std::ostream &out, const MappingBin &bin, std::string ident);

    typedef std::vector<LayerConf::PredictionLayer>::iterator LayerIter;

    MappingBin* _make_hierachy(int layerIdx, LayerIter curr, LayerIter end);

    //always for the src benchmark (bench1)
    static MappingBin* _find_bins(std::vector<AveragedMatchedWindow> *samples, LayerIter curr);
};


struct BinFuncs {
    static double procTimeShare(task_csv_data_t &data);
    static double ipsTotal(task_csv_data_t &data);
    static double ipsActive(task_csv_data_t &data);
    static double ipcActive(task_csv_data_t &data);
    static double powerTotal(task_csv_data_t &data);
    static double powerIdle(task_csv_data_t &data);
    static double powerActive(task_csv_data_t &data);
    static double powerActiveFromIdle(task_csv_data_t &data);
    static double memRate(task_csv_data_t &data);
    static double branchRate(task_csv_data_t &data);
    static double fpRate(task_csv_data_t &data);
    static double brMisspred(task_csv_data_t &data);
    static double brMisspredPerInstr(task_csv_data_t &data);
    static double l1Dmiss(task_csv_data_t &data);
    static double l1DmissPerInstr(task_csv_data_t &data);
    static double l1ImissPerInstr(task_csv_data_t &data);
    static double l1Imiss(task_csv_data_t &data);
    static double dTLBmiss(task_csv_data_t &data);
    static double iTLBmiss(task_csv_data_t &data);
    static double globalL2miss(task_csv_data_t &data);
    static double localL2miss(task_csv_data_t &data);
    static double L2missPerInstr(task_csv_data_t &data);
    static double branchAndCacheSum(task_csv_data_t &data);
    static double branchAndCacheMult(task_csv_data_t &data);
    static double L2L1Dsum(task_csv_data_t &data);
    static double L1DL1Isum(task_csv_data_t &data);
    static double L1DL1IL2sum(task_csv_data_t &data);
    static double L1DL1IL2sum2(task_csv_data_t &data);
    static double L1DL2missPerInstr(task_csv_data_t &data);
    static double ipsWatt(task_csv_data_t &data);
};
#define makeMetric(func) BinFuncs::func,#func



void obtain_windows(
        std::vector<std::string> traceDirs,
        std::set<core_arch_t> archFilter, bool archFilterInclude,
        std::set<core_freq_t> freqFilter, bool freqFilterInclude,
        std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows);

void obtain_windows_single_sample(
        std::vector<std::string> traceDirs,
        std::set<core_arch_t> archFilter, bool archFilterInclude,
        std::set<core_freq_t> freqFilter, bool freqFilterInclude,
        std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows);


void create_predictors(std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows,
        std::map<CoreFreqPair, MappingBin *> &predictors, LayerConf &funcs);

bin_pred_ptr_t gen_cpred(LayerConf funcs, std::map<CoreFreqPair, MappingBin *> &predictors);

void print_predictor(std::map<CoreFreqPair, MappingBin *> &predictors,LayerConf &funcs);

#endif
