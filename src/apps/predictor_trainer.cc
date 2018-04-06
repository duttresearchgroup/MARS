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
#include <stdlib.h>
#include <map>
#include <vector>


#include <runtime/common/traceparser.h>
#include <runtime/framework/models/bin_based_predictor.h>

// Trains a bin based predictor using the provided traces
// App params:
//   1) The path where the predictor file is saved
//   2...n) Paths to directories containing traces used for training

int main(int argc, char * argv[]){
    try {
        assert_true(argc >= 3);

        std::string predPath = argv[1];
        std::vector<std::string> tracePaths;
        for(int i = 2; i < argc; ++i) tracePaths.push_back(argv[i]);

        TraceParser parser(tracePaths);

        BinBasedPred::TrainingData trainingData(parser.traces());

        using BinBasedPred::BinFuncID::ipcBusy;
        using BinBasedPred::BinFuncID::power;
        using BinBasedPred::BinFuncID::brMisspredPerInstr;
        using BinBasedPred::BinFuncID::l1DmissPerInstr;
        using BinBasedPred::BinFuncID::LLCmissPerInstr;
        using BinBasedPred::BinFuncID::ipcBusy;
        using BinBasedPred::BinFunc;

        BinBasedPred::LayerConf funcsIPCPower;
        funcsIPCPower.final_metric.push_back(BinFunc(ipcBusy));
        funcsIPCPower.final_metric.push_back(BinFunc(power));
        funcsIPCPower.addLayer(10, BinFunc(brMisspredPerInstr));
        funcsIPCPower.addLayer(5, BinFunc(l1DmissPerInstr));
        funcsIPCPower.addLayer(5, BinFunc(LLCmissPerInstr));
        funcsIPCPower.addLayer(10, BinFunc(ipcBusy));

        BinBasedPred::Predictor predictor(funcsIPCPower,trainingData);

        //predictor.test(trainingData);

        std::cout << "Saving to file\n\n";
        predictor.saveToFile(predPath);

        std::cout << "Reading back from file\n";
        BinBasedPred::Predictor anotherPredictor(predPath);

        anotherPredictor.test(trainingData);

        std::cout << "\nDONE\n";

    } arm_catch(ARM_CATCH_NO_EXIT)
}


