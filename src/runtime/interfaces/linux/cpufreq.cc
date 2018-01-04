#include <sstream>

#include "cpufreq.h"

CpuFreq::CpuFreq(const sys_info_t &info)
	:_info(info),
	 _scaling_governor_i(new std::ifstream[info.core_list_size]),
	 _scaling_governor_o(new std::ofstream[info.core_list_size]),
	 _scaling_governor_inital_val(new std::string[info.core_list_size]),
	 _scaling_cur_freq(new std::ifstream[info.core_list_size]),
	 _scaling_setspeed(new std::ofstream[info.core_list_size]),
	 _scaling_max_freq_o(new std::ofstream[info.core_list_size]),
	 _scaling_max_freq_i(new std::ifstream[info.core_list_size]),
	 _scaling_max_freq_initial_val(new std::string[info.core_list_size]),
	 _scaling_min_freq_o(new std::ofstream[info.core_list_size]),
	 _scaling_min_freq_i(new std::ifstream[info.core_list_size]),
	 _scaling_min_freq_initial_val(new std::string[info.core_list_size])
{
	for(int i = 0; i < info.core_list_size; ++i){
		std::ostringstream f;
		f << "/sys/devices/system/cpu/cpu" << i << "/cpufreq/scaling_governor";
		_scaling_governor_i[i].open(f.str());
		_scaling_governor_o[i].open(f.str());
		if(_scaling_governor_i[i].is_open())
			_scaling_governor_i[i] >> _scaling_governor_inital_val[i];
		else
			arm_throw(CpuFreqException,"Unable to open cpufreq scaling_governor file");
		if(!_scaling_governor_o[i].is_open())
			arm_throw(CpuFreqException,"Unable to open cpufreq scaling_setspeed file");
	}

	for(int i = 0; i < info.core_list_size; ++i){
		std::ostringstream f;
		f << "/sys/devices/system/cpu/cpu" << i << "/cpufreq/scaling_setspeed";
		_scaling_setspeed[i].open(f.str());
		if(!_scaling_setspeed[i].is_open())
			arm_throw(CpuFreqException,"Unable to open cpufreq scaling_setspeed file");
	}

	for(int i = 0; i < info.core_list_size; ++i){
		std::ostringstream f;
		f << "/sys/devices/system/cpu/cpu" << i << "/cpufreq/scaling_cur_freq";
		_scaling_cur_freq[i].open(f.str());
		if(!_scaling_cur_freq[i].is_open())
			arm_throw(CpuFreqException,"Unable to open cpufreq scaling_cur_freq file");

	}

	for(int i = 0; i < info.core_list_size; ++i){
		std::ostringstream f;
		f << "/sys/devices/system/cpu/cpu" << i << "/cpufreq/scaling_max_freq";
		_scaling_max_freq_i[i].open(f.str());
		_scaling_max_freq_o[i].open(f.str());
		if(_scaling_max_freq_i[i].is_open())
			_scaling_max_freq_i[i] >> _scaling_max_freq_initial_val[i];
		else
			arm_throw(CpuFreqException,"Unable to open cpufreq scaling_max_freq file");
		if(!_scaling_max_freq_o[i].is_open())
			arm_throw(CpuFreqException,"Unable to open cpufreq scaling_max_freq file");
	}

	for(int i = 0; i < info.core_list_size; ++i){
		std::ostringstream f;
		f << "/sys/devices/system/cpu/cpu" << i << "/cpufreq/scaling_min_freq";
		_scaling_min_freq_i[i].open(f.str());
		_scaling_min_freq_o[i].open(f.str());
		if(_scaling_min_freq_i[i].is_open())
			_scaling_min_freq_i[i] >> _scaling_min_freq_initial_val[i];
		else
			arm_throw(CpuFreqException,"Unable to open cpufreq scaling_min_freq file");
		if(!_scaling_min_freq_o[i].is_open())
			arm_throw(CpuFreqException,"Unable to open cpufreq scaling_min_freq file");
	}
}

CpuFreq::CpuFreq(const sys_info_t &info, const char *governor)
	:CpuFreq(info)
{
	scaling_governor(governor);
}

