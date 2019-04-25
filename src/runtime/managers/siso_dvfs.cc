/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * Copyright (C) 2018 Bryan Donyanavard <bdonyana@uci.edu>
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

#include "siso_dvfs.h"

void SISODVFSPolicy::execute(int wid)
{
    ++_samples;

    int curr_phase = curr_ref_phase();
    if(curr_phase==-1) {
        return;
    }

    set_ctrl_ref();

    /*******************************************
     * Read current local frequency
     */
    unsigned int last_freq = sense<SEN_FREQ_MHZ>(
        &(info()->freq_domain_list[_domain_id]),wid);

    /*******************************************
     * Calculate average IPS for freq domain
     */
    uint64_t totalInsts = 0;
    double totalCPUTime = 0;

    freq_domain_info_t &fd = info()->freq_domain_list[_domain_id];
    for(int i = 0; i < info()->core_list_size; ++i){
        core_info_t &core = info()->core_list[i];
        if (core.freq->domain_id == fd.domain_id) {
            totalInsts += sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,&core,wid);
            totalCPUTime += sense<SEN_BUSYTIME_S>(&core,wid);
        }
    }
    double avg_ips = totalInsts / totalCPUTime;

    /*******************************************
     * Invoke controller
     */
    double ctrl_out = _ctrl_f_ips.nextInput((avg_ips / 1000000));
    unsigned int next_freq_mhz = ctrl_out;
    unsigned int next_freq_closest = freqToMHz(next_freq_mhz);
    pinfo("last freq = %u\tctrl_out = %f (mips = %f, ref = %f)\t"
          "next_freq_mhz = %u\tnext freq closest = %u\n",
        last_freq, ctrl_out, avg_ips/1000000, _p_ref[_ref_phase]/1000000,
        next_freq_mhz, next_freq_closest);

    /*******************************************
     * Set local frequency
     */
    actuate<ACT_FREQ_MHZ>(&(info()->freq_domain_list[_domain_id]),
        next_freq_closest);
}

void SISODVFSPolicy::onRegister() {
    init_ctrl();
}

void SISODVFSPolicy::init_ctrl()
{
    controller_gains_all();

    parse_refs();
    set_ctrl_ref();
}

/**
 * These are example PID controller coefficients from (Matlab) system
 * identification of an ARM A15 core in a Samsung Exynos5422 (ODROIDXU3)
 * executing MARS ubenchmarks on Ubuntu
 */
void SISODVFSPolicy::controller_gains_all() {
    double kp,ki,mu;
    kp = 0.017742; //ubench training
    ki = 0.035484; //ubench training
    mu = 1650;

    _ctrl_f_ips.pid.gains(kp,ki,0);
    _ctrl_f_ips.inputFilter.offset(mu);
}

/**
 * Two options required to specify references:
 * 1. "refs": comma-separated list of MIPS reference values (double)
 * 2. "ref_interval": length in seconds of each reference phase (int)
 *
 */
const std::string SISODVFSPolicy::OPT_REFS = "refs";
const std::string SISODVFSPolicy::OPT_REF_INT = "ref_interval";

void SISODVFSPolicy::parse_refs()
{
    auto refs = OptionParser::getVector<OPT_REFS_TYPE>(OPT_REFS);

    _ref_phase_cnt = refs.size();
    if(_ref_phase_cnt < 1) {
        arm_throw(UnsupervisedSysException,
	    "Invalid number of references provided");
    }

    _ref_time = OptionParser::get<OPT_REF_INT_TYPE>(OPT_REF_INT);

    for(int i = 0; i < _ref_phase_cnt; i++){
        //ref is MIPS
        //divide by #cores is a shortcut where we distribute IPS ref evenly to each core
        _p_ref.push_back(std::stod(refs[i])/info()->core_list_size);
    }

    for(int i = 0; i < _ref_phase_cnt; ++i){
        pinfo("\tat %d secs: mips_ref=%f\n",_ref_time*(i+1),_p_ref[i]);
    }
}

void SISODVFSPolicy::set_ctrl_ref()
{
    if (_ref_phase != curr_ref_phase()) {
        _ref_phase = curr_ref_phase();
        pinfo("Now working with ctrl refs phase %d\n",_ref_phase);
    }

    _ctrl_f_ips.errorFilter.ref(_p_ref[_ref_phase]);
}

int SISODVFSPolicy::curr_ref_phase()
{
    int val = -1;
    for(int i = 0; i < _ref_phase_cnt; ++i){
        if((_samples*periodMS()) >= (_ref_time*(i+1)*1000))
            val = i;
    }
    return val;
}
