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

#ifndef __arm_rt_system_info_h
#define __arm_rt_system_info_h

#include <vector>
#include <string>


/*
 * Proposed replacement for the old sys info struct
 *
 * Not used yet
 *
 */

class CoreInfo;
class CoreTypeInfo;
class FrequencyDomainInfo;
class PowerSensingDomainInfo;
class ThermalSensingDomainInfo;

class SystemInfo{
  public:
	//List of all processing element types available
	std::vector<CoreTypeInfo&> core_types;

	//List of all processing elements
	std::vector<CoreInfo&> proc_elems;

	//List of all DVFS domains
	std::vector<FrequencyDomainInfo&> freq_domains;

	//List of all power domains
	std::vector<PowerSensingDomainInfo&>       power_domains;

	//List of all thermal domains
	std::vector<ThermalSensingDomainInfo&>     thermal_domains;
};

class InfoObject {
  public:
	//name of this object
	const std::string name;

	//id of this object. must also be the index it appears in
	//the object-specific list in the SystemInfo struct
	const int id;

  protected:
	InfoObject(const std::string &name, int id) :name(name), id(id) {};
};

class CoreTypeInfo : InfoObject {
  public:
	CoreTypeInfo(const std::string &name, int id):InfoObject(name, id){}
};

class CoreInfo : InfoObject {
  public:
	//this element type
	CoreTypeInfo &type;

	//freq domain this element belongs to
	FrequencyDomainInfo &freq_domain;

	//power domain this element belongs to
	PowerSensingDomainInfo &power_domain;

	//thermal domain this element belongs to
	ThermalSensingDomainInfo &thermal_domain;

	CoreInfo(int id,
			CoreTypeInfo &type,
			FrequencyDomainInfo &freq_domain,
			PowerSensingDomainInfo &power_domain,
			ThermalSensingDomainInfo &thermal_domain)
		:InfoObject("core", id),
		 type(type),
		 freq_domain(freq_domain),
		 power_domain(power_domain),
		 thermal_domain(thermal_domain)
		 {}
};


class FrequencyDomainInfo : InfoObject {
  public:
	//all cores in this frequency domain
	std::vector<CoreInfo&> cores;

	//list of supported frequencies in mhz, ordered from lowest to highest
	std::vector<int> freqs;

	int freqMin() { return freqs.front();}
	int freqMax() { return freqs.back();}

	FrequencyDomainInfo(int id) :InfoObject("freq_domain", id){}

};

class PowerSensingDomainInfo : InfoObject {
  public:
	//all cores in this power domain
	std::vector<CoreInfo&> cores;

	PowerSensingDomainInfo(int id) :InfoObject("power_domain", id){}
};

class ThermalSensingDomainInfo : InfoObject {
  public:
	//all cores in this power domain
	std::vector<CoreInfo&> cores;

	ThermalSensingDomainInfo(int id) :InfoObject("thermal_domain", id){}
};

#endif

