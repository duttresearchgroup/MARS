/*******************************************************************************
 * Copyright (C) 2018 Biswadip Maity <biswadip.maity@gmail.com>
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
#include <runtime/framework/actuation_interface.h>
#include <runtime/common/strings.h>
#include <runtime/common/perffilter.h>
#include <unordered_set>

class BeatsMonitor : public PolicyManager {
protected:
    static const int WINDOW_LENGTH = 50;

    virtual void setup();

    const SensingWindowManager::WindowInfo *sensingWindow;

    static void window_handler(int wid,PolicyManager *owner);

    ExecutionTrace _execTrace;

    PerfFilter _perffilter;


public:
    BeatsMonitor(SensingModule *sm) :PolicyManager(sm),
        sensingWindow(nullptr), _execTrace("execTrace"){}

};

void BeatsMonitor::setup()
{
    sensingModule()->enablePerTaskSensing();
    sensingWindow = windowManager()->addSensingWindowHandler(WINDOW_LENGTH,this,window_handler);

    _perffilter.beatsFilter(WINDOW_LENGTH);
}

void BeatsMonitor::window_handler(int wid,PolicyManager *owner)
{
    BeatsMonitor *self =  dynamic_cast<BeatsMonitor*>(owner);

    auto sensedData = self->sensingModule()->data();
    auto trace = self->_execTrace.getHandle(sensedData,wid);

    //beats
    self->_perffilter.sampleTasks(wid, owner);
    // trace("beatsTasks_beatsAcc") = self->_perffilter.beatsAcc();
    trace("beatsTasks_beats") = self->_perffilter.beats();
    trace("beatsTasks_beatsFiltered") = self->_perffilter.beatsFiltered();
    trace("beatsTasks_ips") = self->_perffilter.ips();
    trace("beatsTasks_ipsFiltered") = self->_perffilter.ipsFiltered();

    //save total power
    double totalPowerW = 0;
    for(int domain_id = 0; domain_id < owner->info()->power_domain_list_size; ++domain_id){
        totalPowerW += sense<SEN_POWER_W>(&owner->info()->power_domain_list[domain_id],wid);
    }
    trace("total_power_w") = totalPowerW;

    //save freqs
    assert_true(owner->info()->freq_domain_list_size == 2);
    trace("fd0_freq_mhz") = sense<SEN_FREQ_MHZ>(&owner->info()->freq_domain_list[0],wid);
    trace("fd1_freq_mhz") = sense<SEN_FREQ_MHZ>(&owner->info()->freq_domain_list[1],wid);

    //number of tasks per domain
    int fd0_tsks = 0; int fd1_tsks = 0; int tsks = 0;
    for(int i = 0; i < sensedData.numCreatedTasks(); ++i){
        if((sensedData.task(i).num_beat_domains > 0) || (sensedData.task(i).parent_has_beats)){
            int cpu = sense<SEN_LASTCPU>(&sensedData.task(i),wid);
            auto instr = sense<SEN_PERFCNT>(PERFCNT_INSTR_EXE,&sensedData.task(i),wid);
            if((cpu >= 0) && (cpu < owner->info()->core_list_size) && (instr > 0)){
                if(owner->info()->core_list[cpu].freq->domain_id == 0)
                    fd0_tsks += 1;
                else
                    fd1_tsks += 1;
            }
            if(!sensedData.task(i).task_finished)
                tsks += 1;
        }
    }

    // beats tasks that ran in each domain
    trace("fd0_beatsTasks") = fd0_tsks;
    trace("fd1_beatsTasks") = fd1_tsks;

    // total number of beats tasks still alive (regardless if they run or not in the last window)
    trace("beatsTasks") = tsks;
}


int main(int argc, char * argv[]){
    daemon_run<BeatsMonitor>(argc,argv);
	return 0;
}
