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

#ifndef __arm_rt_models_binbased_h
#define __arm_rt_models_binbased_h

#include "bin_based_predictor_defs.h"
#include <unordered_set>

namespace BinBasedPred {

struct TrainingData {
    std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> averagedWindows;

    TrainingData(const TraceParser::Traces& traces,
            std::set<ArchName> archFilter, bool archFilterInclude,
            std::set<FrequencyMHz> freqFilter, bool freqFilterInclude,
            int minWindowSize=1, int maxWindowSize=0)
    {
        obtain_windows(traces,
                archFilter,archFilterInclude,freqFilter,freqFilterInclude,
                averagedWindows,
                minWindowSize,maxWindowSize);
    }
    TrainingData(const TraceParser::Traces& traces,
                int minWindowSize=1, int maxWindowSize=0)
    {
            obtain_windows(traces,
                    {},false,{},false,
                    averagedWindows,
                    minWindowSize,maxWindowSize);
    }

    ~TrainingData()
    {
        for(auto i : averagedWindows){
            delete i.second;
        }
    }

  private:
    static void obtain_windows(
            const TraceParser::Traces& traces,
            std::set<ArchName> archFilter, bool archFilterInclude,
            std::set<FrequencyMHz> freqFilter, bool freqFilterInclude,
            std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows,
            int minWindowSize=1, int maxWindowSize=0);
};

class Predictor : SensingInterfaceImpl {
  private:
    const sys_info_t *_sys_info;//needs to be set for runtime predictions, may be null otherwise
    LayerConf _funcs;
    std::map<CoreFreqPair, MappingBin *> _predictors;

    // This is a faster representation for runtime predictions
    // (we do not need to create the CoreFreqPair objs on the fly)
    std::unordered_map<
            core_arch_t,
            std::unordered_map<
                FrequencyMHz,
                std::unordered_map<
                    core_arch_t,
                    std::unordered_map<
                        FrequencyMHz,
                        MappingBin *
                    >
                >
            >
    > _predictors_faster;
    std::unordered_map<core_arch_t,std::set<FrequencyMHz>> _availableSrcFreqs;
    std::unordered_map<core_arch_t,std::set<FrequencyMHz>> _availableTgtFreqs;

  public:
    // Creates predictor from training data using the specified layers
    Predictor(LayerConf &funcs, TrainingData &training)
        :_sys_info(nullptr), _funcs(funcs)
    {
        _create_predictors(training.averagedWindows,_predictors,funcs);
        _make_faster();
        checkConsistency();
    }

    // Loads a predictor saved to a file
    Predictor(const std::string& filepath)
        :_sys_info(nullptr)
    {
        loadFromFile(filepath);
        checkConsistency();
    }

    // The constructors we wanna be using for runtime predictions
    Predictor(const sys_info_t *sys_info, const std::string& filepath)
        :_sys_info(sys_info)
    {
        loadFromFile(filepath);
        checkConsistency();
    }

    // The constructors we wanna be using for runtime predictions
    Predictor(const sys_info_t *sys_info)
    :_sys_info(sys_info)
    {
    }


    ~Predictor()
    {
        for(auto i : _predictors)
            delete i.second;
    }

    void saveToFile(const std::string& filepath);

    void loadFromFile(const std::string& filepath);

    // Test the predictor using some training data
    void test(TrainingData &training)
    {
        _test_predictors(training.averagedWindows,_predictors,_funcs);
    }

    // Predict using the sensing data for the given task.
    // Stores results in the provided vector
    void predict(std::vector<double> &result, const tracked_task_data_t *task, int wid, const core_info_t *target_core, int target_freq_mhz);

    const LayerConf& getFuncs()
    {
        return _funcs;
    }

    void checkConsistency()
    {
        assert_true(_predictors.size() > 0);
        for(auto corefreq : _predictors)
            corefreq.second->checkConsistency();
    }

  private:
    static void _create_predictors(std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows,
            std::map<CoreFreqPair, MappingBin *> &predictors, LayerConf &funcs);

    static void _test_predictors(std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> &averagedWindows,
            std::map<CoreFreqPair, MappingBin *> &predictors, LayerConf &funcs);

    void _make_faster();

    // Used whe we have the exact predictors for the src and tgt frequencies
    void _predict_single(std::vector<double> &result, const tracked_task_data_t *task, int wid,
            core_arch_t srcArch, FrequencyMHz srcFreq, core_arch_t tgtArch, FrequencyMHz tgtFreq);

    // Used when we don't have the exact predictors. We use the two nearest src/tgt frequencies
    // and use linear interpolation to get the final value
    void _predict_interpolate(std::vector<double> &result, const tracked_task_data_t *task, int wid,
            core_arch_t srcArch, FrequencyMHz srcFreq,
            core_arch_t tgtArch, FrequencyMHz tgtFreq0, FrequencyMHz tgtFreq1, FrequencyMHz origTgtFreq);

    // Aux vectors for predict interpolate
    static thread_local std::vector<double> _predict_interpolate_result0;
    static thread_local std::vector<double> _predict_interpolate_result1;


};


};

#endif
