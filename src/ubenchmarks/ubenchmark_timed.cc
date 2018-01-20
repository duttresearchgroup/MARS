#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <cstdlib>
#include <cstdio>
using namespace std;

#include "time_util.h"
#include "ubenchmark.h"

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
    long int idle_time = args[1];
    long int fiterations = args[2];
    FuncType ftype  = (FuncType)(args[3]);
    long int run_time = args[4];
    
    int *workbuffer = new int[WORK_BUFFER_SIZE]; 
    int *workout = new int;
    
    if(!workbuffer || !workout){
        printf("Couldn't allocate buffers\n");
        exit(-1);
    }
    
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
      
    run_time = run_time/2;

    int i = 0;

    long int time_now = vitamins_bm_time_us();
    while((vitamins_bm_time_us() - time_now) < run_time){

    	long int exec_start = vitamins_bm_time_us();
    	exec_func(ftype,fiterations,workbuffer,workout);
    	exec_start = vitamins_bm_time_us() - exec_start;
    	exec_time_acc += exec_start;
    	
    	idle_wait_us(idle_time);
    	idle_time_acc += idle_time;	
    	++i;
    }
    
    cpu_num = sched_getcpu();
    sem_wait(print_sem);    
    cout << "Thread IDX: " << idx << " now on CPU "<< cpu_num << endl;        
    sem_post(print_sem);

    time_now = vitamins_bm_time_us();
    while((vitamins_bm_time_us() - time_now) < run_time){

    	long int exec_start = vitamins_bm_time_us();
    	exec_func(ftype,fiterations,workbuffer,workout);
    	exec_start = vitamins_bm_time_us() - exec_start;
    	exec_time_acc += exec_start;
    	
    	idle_wait_us(idle_time);
    	idle_time_acc += idle_time;	
    	++i;
    }

    
    long int cpu_util = ((exec_time_acc*1000*100)/(idle_time_acc+exec_time_acc))/1000;

    cpu_num = sched_getcpu();
    sem_wait(print_sem);    
    cout << "Thread IDX: " << idx << " ended on CPU "<< cpu_num <<". Util=" << cpu_util << "% nperiods=" << i << endl;        
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
   
   cout << "main() : running for " << run_time/1000000.0 << " s" << endl;

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

