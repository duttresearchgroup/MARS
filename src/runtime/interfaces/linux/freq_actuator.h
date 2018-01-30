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

#ifndef __arm_rt_linux_actuatorfreq_h
#define __arm_rt_linux_actuatorfreq_h


#include <runtime/framework/types.h>
#include <runtime/framework/actuator.h>

#include "cpufreq.h"

class LinuxFrequencyActuator : public Actuator {

private:
	// Interface to cpufreq module
	CpuFreq _cpufreq;

	// Maximum/minimum/current frequencies per domain
	// This "buffers" the values read from the files
	// and are only checked once at the beggining and if
	// we change them. If someone other program is concurrently
	// messing up with cpufreq, then this will become inconsistent
	int *_freq_max_mhz;
	int *_freq_min_mhz;
	int *_freq_curr_mhz;//this is only buffered after we change the freq the first time
	bool *_freq_curr_valid;


protected:
	void implSystemMode(){
		_cpufreq.scaling_governor("ondemand");
	}
	void implSystemMode(const std::string &arg){
		_cpufreq.scaling_governor(arg);
	}
	void implFrameworkMode(){
		_cpufreq.scaling_governor("userspace");
	}

public:
	LinuxFrequencyActuator(const sys_info_t &_info)
		:Actuator(ACT_FREQ_MHZ,_info),_cpufreq(_info),
		 _freq_max_mhz(new int[_info.freq_domain_list_size]),
		 _freq_min_mhz(new int[_info.freq_domain_list_size]),
		 _freq_curr_mhz(new int[_info.freq_domain_list_size]),
		 _freq_curr_valid(new bool[_info.freq_domain_list_size])
	{
		for(int i = 0; i < _info.freq_domain_list_size; ++i){
			_freq_max_mhz[i] = _cpufreq.scaling_max_freq(&_info.freq_domain_list[i]);
			_freq_min_mhz[i] = _cpufreq.scaling_min_freq(&_info.freq_domain_list[i]);
			_freq_curr_valid[i] = false;
		}
		for(int i = 0; i < _info.freq_domain_list_size; ++i)
			setActForResource(&(_info.freq_domain_list[i]));
	}

	~LinuxFrequencyActuator()
	{
		delete[] _freq_curr_mhz;
		delete[] _freq_curr_valid;
		delete[] _freq_min_mhz;
		delete[] _freq_max_mhz;
	}

	// Standard actuation interface override

	void doSysActuation(freq_domain_info_t *rsc, int val_mhz){
		_cpufreq.scaling_setspeed(rsc,val_mhz);
		_freq_curr_mhz[rsc->domain_id] = _cpufreq.scaling_cur_freq(rsc);
		_freq_curr_valid[rsc->domain_id] = true;
		if((_freq_curr_mhz[rsc->domain_id] > (val_mhz+3)) || (_freq_curr_mhz[rsc->domain_id] < (val_mhz-3)))
			pinfo("WARNING: tried to set domain %d freq to %d mhz, actual set value was %d mhz\n",
					rsc->domain_id,val_mhz,_freq_curr_mhz[rsc->domain_id]);
	}
	void getSysActuation(freq_domain_info_t *rsc, int *val_mhz){
		if(_freq_curr_valid[rsc->domain_id])
			*val_mhz = _freq_curr_mhz[rsc->domain_id];
		else
			*val_mhz = _cpufreq.scaling_cur_freq(rsc);
	}


	// Actuator specific functions

	int freqMax(const freq_domain_info_t* rsc) { return _freq_max_mhz[rsc->domain_id];}
	int freqMin(const freq_domain_info_t* rsc) { return _freq_min_mhz[rsc->domain_id];}
	int freqMid(const freq_domain_info_t* rsc){ return (freqMax(rsc)+freqMin(rsc))/2;}

	int freqMax(const freq_domain_info_t& rsc) { return freqMax(&rsc);}
	int freqMin(const freq_domain_info_t& rsc) { return freqMin(&rsc);}
	int freqMid(const freq_domain_info_t& rsc){ return freqMid(&rsc);}


	/*
	 * TODO should actually call the set max from CpuFreq
	 */
	void freqMax(const freq_domain_info_t* rsc,int setMaxMHz) {
		_cpufreq.scaling_max_freq(rsc,setMaxMHz);
		_freq_max_mhz[rsc->domain_id] = _cpufreq.scaling_max_freq(rsc);
		if((_freq_max_mhz[rsc->domain_id] > (setMaxMHz+5)) || (_freq_max_mhz[rsc->domain_id] < (setMaxMHz-5)))
			pinfo("WARNING: tried to set domain %d max freq to %d mhz, actual set value was %d mhz\n",
					rsc->domain_id,setMaxMHz,_freq_max_mhz[rsc->domain_id]);
	}
	void freqMin(const freq_domain_info_t* rsc,int setMinMHz) {
		_cpufreq.scaling_min_freq(rsc,setMinMHz);
		_freq_min_mhz[rsc->domain_id] = _cpufreq.scaling_min_freq(rsc);
		if((_freq_min_mhz[rsc->domain_id] > (setMinMHz+5)) || (_freq_min_mhz[rsc->domain_id] < (setMinMHz-5)))
			pinfo("WARNING: tried to set domain %d min freq to %d mhz, actual set value was %d mhz\n",
					rsc->domain_id,setMinMHz,_freq_min_mhz[rsc->domain_id]);
	}

	void freqMax(const freq_domain_info_t& domain,int setMaxMHz) { freqMax(&domain,setMaxMHz);}
	void freqMin(const freq_domain_info_t& domain,int setMinMHz) { freqMin(&domain,setMinMHz);}

};

#endif

