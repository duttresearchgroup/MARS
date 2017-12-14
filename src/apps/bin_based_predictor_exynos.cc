#include <iostream>
#include <fstream>
#include <cmath>
#include <set>

#include "common/app_common.h"
#include "common/bin_based_predictor_common.h"


int main(){

	vitamins_reset_archs();
	vitamins_init_idle_power_fromfile("idlepower_exynos.data");

	std::map<CoreFreqPair, std::vector<AveragedMatchedWindow>*> averagedWindows;

    obtain_windows_single_sample(
            {"traces/exynos5422-spring16"},{},false,{},false,
            averagedWindows);

    std::map<CoreFreqPair, MappingBin *> predictorsIPCPower;
    LayerConf funcsIPCPower;
    funcsIPCPower.final_metric.push_back({makeMetric(ipcActive)});
    funcsIPCPower.final_metric.push_back({makeMetric(powerActiveFromIdle)});
    //funcsIPCPower.addLayer(10, makeMetric(L1DL2missPerInstr));
    funcsIPCPower.addLayer(10, makeMetric(brMisspredPerInstr));
    funcsIPCPower.addLayer(5, makeMetric(l1DmissPerInstr));
    funcsIPCPower.addLayer(5, makeMetric(L2missPerInstr));
    funcsIPCPower.addLayer(10, makeMetric(ipcActive));
    create_predictors(averagedWindows,predictorsIPCPower,funcsIPCPower);

    bin_pred_ptr_t cpred = gen_cpred(funcsIPCPower, predictorsIPCPower);
    vitamins_bin_predictor_writefile("predictor_exynos.data",cpred);


}


