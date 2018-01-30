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

#ifndef VITAMINS_UTIL_IPC_TEST_H
#define VITAMINS_UTIL_IPC_TEST_H

const int MAX_NUM_THREADS = 32;

const int WORK_BUFFER_SIZE = (1*1024*1024);

enum FuncType{
    HIGH_IPC = 0,
    LOW_IPC_CACHE_MISS = 1,
    LOW_IPC_LOW_ILP = 2
};

void util_ipc_test(long int run_time, int NUM_THREADS, long int thread_args[MAX_NUM_THREADS][5]);

void exec_func(FuncType type, int iterations, int *workbuffer, int *workout);

#endif

