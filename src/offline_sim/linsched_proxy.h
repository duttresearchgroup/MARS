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

#ifndef __linsched_proxy_h
#define __linsched_proxy_h

#include <linsched_interface.h>

class Linsched {

public:
    Linsched(int numcpus);
    ~Linsched();

    int create_task(double run, double sleep, int core, int niceval);
    void sim(double time);
    void task_info(int task, struct task_sched_info *info);
    void print_info();

};


#endif
