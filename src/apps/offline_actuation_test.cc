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

#include <cmath>
#include <sstream>
#include <unistd.h>

#include <runtime/framework/actuation_interface.h>
#include <runtime/framework/policy.h>
#include <runtime/interfaces/offline/trace_simulator.h>

// This policy assumes two domains and migrates tasks between the first
// core of each domain
class TaskMappingPolicy : public Policy {

  public:

    TaskMappingPolicy() :Policy(200,"TaskMappingPolicy") {}

    void execute(int wid) override {
        pinfo(" %04d TaskMappingPolicy(wid=%d, p=%d)\n",
                currExecTimeMS(),wid,periodMS());

        assert_true(info()->freq_domain_list_size == 2);
        assert_true(info()->freq_domain_list[0].core_cnt >= 1);
        assert_true(info()->freq_domain_list[1].core_cnt >= 1);

        core_info_t *core0 = info()->freq_domain_list[0].__vitaminslist_head_cores;
        core_info_t *core1 = info()->freq_domain_list[1].__vitaminslist_head_cores;

        // All tasks in the first core of the first domain
        const PerformanceData &data = PerformanceData::localData();
        for(int i = 0; i < data.numCreatedTasks(); ++i){
            const tracked_task_data_t &task = data.task(i);
            if(!task.task_finished){
                core_info_t *newCore = (sense<SEN_LASTCPU>(&task) == core0->position) ? core1 : core0;
                pinfo(" %04d TaskMappingPolicy(wid=%d, p=%d) migrating task %d to core %d\n",
                        currExecTimeMS(),wid,periodMS(),i,newCore->position);
                actuate<ACT_TASK_MAP>(&task,newCore);
            }
        }
    }
};

// Switch between minimum and maximum freqs
class DVFSPolicy : public Policy {

  public:

    DVFSPolicy() :Policy(50,"DVFSPolicy") {}

    void execute(int wid) override {
        pinfo(" %04d DVFSPolicy(wid=%d, p=%d)\n",
                currExecTimeMS(),wid,periodMS());
        for(int i = 0; i < info()->freq_domain_list_size; ++i){
            freq_domain_info_t &fd = info()->freq_domain_list[i];
            auto &ranges = actuationRanges<ACT_FREQ_MHZ>(&fd);
            int newVal = (actuationVal<ACT_FREQ_MHZ>(&fd) == ranges.min) ? ranges.max : ranges.min;
            pinfo(" %04d DVFSPolicy(wid=%d, p=%d) setting fd %d freq to %d\n",
                    currExecTimeMS(),wid,periodMS(),fd.domain_id,newVal);
            actuate<ACT_FREQ_MHZ>(&fd,newVal);
        }
    }
};


class TestManager : public PolicyManager {

  protected:

    void setup() override {

        assert_true(info()->freq_domain_list_size == 2);

        registerPolicy(new TaskMappingPolicy());
        registerPolicy(new DVFSPolicy());
    }

  public:
    TestManager(SensingModule *sm) :PolicyManager(sm){}

};


int main(int argc, char * argv[]){

    TraceSimulator::core_sim_conf_t coreConf;
    coreConf[COREARCH_Exynos5422_BIG] = 1;
    coreConf[COREARCH_Exynos5422_LITTLE] = 1;

    TraceSimulator::task_sim_conf_t taskConf;
    taskConf["sh"]  = 1;

    TraceParser parser({"traces_test"});

    TraceSimulator sim(coreConf,taskConf,parser.traces());
    sim.addOption("model_path","models/arm_exynos5422");

    sim.simulate<TestManager>(500);

    return 0;
}
