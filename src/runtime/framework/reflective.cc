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

#include "reflective.h"
#include <runtime/framework/sensing_interface.h>
#include <runtime/interfaces/sensing_module.h>
#include <runtime/framework/policy.h>

thread_local ReflectiveEngine::Context ReflectiveEngine::_currentContext;

ReflectiveEngine* ReflectiveEngine::_currentReflectiveEngine = nullptr;


void ReflectiveEngine::addToRscNeedingPredImpl(const freq_domain_info_t *rsc)
{
    core_info_t *c;
    for_each_in_internal_list(rsc,cores,c,freq_domain){
        _baselineModel.sharingModel().actuationNotify(c);
    }

    if(rscNeedingPred.find(rsc) != rscNeedingPred.end()) return;

    rscNeedingPred.insert(rsc);

    //adds all cores
    for_each_in_internal_list(rsc,cores,c,freq_domain){
        addToRscNeedingPredImpl(c);
    }

    //adds all power_domains
    power_domain_info_t *p;
    for_each_in_internal_list(rsc,power_domains,p,freq_domain){
        addToRscNeedingPredImpl(p);
    }
}

void ReflectiveEngine::addToRscNeedingPredImpl(const core_info_t *rsc)
{
    _baselineModel.sharingModel().actuationNotify(rsc);

    if(rscNeedingPred.find(rsc) != rscNeedingPred.end()) return;

    rscNeedingPred.insert(rsc);

    //adds all tasks mapped to this core
    const PerformanceData &data = SensingModule::get().data();
    for(int i = 0; i < data.numCreatedTasks(ReflectiveEngine::currentWID()); ++i){
        const tracked_task_data_t *task = &(data.task(i));
        //See if we have a tryActuate for this task map. If not found use the current map
        if(hasNewActuationVal<ACT_TASK_MAP>(task)){
            if(newActuationVal<ACT_TASK_MAP>(task) == rsc)
                addToRscNeedingPredImpl(task);
        }
        else{
            if(SensingInterface::Impl::sense<SEN_LASTCPU>(task,currentWID()) == rsc->position)
                addToRscNeedingPredImpl(task);
        }
    }

    //also adds the freq domain
    addToRscNeedingPredImpl(rsc->freq);
}

void ReflectiveEngine::addToRscNeedingPredImpl(const tracked_task_data_t *rsc)
{
    rscNeedingPred.insert(rsc);

    //also adds the core this task is mapped to (both current and the tryActuate one)
    //and their domain
    if(hasNewActuationVal<ACT_TASK_MAP>(rsc))
        addToRscNeedingPredImpl(newActuationVal<ACT_TASK_MAP>(rsc));

    int cpu = SensingInterface::Impl::sense<SEN_LASTCPU>(rsc,currentWID());
    if((cpu >= 0) && (cpu < _sys_info->core_list_size))
        addToRscNeedingPredImpl(&(_sys_info->core_list[cpu]));
}

void ReflectiveEngine::addToRscNeedingPredImpl(const power_domain_info_t *rsc)
{
    core_info_t *c;
    for_each_in_internal_list(rsc,cores,c,power_domain){
        _baselineModel.sharingModel().actuationNotify(c);
    }

    if(rscNeedingPred.find(rsc) != rscNeedingPred.end()) return;

    rscNeedingPred.insert(rsc);

    //adds freq_domain
    addToRscNeedingPredImpl(rsc->freq_domain);

    //adds all cores
    for_each_in_internal_list(rsc,cores,c,power_domain){
        addToRscNeedingPredImpl(c);
    }
}

void ReflectiveEngine::addToRscNeedingPredImpl(const NullResource *rsc)
{
    if(rscNeedingPred.find(rsc) != rscNeedingPred.end()) return;
    rscNeedingPred.insert(rsc);
}


ReflectiveEngine::Context::PolicyScope::PolicyScope(Policy *pol)
{
    assert_true(_currentContext._currentModel == nullptr);
    _currentContext._currentModel = pol;
}

void ReflectiveEngine::buildSchedule(Model *currModel)
{
    assert_true(currModel != nullptr);

    // This should be called before execution so times are 0
    assert_true(currModel->currExecTimeMS() == 0);

    const PolicyManager *manager = currModel->manager();

    ReflectiveScheduleEntry *se = nullptr;

    //pinfo("**buildSchedule(cm=%s) **\n",currModel->name().c_str());

    // Advances time until to see which models "execute" before currModel
    int currTime = 0;
    while(true){
        currTime += MINIMUM_WINDOW_LENGHT_MS;
        if(currModel->nextExecTimeMS() == currTime) break;
        for(Model *model = manager->modelFiner();
                model != nullptr;
                model = manager->modelNext(model) )
        {
            if(model->nextExecTimeMS() == currTime){
                model->tmpSetCurrExecTimeMS(currTime);
                if(se == nullptr){
                    se = new ReflectiveScheduleEntry;
                    currModel->schedule(se);
                }
                else {
                    se->next = new ReflectiveScheduleEntry;
                    se->next_copy = se->next;
                    se = se->next;
                }
                se->model = model;
                //pinfo("**buildSchedule(cm=%s) adding %s(%d)**\n",currModel->name().c_str(),model->name().c_str(),model->currExecTimeMS());
            }
        }
    }

    for(Model *model = currModel->manager()->modelFiner();
            model != nullptr;
            model = currModel->manager()->modelNext(model) )
    {
        model->restoreCurrExecTimeMS();
    }
}

void ReflectiveEngine::runFinerGrainedModelsImpl(Model *currModel)
{
    // Cannot nest reflective scopes !
    assert_true(!isReflecting());

    ReflectiveScheduleEntry *sched = currModel->schedule();

    if(sched == nullptr) return;

    ReflectiveScheduleEntry *sched_prev = nullptr;
    while(sched != nullptr){
        assert_true(sched->model != nullptr);

        // We might need to swap the order when periods are not aligned
        if((sched->next != nullptr) && (sched->model->nextExecTimeMS() > sched->next->model->nextExecTimeMS())){
            //pinfo("**runFinerGrainedModelsImpl(cm=%s) swaping %s(%d) and %s(%d)**\n",currModel->name().c_str(),
            //                    sched->model->name().c_str(),sched->model->nextExecTimeMS(),
            //                    sched->next->model->name().c_str(),sched->next->model->nextExecTimeMS());
            ReflectiveScheduleEntry *tmp = sched->next;
            sched->next = tmp->next;
            tmp->next = sched;
            if(sched_prev != nullptr) sched_prev->next = tmp;
            sched = tmp;
        }

        {
            ReflectiveEngine::Context::ReflectiveScope rflScp(sched->model->nextExecTimeMS()-currModel->currExecTimeMS());
            ReflectiveEngine::Context::ModelScope mdlScp(sched->model);
            sched->model->tmpSetCurrExecTimeMS(sched->model->nextExecTimeMS());
            sched->model->incNesting();
            sched->model->execute(currentWID());
            sched->model->decNesting();
        }

        sched_prev = sched;
        sched = sched->next;
    }

    // Restore time and swapped nodes
    sched = currModel->schedule();
    while(sched != nullptr){
        sched->model->restoreCurrExecTimeMS();
        sched->next = sched->next_copy;
        sched = sched->next;
    }
}
