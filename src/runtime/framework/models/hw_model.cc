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

#include <fstream>
#include <sstream>

#include "hw_model.h"

#include <runtime/common/strings.h>

#include <external/minijson_reader/minijson_reader.hpp>


static inline core_arch_t archFromStr(const std::string &arch)
{
    for(int i = 0; i < SIZE_COREARCH; ++i){
        std::string aux(archToString((core_arch_t)i));
        if(aux == arch) return (core_arch_t)i;
    }
    assert_false("Couldn't find arch!");
    return SIZE_COREARCH;
}

void StaticHWModel::_loadModels()
{
    _binPred.loadFromFile(_predictorPathName());

    //chacks if we have the right predictor
    auto predMetrics =  _binPred.getFuncs().final_metric;
    if(predMetrics.size() != 2)
        arm_throw(ModelException,"Loaded predictor is invalid");
    if((predMetrics[BIN_PRED_IPC_BUSY_IDX].id != BinBasedPred::BinFuncID::ipcBusy)||
       (predMetrics[BIN_PRED_POWER_IDX].id != BinBasedPred::BinFuncID::power))
        arm_throw(ModelException,"Loaded predictor is invalid");

    //load idle power
    std::ifstream is(_idlePowerPathName());
    minijson::istream_context ctx(is);

    minijson::parse_object(ctx, [&](const char* name, minijson::value value)
    {
        if(streq(name, "idle_power"))
            minijson::parse_object(ctx, [&](const char* name, minijson::value value)
            {
                core_arch_t arch = archFromStr(name);
                minijson::parse_object(ctx, [&](const char* name, minijson::value value)
                {
                    int freq = fromstr<int>(name);
                    _idlePower[arch][freq] = value.as_double();
                });
            });
        else
            minijson::ignore(ctx);
    });

    is.close();

}
