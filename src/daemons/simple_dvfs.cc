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

#include <daemons/common/deamonizer.h>
#include <runtime/common/reports.h>
#include <runtime/framework/actuation_interface.h>
#include "../runtime/framework/models/baseline_model.h"
#include <runtime/managers/siso_dvfs.h>
#include <unistd.h>

class SimpleDVFSPolicy : public Policy {

    static constexpr int PERIOD_MS = 500;

  public:

    SimpleDVFSPolicy() :Policy(PERIOD_MS) {}

    double try_frequency(freq_domain_info_t *fd, int freq_mhz) {
        //pinfo("### try_frequency (fd=%d freq=%d) ####\n",fd->domain_id,freq_mhz);
        tryActuate<ACT_FREQ_MHZ>(fd,freq_mhz);
        //pinfo("# ips = %f \t power = %f\n",ips/1e6,power);
        return senseIf<SEN_PERFCNT>(PERFCNT_INSTR_EXE,fd) / senseIf<SEN_POWER_W>(fd);
    }

    void execute(int wid) override {
        /*try_frequency(&(info()->freq_domain_list[0]),300);
        try_frequency(&(info()->freq_domain_list[1]),300);

        try_frequency(&(info()->freq_domain_list[0]),300);

        try_frequency(&(info()->freq_domain_list[0]),1000);
        try_frequency(&(info()->freq_domain_list[1]),1000);

        try_frequency(&(info()->freq_domain_list[1]),1000);

        pinfo("Quit from daemon\n");
        manager()->quit();*/

        pinfo("#################\n");
        for(int _fd = 0; _fd < info()->freq_domain_list_size; ++_fd){
            freq_domain_info_t *fd = &(info()->freq_domain_list[_fd]);

            int best_freq = actuationRanges<ACT_FREQ_MHZ>(fd).min;
            double best_eff = 0;
            int step = actuationRanges<ACT_FREQ_MHZ>(fd).steps;
            for(int freq = best_freq;
                    freq <= actuationRanges<ACT_FREQ_MHZ>(fd).max;
                    freq += step){
                double freq_eff = try_frequency(fd,freq);
                if(freq_eff > best_eff){
                    best_freq = freq;
                    best_eff = freq_eff;
                }
            }
            pinfo("best freq for fd %d = %d) ####\n",fd->domain_id,best_freq);
            actuate<ACT_FREQ_MHZ>(fd,best_freq);
        }
        //pinfo("Quit from daemon\n");
        //manager()->quit();
    }

};

class SimpleDVFSManager : public PolicyManager {

  protected:

    static const std::string OPT_POLICY;
    using OPT_POLICY_TYPE = std::string;

    void setup()
    {
        /**
         * Two policies:
         * 1. "simple": SimpleDVFSPolicy
         * 2. "siso": SISODVFSPolicy
         */
        std::string policy = OptionParser::get<OPT_POLICY_TYPE>(OPT_POLICY);

        //pinfo("Waiting 10s for GDB to attach\n");
        //sleep(10);
        sensingModule()->enablePerTaskSensing();

        sensingModule()->tracePerfCounter(PERFCNT_INSTR_EXE);
        sensingModule()->tracePerfCounter(PERFCNT_BUSY_CY);
        sensingModule()->tracePerfCounter(PERFCNT_BRANCH_MISPRED);
        sensingModule()->tracePerfCounter(PERFCNT_L1DCACHE_MISSES);
        sensingModule()->tracePerfCounter(PERFCNT_LLCACHE_MISSES);

        enableReflection();

        if (policy.compare("simple") == 0) {
            registerPolicy(new SimpleDVFSPolicy());
        }
        else if (policy.compare("siso") == 0) {
            for(int i = 0; i < info()->freq_domain_list_size; ++i){
                registerPolicy(new SISODVFSPolicy(i));
            }
        } else {
            arm_throw(OptionsException,"Invalid policy: %s",policy.c_str());
        }
    }

  public:
    SimpleDVFSManager(SensingModule *sm) :PolicyManager(sm){}
};

const std::string SimpleDVFSManager::OPT_POLICY = "policy";

int main(int argc, char * argv[]){
    daemon_run<SimpleDVFSManager>(argc,argv);
    return 0;
}
