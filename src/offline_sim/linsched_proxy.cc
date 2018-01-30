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

#include "linsched_proxy.h"
#include <linsched_interface.h>
#include <mutex>


static std::mutex glb_mutex;
static volatile bool do_init = true;

Linsched::Linsched(int numcpus)
{
    glb_mutex.lock();

    if(do_init) {
        linsched_setup(numcpus);
        do_init = false;
    }
    linsched_reset(numcpus);
}

Linsched::~Linsched(){
    glb_mutex.unlock();
}

int
Linsched::create_task(double run, double sleep, int core, int niceval)
{
    return linsched_create_task(run,sleep,core,niceval);
}

void
Linsched::sim(double time)
{
    linsched_sim(time);
}

void
Linsched::task_info(int task, struct task_sched_info *info)
{
    linsched_task_info(task,info);
}

void
Linsched::print_info()
{
    linsched_print_info();
}




