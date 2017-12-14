
#include <iostream>
#include <fstream>
#include <cmath>
#include <set>

#include "common/app_common.h"
#include "common/bin_based_predictor_common.h"


int main(){

    std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> averagedWindows;

    obtain_windows(
            {"traces/gem5-alpha-aggressive_hmps-dse-dec15"},
            {COREARCH_GEM5_HUGE_HUGE,COREARCH_GEM5_BIG_BIG,COREARCH_GEM5_MEDIUM_MEDIUM,COREARCH_GEM5_LITTLE_LITTLE},true,
            {COREFREQ_2000MHZ,COREFREQ_1500MHZ,COREFREQ_1000MHZ},true,
            averagedWindows);

    std::map<CoreFreqPair, MappingBin *> predictorsIPCPower;
    LayerConf funcsIPCPower;
    funcsIPCPower.final_metric.push_back({makeMetric(ipcActive)});
    funcsIPCPower.final_metric.push_back({makeMetric(powerActive)});
    funcsIPCPower.addLayer(10, makeMetric(L1DL1IL2sum));
    funcsIPCPower.addLayer(10, makeMetric(brMisspredPerInstr));
    funcsIPCPower.addLayer(10, makeMetric(ipcActive));
    create_predictors(averagedWindows,predictorsIPCPower,funcsIPCPower);

    bin_pred_ptr_t cpred = gen_cpred(funcsIPCPower, predictorsIPCPower);
    vitamins_bin_predictor_writefile("predictor_agressive_hmp.data",cpred);


}



