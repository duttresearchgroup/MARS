#ifndef __arm_rt_powerfilter_h
#define __arm_rt_powerfilter_h

#include <runtime/interfaces/sensed_data.h>
#include <core/core.h>

class PowerFilter
{
	const power_domain_info_t &_pd;
	double _currPow;
	double _currPowFiltered;
	double _avgWeight;

public:
	PowerFilter(power_domain_info_t &pd)
		:_pd(pd),_currPow(0),_currPowFiltered(0),_avgWeight(0){ }

	double power() { return _currPow;}
	double powerFiltered() { return _currPowFiltered;}

	void powerFilter(double val) { _avgWeight = val;}
		//sets filter based on the window length
	void powerFilter(int window_length_ms) { _avgWeight = (window_length_ms) > 1000 ? 0 : 1-(((double)window_length_ms*1) / 1000.0);}

	//reads the power from the power domain
	void sampleSys(const SensedData& data, int wid)
	{
		const sensed_data_power_domain_t& powData = data.swCurrData(wid).power_domains[_pd.domain_id];
		_currPow = ((double)powData.avg_power_uW_acc / (double) powData.time_ms_acc)/1000000;
		_currPowFiltered = (_avgWeight*_currPowFiltered) + ((1-_avgWeight)*_currPow);
	}
};


#endif

