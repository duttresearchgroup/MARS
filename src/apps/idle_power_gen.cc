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

#include "common/app_common.h"
#include "offline_sim/inputparser.h"
#include "core_legacy/core.h"

#include <iostream>

static
bool
calc_core_idle_power(input_data_t &inputData, core_arch_t arch, core_freq_t freq, double &idlePower)
{

    const double maxRateDiff = 0.0005;
    double avgPower = 0;
    double avgCnt = 0;

    for (auto bench : inputData.samples){

        if(bench.second.find(arch) == bench.second.end())
            return false;
        else if(bench.second[arch].find(freq) == bench.second[arch].end())
            return false;

        for (auto sample : *(bench.second[arch][freq])){
            assert(sample.data_format == task_data_t::CSV_PRED_AND_SIM);
            double power = sample.data.gatedSubThrLeakPower
                         + sample.data.gateLeakPower
                         + sample.data.l2SubThrLeakPower
                         + sample.data.l2GateLeakPower;
            avgPower += power;
            avgCnt += 1;
            double currAvg = avgPower/avgCnt;

            if(currAvg > power){
                if((currAvg-power)>maxRateDiff){
                    std::cout << "Idle power for " << archToString(arch) << "/" << freqToString(freq) << "="
                              << power << " is diff then avg=" << currAvg << "\n";
                    abort();
                }
            }
            else{
                if((power-currAvg)>maxRateDiff){
                    std::cout << "Idle power for " << archToString(arch) << "/" << freqToString(freq) << "="
                                              << power << " is diff then avg=" << currAvg << "\n";
                    abort();
                }
            }

        }
    }
    //std::cout << "Idle power for " << archToString(arch) << "/" << freqToString(freq) << "=" << avgPower/avgCnt <<  "\n";
    idlePower = avgPower/avgCnt;
    return true;
}

static
void
init_idle_power(input_data_t &inputData)
{
    vitamins_reset_archs();
    for (int core_type = 0; core_type < SIZE_COREARCH; ++core_type) {
        for (int core_freq = 0; core_freq < SIZE_COREFREQ; ++core_freq){
            double power;
            if(calc_core_idle_power(inputData,(core_arch_t)core_type,(core_freq_t)core_freq,power))
                vitamins_init_idle_power((core_arch_t)core_type,(core_freq_t)core_freq,
                                          CONV_DOUBLE_scaledUINT32(power));
        }
    }
}

static
void
init_idle_power_exynos_5422()
{
	double bigClusterIdle_uW[SIZE_COREFREQ];
	double littleClusterIdle_uW[SIZE_COREFREQ];

	for(int freq = 0; freq < SIZE_COREFREQ; ++freq){
		bigClusterIdle_uW[freq] = 0;
		littleClusterIdle_uW[freq] = 0;
	}

	//uW per cluster
	littleClusterIdle_uW[COREFREQ_1000MHZ] = 75000;
	littleClusterIdle_uW[COREFREQ_1100MHZ] = 90000;
	littleClusterIdle_uW[COREFREQ_1200MHZ] = 110000;
	littleClusterIdle_uW[COREFREQ_1300MHZ] = 130000;
	littleClusterIdle_uW[COREFREQ_1400MHZ] = 160000;

	bigClusterIdle_uW[COREFREQ_1200MHZ] = 250000;
	bigClusterIdle_uW[COREFREQ_1300MHZ] = 280000;
	bigClusterIdle_uW[COREFREQ_1400MHZ] = 310000;
	bigClusterIdle_uW[COREFREQ_1500MHZ] = 350000;
	bigClusterIdle_uW[COREFREQ_1600MHZ] = 410000;
	bigClusterIdle_uW[COREFREQ_1700MHZ] = 470000;
	bigClusterIdle_uW[COREFREQ_1800MHZ] = 545000;
	bigClusterIdle_uW[COREFREQ_1900MHZ] = 650000;
	bigClusterIdle_uW[COREFREQ_2000MHZ] = 825000;

	const double bigClusterNumCores = 4;
	const double littleClusterNumCores = 4;

	//convert to W per core
	for(int freq = 0; freq < SIZE_COREFREQ; ++freq){
		bigClusterIdle_uW[freq] = (bigClusterIdle_uW[freq] / 1000000)/bigClusterNumCores;
		littleClusterIdle_uW[freq] = (littleClusterIdle_uW[freq] / 1000000)/littleClusterNumCores;
	}


	//commit
	vitamins_reset_archs();
	for(int freq = 0; freq < SIZE_COREFREQ; ++freq){
		if(littleClusterIdle_uW[freq]!=0)
			vitamins_init_idle_power(COREARCH_Exynos5422_LITTLE,
					(core_freq_t)freq,
					CONV_DOUBLE_scaledUINT32(littleClusterIdle_uW[freq]));

		if(bigClusterIdle_uW[freq]!=0)
			vitamins_init_idle_power(COREARCH_Exynos5422_BIG,
					(core_freq_t)freq,
					CONV_DOUBLE_scaledUINT32(bigClusterIdle_uW[freq]));
	}


}

int main(){

	input_data_t *inputs;

	inputs = parse_csvs({"traces/gem5-alpha-aggressive_hmps-spring15/parsec","traces/gem5-alpha-aggressive_hmps-spring15/mibench","traces/gem5-alpha-aggressive_hmps-spring15/ubench"});
	check_inputs(*inputs);
	init_idle_power(*inputs);
	vitamins_writefile_idle_power("idlepower_agressive_hmp.data");
	delete inputs;

	inputs = parse_csvs({"traces/gem5-alpha-aggressive_hmps-dse-dec15"});
	check_inputs(*inputs);
	init_idle_power(*inputs);
	vitamins_writefile_idle_power("idlepower_agressive_hmp_dse.data");
	delete inputs;

	inputs = parse_csvs({"traces/gem5-alpha-mimo-ctrl-mar17"});
	check_inputs(*inputs);
	init_idle_power(*inputs);
	vitamins_writefile_idle_power("idlepower_mimo_ctrl.data");
	delete inputs;

	init_idle_power_exynos_5422();
    vitamins_writefile_idle_power("idlepower_exynos.data");

}

