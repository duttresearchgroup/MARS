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

#include <runtime/daemon/deamonizer.h>
#include <runtime/common/reports.h>
#include <runtime/framework/actuation_interface.h>
#include "../runtime/framework/models/baseline_model.h"

// Always increases ACT_DUMMY1
class DummyPolicy1 : public Policy {

  public:

    DummyPolicy1() :Policy(100,"DummyPolicy1") {}

    void execute(int wid) override {
        std::stringstream ss; for(int i = 0; i < nesting(); ++i) ss << "    ";
        pinfo("%s %04d DummyPolicy1(wid=%d p=%dms) as %s: dummyVal=%f\n",
                ss.str().c_str(),
                currExecTimeMS(),
                wid,periodMS(),
                ReflectiveEngine::isReflecting() ? "MODEL" : "POLICY",
                sense<SEN_DUMMY>(nullResource()));

        actuate<ACT_DUMMY1>(nullResource(),actuationVal<ACT_DUMMY1>(nullResource())+1);
    }
};

// Tries to maintain SEN_DUMMY value=5 by setting ACT_DUMMY2
class DummyPolicy2 : public Policy {

  public:

    DummyPolicy2() :Policy(250,"DummyPolicy2") {}

    void execute(int wid) override {
        //SensingModule::get().sensingStop();
        std::stringstream ss; for(int i = 0; i < nesting(); ++i) ss << "    ";
        pinfo("%s %04d DummyPolicy2(wid=%d p=%dms) as %s: dummyVal=%f\n",
                ss.str().c_str(),
                currExecTimeMS(),
                wid,periodMS(),
                ReflectiveEngine::isReflecting() ? "MODEL" : "POLICY",
                sense<SEN_DUMMY>(nullResource()));

        // Non-reflective version
        //actuate<ACT_DUMMY2>(nullResource(),
        //        actuationVal<ACT_DUMMY2>(nullResource()) +
        //        (5 - sense<SEN_DUMMY>(nullResource()))
        //);

        // Reflective version
        pinfo("  %s %04d DummyPolicy2(wid=%d p=%dms): predicted dummyVal=%f\n",
                        ss.str().c_str(),
                        currExecTimeMS(),
                        wid,periodMS(),
                        senseIf<SEN_DUMMY>(nullResource()));

        constexpr int maxIters = 4;
        int act2Val = 0; int iters = 0;
        while(std::fabs(senseIf<SEN_DUMMY>(nullResource())-5) > 0.5){
            act2Val = tryActuationVal<ACT_DUMMY2>(nullResource()) + (5 - senseIf<SEN_DUMMY>(nullResource()));

            pinfo("  %s %04d DummyPolicy2(wid=%d p=%dms): act2Val=%d because dummyVal=%f\n",
                    ss.str().c_str(),
                    currExecTimeMS(),
                    wid,periodMS(),
                    act2Val,
                    senseIf<SEN_DUMMY>(nullResource()));

            tryActuate<ACT_DUMMY2>(nullResource(), act2Val);

            if(++iters >= maxIters) break;
        }
        if(iters > 0)
            actuate<ACT_DUMMY2>(nullResource(),act2Val);
    }
};

// Does nothing
class DummyModel : public Model {

  public:

    DummyModel() :Model(50,"DummyModel") {}

    void execute(int wid) override {
        std::stringstream ss; for(int i = 0; i < nesting(); ++i) ss << "    ";
        pinfo("%s %04d DummyModel(wid=%d p=%dms) as %s: dummyVal=%f\n",
                ss.str().c_str(),
                currExecTimeMS(),
                wid,periodMS(),
                ReflectiveEngine::isReflecting() ? "MODEL" : "POLICY",
                sense<SEN_DUMMY>(nullResource()));
    }

};

class PolicyTestManager : public PolicyManager {

  protected:

    void setup() override {
        //pinfo("Waiting 10s for GDB to attach\n");
        //sleep(10);
        registerPolicy(new DummyPolicy1());
        registerPolicy(new DummyPolicy2());
        registerModel(new DummyModel());

        enableReflection();
    }

  public:
    PolicyTestManager(SensingModule *sm) :PolicyManager(sm){}

};


int main(int argc, char * argv[]){
    daemon_run<PolicyTestManager>(argc,argv);
    return 0;
}
