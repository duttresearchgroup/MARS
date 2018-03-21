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
#include <unordered_map>

class IdlePowerChecker : public PolicyManager {
protected:
    static const int WINDOW_LENGTH_MS = 200;

    static const int FREQ_STEPS_MHZ = 100;

    static const int ITERATIONS = 4;

    virtual void setup();

    const SensingWindowManager::WindowInfo *sensingWindow;

    static void window_handler(int wid,PolicyManager *owner);

    enum state {
        INCREASING,
        DECREASING
    };

    state _state;
    int _iterations;

    std::unordered_map<void*,ExecutionTrace*> _execTraces;

    template<typename Rsc>
    ExecutionTrace::ExecutionTraceHandle& _getTraceHandle(const std::string &type, const Rsc &rsc, int wid)
    {
        auto iter = _execTraces.find((void*)&rsc);
        if(iter != _execTraces.end())
            return iter->second->getHandle(wid);
        else{
            ExecutionTrace *execTrace = new ExecutionTrace("idle_trace."+type+"."+std::to_string(rsc.domain_id));
            _execTraces[(void*)&rsc] = execTrace;
            return execTrace->getHandle(wid);
        }
    }

public:
    IdlePowerChecker() :PolicyManager(),
        sensingWindow(nullptr),
        _state(INCREASING),_iterations(0){};

    virtual ~IdlePowerChecker(){
        for(auto iter : _execTraces){
            delete iter.second;
        }
    }

};

void IdlePowerChecker::setup()
{
    sensingWindow = windowManager()->addSensingWindowHandler(WINDOW_LENGTH_MS,this,window_handler);

    for(int domain_id = 0; domain_id < info()->freq_domain_list_size; ++domain_id){
        actuate<ACT_FREQ_MHZ>(
                            &(info()->freq_domain_list[domain_id]),
                            actuationRanges<ACT_FREQ_MHZ>(&(info()->freq_domain_list[domain_id])).min);
    }
}


void IdlePowerChecker::window_handler(int wid,PolicyManager *owner)
{
    IdlePowerChecker *self =  dynamic_cast<IdlePowerChecker*>(owner);

    for(int i = 0; i < self->info()->power_domain_list_size; ++i){
        power_domain_info_t &pd = self->info()->power_domain_list[i];

        auto trace = self->_getTraceHandle("power_domain",pd,wid);

        trace("power_w") = sense<SEN_POWER_W>(&pd,wid);
        double total_time_s = 0;
        double busy_time_s = 0;
        for(int core = 0; core < self->info()->core_list_size; ++core){
            core_info_t &c = self->info()->core_list[core];
            if(c.power->domain_id == pd.domain_id){
                total_time_s += sense<SEN_TOTALTIME_S>(&c,wid);
                busy_time_s += sense<SEN_BUSYTIME_S>(&c,wid);
            }
        }
        trace("total_time_s") = total_time_s;
        trace("busy_time_s") = busy_time_s;
    }
    for(int i = 0; i < self->info()->freq_domain_list_size; ++i){
        freq_domain_info_t &fd = self->info()->freq_domain_list[i];
        auto trace = self->_getTraceHandle("freq_domain",fd,wid);
        trace("freq_mhz") = sense<SEN_FREQ_MHZ>(&fd,wid);
    }

    bool changed = false;
    for(int i = 0; i < self->info()->freq_domain_list_size; ++i){
        freq_domain_info_t &fd = self->info()->freq_domain_list[i];
        auto val = actuationVal<ACT_FREQ_MHZ>(&fd);
        if(self->_state == INCREASING){
            val += actuationRanges<ACT_FREQ_MHZ>(&fd).steps;
            if(val <= actuationRanges<ACT_FREQ_MHZ>(&fd).max) {
                actuate<ACT_FREQ_MHZ>(&fd, val);
                changed = true;
            }
        }
        else if(self->_state == DECREASING){
            val -= actuationRanges<ACT_FREQ_MHZ>(&fd).steps;
            if(val >= actuationRanges<ACT_FREQ_MHZ>(&fd).min) {
                actuate<ACT_FREQ_MHZ>(&fd, val);
                changed = true;
            }
        }
    }

    if(!changed){
        if(self->_state==INCREASING) self->_state=DECREASING;
        else if(self->_state==DECREASING){
            self->_state=INCREASING;
            self->_iterations += 1;
        }
        if(self->_iterations >= ITERATIONS)
            self->quit();
    }
}


int main(int argc, char * argv[]){
	daemon_setup(argc,argv);
	daemon_run_sys<IdlePowerChecker>();
	return 0;
}
