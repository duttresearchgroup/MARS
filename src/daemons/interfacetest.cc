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
#include <runtime/interfaces/actuation_interface.h>
#include <map>

class InterfaceTest : public System {
protected:
    static const int WINDOW_LENGTH_FINE_MS = 50;
    static const int WINDOW_LENGTH_COARSE_MS = 200;

    virtual void setup();
    virtual void report();

    const SensingWindowManager::WindowInfo *sensingWindow_fine;
    const SensingWindowManager::WindowInfo *sensingWindow_coarse;

    static void fine_window_handler(int wid,System *owner);
    static void coarse_window_handler(int wid,System *owner);

    ExecutionTrace _execTrace_fine;
    ExecutionTrace _execTrace_coarse;

    FrequencyActuator _freqAct;

    //is freq increassing or decreassing ?
    std::map<int,bool> _fd_state;

public:
    InterfaceTest() :System(),
        sensingWindow_fine(nullptr),sensingWindow_coarse(nullptr),
        _execTrace_fine("execTraceFine"),_execTrace_coarse("execTraceCoarse"),
        _freqAct(*info()){};

};

void InterfaceTest::setup()
{
    _manager->sensingModule()->enablePerTaskSensing();
    sensingWindow_fine = _manager->addSensingWindowHandler(WINDOW_LENGTH_FINE_MS,this,fine_window_handler);
    sensingWindow_coarse = _manager->addSensingWindowHandler(WINDOW_LENGTH_COARSE_MS,this,coarse_window_handler);

    _freqAct.setFrameworkMode();
    for(int domain_id = 0; domain_id < info()->power_domain_list_size; ++domain_id){
        _fd_state[domain_id] = false;
        actuate<ACT_FREQ_MHZ>(
                            info()->freq_domain_list[domain_id],
                            _freqAct.freqMax(info()->freq_domain_list[domain_id]));
    }
}

static char _formatstr_buff[64];
template<typename... Args>
static const char * formatstr(const char *s, Args... args){
    std::snprintf(_formatstr_buff,64,s,args...);
    return _formatstr_buff;
}

void InterfaceTest::fine_window_handler(int wid,System *owner)
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

        int curr = actuationVal<ACT_FREQ_MHZ>(fd);

        trace(formatstr("freq_domain%d_set",i)) = curr;

        //reached max and we were increassing freq
        if((curr >= self->_freqAct.freqMax(fd)) && self->_fd_state[i])
            self->_fd_state[i] = false;//now we decrease
        //reached min and we were decreassing freq
        else if((curr <= self->_freqAct.freqMin(fd)) && !self->_fd_state[i])
            self->_fd_state[i] = true;//now we increase

        if(self->_fd_state[i])
            actuate<ACT_FREQ_MHZ>(fd,curr+100);
        else
            actuate<ACT_FREQ_MHZ>(fd,curr-100);
    }
}

void InterfaceTest::coarse_window_handler(int wid,System *owner)
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

void InterfaceTest::report()
{
    ExecutionSummary db(info());
    db.setWid(sensingWindow_fine->wid);
    db.record();
}

int main(int argc, char * argv[]){
	daemon_setup(argc,argv);
	daemon_run_sys<InterfaceTest>();
	return 0;
}
