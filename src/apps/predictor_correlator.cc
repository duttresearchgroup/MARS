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
#include <set>

#include <runtime/framework/models/bin_based_predictor.h>
#include <runtime/common/traceparser.h>
#include <runtime/common/statistics.h>

// Checks correlation of functions to perf/power
// App params:
//   1...n) Paths to directories containing traces used for training

void doit(int argc, char * argv[]){

    assert_true(argc >= 2);

    std::vector<std::string> tracePaths;
    for(int i = 1; i < argc; ++i) tracePaths.push_back(argv[i]);

    TraceParser parser(tracePaths);

    using BinBasedPred::BinFuncID::procTimeShare;
    using BinBasedPred::BinFuncID::ipcBusy;
    using BinBasedPred::BinFuncID::power;
    using BinBasedPred::BinFunc;

    BinFunc target(ipcBusy);
    std::vector<BinFunc> source;
    source.push_back(BinFunc(BinBasedPred::BinFuncID::brMisspredPerInstr));
    source.push_back(BinFunc(BinBasedPred::BinFuncID::l1DmissPerInstr));
    source.push_back(BinFunc(BinBasedPred::BinFuncID::l1ImissPerInstr));
    source.push_back(BinFunc(BinBasedPred::BinFuncID::LLCmissPerInstr));
    source.push_back(BinFunc(BinBasedPred::BinFuncID::dTLBmissPerInstr));
    source.push_back(BinFunc(BinBasedPred::BinFuncID::iTLBmissPerInstr));
    source.push_back(BinFunc(BinBasedPred::BinFuncID::branchAndCacheSum));
    source.push_back(BinFunc(BinBasedPred::BinFuncID::branchAndCacheMult));
    source.push_back(BinFunc(BinBasedPred::BinFuncID::L1DL1ILLCsum2));
    source.push_back(BinFunc(BinBasedPred::BinFuncID::L1DLLCmissPerInstr));

    std::vector<Statistics*> stats;

    std::cout << "COmputing samples\n";

    auto traces = parser.traces();
    TraceSensingInterace::set(traces);

    for(auto metric : source){
        Statistics *stat = new Statistics(*(metric.str), *(target.str));
        stats.push_back(stat);
        for(auto task : traces.samples){
            for(auto archSrc : task.second){
                for(auto freqSrc : archSrc.second){
                    for(auto sample : *(freqSrc.second)){
                        double x = metric(sample._data);
                        double y = target(sample._data);
                        stat->addSample(x,y);
                    }
                }
            }
        }
    }


    BinFunc ipc(ipcBusy);
    BinFunc pow(power);
    Statistics *stat = new Statistics(*(ipc.str), *(pow.str));
    stats.push_back(stat);
    for(auto task : traces.samples){
        for(auto archSrc : task.second){
            for(auto freqSrc : archSrc.second){
                for(auto sample : *(freqSrc.second)){
                    //here we only want samples in the active phase
                    if (BinFunc(procTimeShare)(sample._data) < 0.9)
                        continue;
                    double x = ipc(sample._data);
                    double y = pow(sample._data);
                    stat->addSample(x,y);
                }
            }
        }
    }


    auto comp = [](Statistics* x, Statistics* y){ return std::fabs(x->correlation()) > std::fabs(y->correlation()); };
    auto set  = std::set<Statistics*,decltype(comp)>( comp );
    for(auto stat : stats) set.insert(stat);
    for(auto stat : set){
        std::cout << "corr(" << stat->sampleXName << ", " << stat->sampleYName << ") = "
                  << stat->correlation() << "\n";
    }

    for(auto stat : stats) delete stat;
}

int main(int argc, char * argv[]){
    try{
        doit(argc,argv);
    } arm_catch(ARM_CATCH_NO_EXIT)
    return 0;
}
