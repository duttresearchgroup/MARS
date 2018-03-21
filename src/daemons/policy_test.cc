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
#include <runtime/common/reports.h>
#include <runtime/framework/actuation_interface.h>
#include <runtime/framework/models/hw_model.h>

template<int pMS, Policy::Priority prio>
class DummyPolicy : public Policy {

  public:

    DummyPolicy() :Policy(pMS,prio) {}

    void execute(int wid) override {
        pinfo("Dummy policy p=%dms prio=%d\n",periodMS(),priority());
    }

};

template<int pMS, Policy::Priority prio>
class DummyModel : public Model {

  public:

    DummyModel() :Model(pMS,prio) {}

    void execute(int wid) override {
        pinfo("Dummy model p=%dms prio=%d\n",periodMS(),priority());
    }

};

class PolicyTestManager : public PolicyManager {

  protected:

    void setup() override;

    template<int prio>
    static void dummyHandler(int wid, PolicyManager *owner){
        auto winfo = owner->windowManager()->winfo(wid);
        pinfo("Dummy handler p=%dms prio=%d\n",winfo->period_ms,prio);
    }
};

void PolicyTestManager::setup()
{
    registerPolicy(new DummyPolicy<500,Policy::PRIORITY_MIN>());
    registerPolicy(new DummyPolicy<500,Policy::PRIORITY_MAX>());
    registerPolicy(new DummyPolicy<250,Policy::PRIORITY_DEFAULT>());
    registerPolicy(new DummyPolicy<100,Policy::PRIORITY_DEFAULT>());
    registerModel(new DummyModel<50,Model::PRIORITY_DEFAULT>());
    windowManager()->addSensingWindowHandler(500,this,dummyHandler<Policy::PRIORITY_DEFAULT>,Policy::PRIORITY_DEFAULT);
    windowManager()->addSensingWindowHandler(500,this,dummyHandler<Policy::PRIORITY_MIN>,Policy::PRIORITY_MIN);
    windowManager()->addSensingWindowHandler(500,this,dummyHandler<Policy::PRIORITY_MAX>,Policy::PRIORITY_MAX);
}


int main(int argc, char * argv[]){
    daemon_setup(argc,argv);
    daemon_run_sys<PolicyTestManager>();
    return 0;
}
