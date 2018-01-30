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