CpuFreq::~CpuFreq()
{
	//restores original governor, closes files, and cleanup
	for(int i = 0; i < _info.core_list_size; ++i){
		if(_scaling_governor_o[i].is_open()){
			_scaling_governor_o[i] << _scaling_governor_inital_val[i];
			_scaling_governor_o[i].close();
		}
		if(_scaling_governor_i[i].is_open()) _scaling_governor_i[i].close();
	}
	delete[] _scaling_governor_i;
	delete[] _scaling_governor_o;
	delete[] _scaling_governor_inital_val;

	//restores original maximum frequency, closes files, and cleanup
	for(int i = 0; i < _info.core_list_size; ++i){
		if(_scaling_max_freq_o[i].is_open()){
			_scaling_max_freq_o[i] << _scaling_max_freq_initial_val[i];
			_scaling_max_freq_o[i].close();
		}
		if(_scaling_max_freq_i[i].is_open()) _scaling_max_freq_i[i].close();
	}
	delete[] _scaling_max_freq_i;
	delete[] _scaling_max_freq_o;
	delete[] _scaling_max_freq_initial_val;

	//restores original minimum frequency, closes files, and cleanup
	for(int i = 0; i < _info.core_list_size; ++i){
		if(_scaling_min_freq_o[i].is_open()){
			_scaling_min_freq_o[i] << _scaling_min_freq_initial_val[i];
			_scaling_min_freq_o[i].close();
		}
		if(_scaling_min_freq_i[i].is_open()) _scaling_min_freq_i[i].close();
	}
	delete[] _scaling_min_freq_i;
	delete[] _scaling_min_freq_o;
	delete[] _scaling_min_freq_initial_val;

	//closes cur frequency files, and cleanup
	for(int i = 0; i < _info.core_list_size; ++i){
		if(_scaling_cur_freq[i].is_open()) _scaling_cur_freq[i].close();
		if(_scaling_setspeed[i].is_open()) _scaling_setspeed[i].close();
	}
	delete[] _scaling_cur_freq;
	delete[] _scaling_setspeed;
}

void CpuFreq::scaling_governor(const freq_domain_info_t *domain, const char * governor)
{
	_scaling_governor_o[domain->__vitaminslist_head_cores->position] << governor;
	_scaling_governor_o[domain->__vitaminslist_head_cores->position].flush();
}
void CpuFreq::scaling_governor(const freq_domain_info_t *domain, const std::string &governor)
{
	_scaling_governor_o[domain->__vitaminslist_head_cores->position] << governor;
	_scaling_governor_o[domain->__vitaminslist_head_cores->position].flush();
}
void CpuFreq::scaling_governor(const std::string &governor)
{
	for(int i = 0; i < _info.freq_domain_list_size; ++i)
		scaling_governor(&(_info.freq_domain_list[i]),governor);
}
std::string CpuFreq::scaling_governor(const freq_domain_info_t *domain)
{
	std::string s;
	_scaling_governor_i[domain->__vitaminslist_head_cores->position].seekg(0);
	_scaling_governor_i[domain->__vitaminslist_head_cores->position] >> s;
	return s;
}

int CpuFreq::scaling_cur_freq(const freq_domain_info_t *domain)
{
	int freq_khz;
	_scaling_cur_freq[domain->__vitaminslist_head_cores->position].seekg(0);
	_scaling_cur_freq[domain->__vitaminslist_head_cores->position] >> freq_khz;
	return freq_khz / 1000;
}
void CpuFreq::scaling_setspeed(const freq_domain_info_t *domain, int freq_mhz)
{
	_scaling_setspeed[domain->__vitaminslist_head_cores->position] << _getFreq(freq_mhz);
	_scaling_setspeed[domain->__vitaminslist_head_cores->position].flush();
}
void CpuFreq::scaling_setspeed(const freq_domain_info_t *domain, core_freq_t freq)
{
	scaling_setspeed(domain,freqToValMHz_i(freq));
}

void CpuFreq::scaling_max_freq(const freq_domain_info_t *domain, int freq_mhz)
{
	_scaling_max_freq_o[domain->__vitaminslist_head_cores->position] << _getFreq(freq_mhz);
	_scaling_max_freq_o[domain->__vitaminslist_head_cores->position].flush();
}

void CpuFreq::scaling_min_freq(const freq_domain_info_t *domain, int freq_mhz)
{
	_scaling_min_freq_o[domain->__vitaminslist_head_cores->position] << _getFreq(freq_mhz);
	_scaling_min_freq_o[domain->__vitaminslist_head_cores->position].flush();
}

int CpuFreq::scaling_max_freq(const freq_domain_info_t *domain)
{
	int freq_khz;
	_scaling_max_freq_i[domain->__vitaminslist_head_cores->position].seekg(0);
	_scaling_max_freq_i[domain->__vitaminslist_head_cores->position] >> freq_khz;
	return freq_khz / 1000;
}
int CpuFreq::scaling_min_freq(const freq_domain_info_t *domain)
{
	int freq_khz;
	_scaling_min_freq_i[domain->__vitaminslist_head_cores->position].seekg(0);
	_scaling_min_freq_i[domain->__vitaminslist_head_cores->position] >> freq_khz;
	return freq_khz / 1000;
}

const std::string & CpuFreq::_getFreq(int valMHz)
{
	auto iter = _freqMap.find(valMHz);
	if(iter != _freqMap.end())
		return iter->second;
	else{
		std::ostringstream f;
		f << valMHz * 1000;
		_freqMap[valMHz] = f.str();
		return _freqMap[valMHz];
	}
}
