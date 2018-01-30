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

#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <cstdlib>
#include <cstdio>
using namespace std;

#include "time_util.h"
#include "ubenchmark.h"

#ifdef HAS_BEATS
#include <runtime/uapi/beats.h>

#ifndef BEATS_TGT
#define BEATS_TGT 0
#endif
static task_beat_info_t *_vitamins_info[MAX_NUM_THREADS];
static task_beat_data_t *_vitamins_beats[MAX_NUM_THREADS];

static inline void beats(int idx){
    if(_vitamins_beats[idx])
        task_beat(_vitamins_beats[idx]);
}

static inline void beats_setup(int idx){
        _vitamins_beats[idx] = 0;
        _vitamins_info[idx] = task_beat_register_task();
        if(_vitamins_info[idx]){
            _vitamins_beats[idx] = task_beat_create_domain(_vitamins_info[idx],BEAT_PERF,(uint64_t)(BEATS_TGT));
            if(!_vitamins_beats[idx])
                printf("Cannot setup beats!\n");
            else {
               printf("Beats setup with rate = %d beats/s\n",BEATS_TGT);
            }
                
        }   
        else {
           printf("Cannot use connect to beats sensing module!\n");
        } 
}
#else
static inline void beats(int idx){}
static inline void beats_setup(int idx){}
#endif


sem_t *print_sem;

sem_t *task_init_start;
sem_t *task_init_done;
sem_t *task_wait_begin;
sem_t *task_end;
sem_t *task_main_end;


void idle_wait_us(long int time){
    if(time <= 0) return;
    struct timespec req;
    req.tv_sec = time/1000000;
    req.tv_nsec = (time%1000000) * 1000;
    nanosleep(&req, NULL);
}

void *thread_func(void *_args)
{

    long int* args = (long int*)_args;

    long int idx = args[0];
    long int periodTime = args[1];
    long int computeIterations = args[2];
    FuncType ftype  = (FuncType)(args[3]);
    long int numPeriods = args[4];
    
    int *workbuffer = new int[WORK_BUFFER_SIZE]; 
    int *workout = new int;
    
    if(!workbuffer || !workout){
        printf("Couldn't allocate buffers\n");
        exit(-1);
    }
    
    beats_setup(idx);
    
    sem_wait(&(task_init_start[idx]));
   
    sem_wait(print_sem);
    int cpu_num = sched_getcpu();
    cout << "Thread idx " << idx << " type ";
        switch(ftype){
        case HIGH_IPC: 
            cout << "HIGH_IPC"; break;
        case LOW_IPC_CACHE_MISS:
            cout << "LOW_IPC_CACHE_MISS"; break;
        case LOW_IPC_LOW_ILP:
            cout << "LOW_IPC_LOW_ILP"; break;
        default:
            cout << "unknown"; break;
        }
    cout << ": running on CPU " << cpu_num << endl;
    sem_post(print_sem);
  
  
    sem_post(&(task_init_done[idx]));


    sem_wait(&(task_wait_begin[idx]));
    
    long int idle_time_acc = 0;
    long int exec_time_acc = 0;
      
    long int timeLeft = periodTime;
    for (int i = 0; i < numPeriods; ++i){
        if(timeLeft >0){
            idle_wait_us(timeLeft);
            idle_time_acc += timeLeft;	
        }
        
        long int exec_start = vitamins_bm_time_us();
    	exec_func(ftype,computeIterations,workbuffer,workout);
    	beats(idx);
    	exec_start = vitamins_bm_time_us() - exec_start;
    	exec_time_acc += exec_start;
    	
    	timeLeft = periodTime - exec_start;
    }
    
    long int cpu_util = ((exec_time_acc*1000*100)/(idle_time_acc+exec_time_acc))/1000;

    cpu_num = sched_getcpu();
    sem_wait(print_sem);    
    cout << "Thread IDX: " << idx << " ended on CPU "<< cpu_num <<". Util=" << cpu_util << "%" << endl;        
    sem_post(print_sem);

    delete[] workbuffer;
    delete workout;

    sem_post(&(task_end[idx]));

    sem_wait(&(task_main_end[idx]));
   
    return NULL;
}


void util_ipc_test(long int run_time, int NUM_THREADS, long int thread_args[MAX_NUM_THREADS][5]){

   print_sem = new sem_t;
   sem_init(print_sem, 0, 1);

   task_init_done = new sem_t[NUM_THREADS];
   task_init_start = new sem_t[NUM_THREADS];
   task_wait_begin = new sem_t[NUM_THREADS];
   task_end = new sem_t[NUM_THREADS];
   task_main_end = new sem_t[NUM_THREADS];

      
   pthread_t threads[NUM_THREADS];
   
   cout << "main() : running on CPU " << sched_getcpu() << endl;
   
   for(int i=0; i < NUM_THREADS; i++ ){
      cout << "main() : creating thread " << i << endl;
      
      sem_init(&(task_init_done[i]), 0, 0);
      sem_init(&(task_init_start[i]), 0, 0);
      sem_init(&(task_wait_begin[i]), 0, 0);
      sem_init(&(task_end[i]), 0, 0);
      sem_init(&(task_main_end[i]), 0, 0);

      int rc = pthread_create(&threads[i], NULL, 
                      thread_func, (void*)thread_args[i]);
      if (rc){
         cout << "Error:unable to create thread," << rc << endl;
         exit(-1);
      }
   }
   
   for(int i=0; i < NUM_THREADS; i++ ) sem_post(&(task_init_start[i]));
   for(int i=0; i < NUM_THREADS; i++ ) sem_wait(&(task_init_done[i]));

   long int total_runtime = vitamins_bm_time_us();

   for(int i=0; i < NUM_THREADS; i++ ) sem_post(&(task_wait_begin[i]));

   for(int i=0; i < NUM_THREADS; i++ ) sem_wait(&(task_end[i]));

   cout << "main() : total runtime =  " << (vitamins_bm_time_us()-total_runtime)/1000000.0 << " s" << endl;

   for(int i=0; i < NUM_THREADS; i++ ) sem_post(&(task_main_end[i]));

   for(int i=0; i < NUM_THREADS; i++ ){
      pthread_join(threads[i], NULL);
   }       

}

