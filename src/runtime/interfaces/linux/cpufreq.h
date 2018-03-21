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

#ifndef __arm_rt_cpufreq_h
#define __arm_rt_cpufreq_h

#include <fstream>
#include <map>

#include <base/base.h>

class CpuFreq
{

	const sys_info_t &_info;
	//per core
	std::ifstream *_scaling_governor_i;
	std::ofstream *_scaling_governor_o;
	std::string *_scaling_governor_inital_val;
	std::ifstream *_scaling_cur_freq;
	std::ofstream *_scaling_setspeed;
	std::ofstream *_scaling_max_freq_o;
	std::ifstream *_scaling_max_freq_i;
	std::string *_scaling_max_freq_initial_val;
	std::ofstream *_scaling_min_freq_o;
	std::ifstream *_scaling_min_freq_i;
	std::string *_scaling_min_freq_initial_val;

	//maps a int val in mhz to the string in khz
	std::map<int,std::string> _freqMap;

	const std::string & _getFreq(int valMHz);

public:
	CpuFreq(const sys_info_t &info);
	CpuFreq(const sys_info_t &info, const char *governor);
	~CpuFreq();


	void scaling_governor(const freq_domain_info_t *domain, const char *governor);
	void scaling_governor(const freq_domain_info_t *domain, const std::string &governor);
	void scaling_governor(const std::string &governor);
	std::string scaling_governor(const freq_domain_info_t *domain);

	//return freq in mhz
	int scaling_cur_freq(const freq_domain_info_t *domain);

	void scaling_setspeed(const freq_domain_info_t *domain, int freq_mhz);
	void scaling_setspeed(const freq_domain_info_t *domain, core_freq_t freq);

	void scaling_max_freq(const freq_domain_info_t *domain, int freq_mhz);
	void scaling_min_freq(const freq_domain_info_t *domain, int freq_mhz);
	//return min/max in mhz
	int scaling_max_freq(const freq_domain_info_t *domain);
	int scaling_min_freq(const freq_domain_info_t *domain);

	const sys_info_t& sys_info() { return _info; }

};


#endif

