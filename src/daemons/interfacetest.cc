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

#include <runtime/daemon/deamonizer.h>
#include <runtime/framework/actuation_interface.h>
#include <runtime/common/strings.h>
#include <map>

class InterfaceTest : public PolicyManager {
protected:
    static const int WINDOW_LENGTH_FINE_MS = 50;
    static const int WINDOW_LENGTH_COARSE_MS = 200;

    virtual void setup();

    const SensingWindowManager::WindowInfo *sensingWindow_fine;
    const SensingWindowManager::WindowInfo *sensingWindow_coarse;

    static void fine_window_handler(int wid,PolicyManager *owner);
    static void coarse_window_handler(int wid,PolicyManager *owner);

    ExecutionTrace _execTrace_fine;
    ExecutionTrace _execTrace_coarse;

    //is freq increassing or decreassing ?
    std::map<int,bool> _fd_state;

public:
    InterfaceTest(SensingModule *sm) :PolicyManager(sm),
        sensingWindow_fine(nullptr),sensingWindow_coarse(nullptr),
        _execTrace_fine("execTraceFine"),_execTrace_coarse("execTraceCoarse")
        {};

};

void InterfaceTest::setup()
{
    sensingModule()->enablePerTaskSensing();
    sensingWindow_fine = windowManager()->addSensingWindowHandler(WINDOW_LENGTH_FINE_MS,this,fine_window_handler);
    sensingWindow_coarse = windowManager()->addSensingWindowHandler(WINDOW_LENGTH_COARSE_MS,this,coarse_window_handler);

    for(int domain_id = 0; domain_id < info()->power_domain_list_size; ++domain_id){
        _fd_state[domain_id] = false;
        actuate<ACT_FREQ_MHZ>(
                            &(info()->freq_domain_list[domain_id]),
                            actuationRanges<ACT_FREQ_MHZ>(&(info()->freq_domain_list[domain_id])).max);
    }
}

void InterfaceTest::fine_window_handler(int wid,PolicyManager *owner)
{
    InterfaceTest *self =  dynamic_cast<InterfaceTest*>(owner);

    auto sensedData = self->sensingModule()->data();
    auto trace = self->_execTrace_fine.getHandle(sensedData,wid);

    //save total power
    double totalPowerW = 0;
    for(int domain_id = 0; domain_id < owner->info()->power_domain_list_size; ++domain_id){
        totalPowerW += sense<SEN_POWER_W>(&owner->info()->power_domain_list[domain_id],wid);
    }
    trace("total_power_w") = totalPowerW;

    uint64_t totalInsts = 0;
    double totalCPUTime = 0;

    for(int i = 0; i < owner->info()->core_list_size; ++i){
        core_info_t &core = owner->info()->core_list[i];
        totalInsts += sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,&core,wid);
        totalCPUTime += sense<SEN_BUSYTIME_S>(&core,wid);
    }
    trace("total_cpu_time_s") = totalCPUTime;
    trace("total_instr") = totalInsts;

    for(int i = 0; i < owner->info()->freq_domain_list_size; ++i){
        freq_domain_info_t &fd = owner->info()->freq_domain_list[i];

        trace(formatstr("freq_domain%d_sensed",i)) = sense<SEN_FREQ_MHZ>(&fd,wid);

        int curr = actuationVal<ACT_FREQ_MHZ>(&fd);

        trace(formatstr("freq_domain%d_set",i)) = curr;

        //reached max and we were increassing freq
        if((curr >= actuationRanges<ACT_FREQ_MHZ>(&fd).max) && self->_fd_state[i])
            self->_fd_state[i] = false;//now we decrease
        //reached min and we were decreassing freq
        else if((curr <= actuationRanges<ACT_FREQ_MHZ>(&fd).min) && !self->_fd_state[i])
            self->_fd_state[i] = true;//now we increase

        if(self->_fd_state[i])
            actuate<ACT_FREQ_MHZ>(&fd,curr + actuationRanges<ACT_FREQ_MHZ>(&fd).steps);
        else
            actuate<ACT_FREQ_MHZ>(&fd,curr - actuationRanges<ACT_FREQ_MHZ>(&fd).steps);
    }
}

void InterfaceTest::coarse_window_handler(int wid,PolicyManager *owner)
{
    InterfaceTest *self =  dynamic_cast<InterfaceTest*>(owner);

    auto sensedData = self->sensingModule()->data();
    auto trace = self->_execTrace_coarse.getHandle(sensedData,wid);

    //save total power
    double totalPowerW = 0;
    for(int domain_id = 0; domain_id < owner->info()->power_domain_list_size; ++domain_id){
        totalPowerW += sense<SEN_POWER_W>(&owner->info()->power_domain_list[domain_id],wid);
    }
    trace("total_power_w") = totalPowerW;

    uint64_t totalInsts = 0;
    double totalCPUTime = 0;

    for(int i = 0; i < owner->info()->core_list_size; ++i){
        core_info_t &core = owner->info()->core_list[i];
        totalInsts += sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,&core,wid);
        totalCPUTime += sense<SEN_BUSYTIME_S>(&core,wid);
    }
    trace("total_cpu_time_s") = totalCPUTime;
    trace("total_instr") = totalInsts;

    for(int i = 0; i < owner->info()->freq_domain_list_size; ++i){
        freq_domain_info_t &fd = owner->info()->freq_domain_list[i];

        trace(formatstr("freq_domain%d_sensed",i)) = sense<SEN_FREQ_MHZ>(&fd,wid);
    }
}

int main(int argc, char * argv[]){
	daemon_run<InterfaceTest>(argc,argv);
	return 0;
}
