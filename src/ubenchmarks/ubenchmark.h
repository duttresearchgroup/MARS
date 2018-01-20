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

