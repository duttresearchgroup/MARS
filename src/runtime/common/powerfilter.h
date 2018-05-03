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

#ifndef __arm_rt_powerfilter_h
#define __arm_rt_powerfilter_h

#include <runtime/framework/sensing_interface.h>
#include <base/base.h>
#include <runtime/interfaces/performance_data.h>

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
	void sampleSys(int wid)
	{
		_currPow = SensingInterface::sense<SEN_POWER_W>(&_pd,wid);
		_currPowFiltered = (_avgWeight*_currPowFiltered) + ((1-_avgWeight)*_currPow);
	}
};


#endif

