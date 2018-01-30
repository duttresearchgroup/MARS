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
#include <sstream>
#include <stdlib.h>
#include <cassert>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <stddef.h>
using namespace std;

inline long int time_us(){
	struct timeval now;
	gettimeofday(&now, NULL);
	long int us_now = now.tv_sec * 1000000 + now.tv_usec;
	return us_now;
}



int *buffer = 0;
int buffer_size = 0;
int number_threads = 0;
pthread_t *threads = 0;
sem_t *print_sem;
sem_t *buffer_sem;
int *thread_begin_idx;
int *thread_end_idx;


sem_t *thread_init_start;
sem_t *thread_init_done;
sem_t *thread_wait_begin;
sem_t *thread_end;
sem_t *thread_validate_begin;

sem_t *write_rdy;
sem_t *read_rdy;

void init(int argc, char *argv[]);
void cleanup();
void run_tests(int threads);

int main (int argc, char *argv[])
{

    long int time  = time_us();
    
    init(argc,argv);
    
    run_tests(1);
    cout << "\n" << endl;
    run_tests(number_threads);
    
    cout << "\n\nRunning on a different buffer" << endl;
    int *oldbuffer = buffer;
    buffer = new int[buffer_size];
    assert(buffer!=0);
    run_tests(number_threads);
    
    cout << "\n\nRunning again on the original buffer" << endl;
    delete[] buffer;
    buffer = oldbuffer;
    run_tests(number_threads);
    
    cleanup();
    
    time = time_us()-time;
    
    cout << "\nFinished in " << time/1000 << " ms" << endl;
    
    
    
    return 0;
}


void init(int argc, char *argv[]){
    if((argc != 3)) {
        cout << "You must provide BUFFER_SIZE and N_THREADS as arguments\n";
        exit(-1);
    }
   

    istringstream(argv[1]) >> buffer_size;
    istringstream(argv[2]) >> number_threads;
    
    assert(buffer_size >= 1);
    assert(number_threads >= 1);
    
    buffer = new int[buffer_size];
    assert(buffer!=0);
    
    threads = new pthread_t[number_threads];
    assert(threads!=0);
    
    print_sem = new sem_t;
    sem_init(print_sem, 0, 1);
    
    buffer_sem = new sem_t;
    sem_init(buffer_sem, 0, 1);
    
    thread_begin_idx = new int[number_threads];
    thread_end_idx = new int[number_threads];
    
    
    thread_init_done = new sem_t[number_threads];
    thread_init_start = new sem_t[number_threads];
    thread_wait_begin = new sem_t[number_threads];
    thread_end = new sem_t[number_threads];
    thread_validate_begin = new sem_t[number_threads];
    
    for(int i=0; i < number_threads; i++ ){
      sem_init(&(thread_init_done[i]), 0, 0);
      sem_init(&(thread_init_start[i]), 0, 0);
      sem_init(&(thread_wait_begin[i]), 0, 0);
      sem_init(&(thread_end[i]), 0, 0);
      sem_init(&(thread_validate_begin[i]), 0, 0);
    }
    
    write_rdy = new sem_t;
    sem_init(write_rdy, 0, 0);
    
    read_rdy = new sem_t;
    sem_init(read_rdy, 0, 0);
}

void cleanup(){
    delete[] buffer;
    
    delete[] threads;
    
    delete print_sem;  
    delete buffer_sem;
    
    delete[] thread_begin_idx;
    delete[] thread_end_idx;
    
    delete[] thread_init_done;
    delete[] thread_init_start;
    delete[] thread_wait_begin;
    delete[] thread_end;
    delete[] thread_validate_begin;    
}


int test_func(int bufferIdx){
    return (buffer_size-bufferIdx)*number_threads;
}

int validate(int begin, int end){
    assert(begin >= 0);
    assert(end <= buffer_size);
    
    for (int i = begin; i < end; ++i)
        if(buffer[i] != test_func(i)) return i;
    return -1;
}


int curr_idx = 0;
void reset_idx() {
    sem_wait(buffer_sem);    
    curr_idx = 0;
    sem_post(buffer_sem);
}
int next_idx(){
    sem_wait(buffer_sem);    
    int idx = curr_idx;
    if (curr_idx < buffer_size) curr_idx += 1;
    sem_post(buffer_sem);
    return idx;
}
int get_idx(){
    sem_wait(buffer_sem);    
    int idx = curr_idx;
    sem_post(buffer_sem);
    return idx;
}


const int NEXT_READER = 0;
const int NEXT_WRITER = 1;
const int NEXT_WAIT = 2;

int role = NEXT_READER;

void reset_role(){
    sem_wait(buffer_sem);
    role = NEXT_READER;
    sem_post(buffer_sem);
}

int next_role(){
    sem_wait(buffer_sem);
    int ret = role;
    if (role == NEXT_READER) role = NEXT_WRITER;
    else if (role == NEXT_WRITER) role = NEXT_WAIT;  
    sem_post(buffer_sem);
    return ret;

}

struct test_struct {
    virtual ~test_struct(){}
    int thread_idx;
    virtual void run_test() = 0;
};

struct test_struct_step : public test_struct {
    void run_test(){
        int bidx = next_idx();
        while(bidx != buffer_size){
            buffer[bidx] = test_func(bidx);
            assert(buffer[bidx]==test_func(bidx));
            bidx = next_idx();
        }
    }
    
    virtual ~test_struct_step(){}
};

struct test_struct_blk : public test_struct  {
    int blk_begin;
    int blk_end;
    void _test_blk(int begin, int end){
        assert(begin >= 0);
        assert(end <= buffer_size);
        sem_wait(print_sem);    
        cout << "Thread " << thread_idx << " got blk "<< blk_begin << "-" << blk_end <<  endl;        
        sem_post(print_sem);
    
        for (int i = begin; i < end; ++i) buffer[i] = test_func(i);
    }
    
    void run_test(){
        _test_blk(blk_begin,blk_end);
    }
    
    virtual ~test_struct_blk(){}
};

struct test_struct_cyclic : public test_struct  {
    void run_test() {
        while(curr_idx < buffer_size) {
            int r = next_role();
            if(curr_idx >= buffer_size) return;
            if (r == NEXT_READER){
                buffer[curr_idx] = 125;
                assert(buffer[curr_idx] == 125);
                sem_post(write_rdy);
                sem_wait(read_rdy);
                assert(buffer[curr_idx] == test_func(curr_idx));
                
                ++curr_idx;
                reset_role();
            }
            else if (r == NEXT_WRITER){
                sem_wait(write_rdy);
                buffer[curr_idx] = test_func(curr_idx);
                sem_post(read_rdy);
            }
        };
    }
    virtual ~test_struct_cyclic(){}
};


void* test_thread(void *_args){
    test_struct *test_info = (test_struct*)_args;

    int idx = test_info->thread_idx;
    
    sem_wait(&(thread_init_start[idx]));
    
    int cpu_num = sched_getcpu();
    
    sem_wait(print_sem);    
    cout << "Thread " << idx << " on CPU "<< cpu_num << " running" <<  endl;        
    sem_post(print_sem);
    
    sem_post(&(thread_init_done[idx]));

    sem_wait(&(thread_wait_begin[idx]));
    
    test_info->run_test();
 
    sem_wait(print_sem);    
    cout << "Thread " << idx << " on CPU "<< cpu_num << " is done" <<  endl;        
    sem_post(print_sem);
    
    sem_post(&(thread_end[idx]));

    sem_wait(&(thread_validate_begin[idx]));
    
    sem_wait(print_sem);    
    cout << "Thread " << idx << " on CPU "<< cpu_num << " is validating" <<  endl;        
    sem_post(print_sem);

    int result = validate(0,buffer_size);
    if (result != -1){
        sem_wait(print_sem);    
        cout << "Thread " << idx << " on CPU "<< cpu_num << ". Validation failed: buffer[" << result << "]==" << buffer[result] << ", should be " << test_func(result) <<  endl;        
        sem_post(print_sem);
    }
    else{
        sem_wait(print_sem);    
        cout << "Thread " << idx << " on CPU "<< cpu_num << ". Validation succesfull" << endl;        
        sem_post(print_sem);
    }
    assert(result == -1);
    
    delete test_info;
    
    return 0;
}


void create_thread_step_test(int tnum){
    for(int i=0; i < tnum; i++ ){
        test_struct *ts = new test_struct_step;    
        ts->thread_idx = i;
      
        int rc = pthread_create(&threads[i], NULL, 
                      test_thread, (void*)ts);
        if (rc){
            cout << "Error:unable to create thread," << rc << endl;
            exit(-1);
        }
   }
}

void create_thread_blk_test(int tnum){
    for(int i=0; i < tnum; i++ ){
        test_struct_blk *ts = new test_struct_blk;    
        ts->thread_idx = i;
        ts->blk_begin = i*(buffer_size/tnum);
        ts->blk_end = (i+1)*(buffer_size/tnum);
        if(i == (tnum-1)) ts->blk_end = buffer_size;
      
        int rc = pthread_create(&threads[i], NULL, 
                      test_thread, (void*)ts);
        if (rc){
            cout << "Error:unable to create thread," << rc << endl;
            exit(-1);
        }
   }
}

void create_thread_cyclic_test(int tnum){
    for(int i=0; i < tnum; i++ ){
        test_struct *ts = new test_struct_cyclic;    
        ts->thread_idx = i;
      
        int rc = pthread_create(&threads[i], NULL, 
                      test_thread, (void*)ts);
        if (rc){
            cout << "Error:unable to create thread," << rc << endl;
            exit(-1);
        }
   }
}

void run_threads(int tnum){

   for(int i = 0; i < buffer_size; ++i) {
       buffer[i] = time_us();
       if(buffer[i] == test_func(i)) ++buffer[i];
   }
   reset_idx();
   reset_role();

   for(int i=0; i < tnum; i++ ) sem_post(&(thread_init_start[i]));
   for(int i=0; i < tnum; i++ ) sem_wait(&(thread_init_done[i]));

   for(int i=0; i < tnum; i++ ) sem_post(&(thread_wait_begin[i]));

   for(int i=0; i < tnum; i++ ) sem_wait(&(thread_end[i]));

   for(int i=0; i < tnum; i++ ) sem_post(&(thread_validate_begin[i]));

   for(int i=0; i < tnum; i++ ){
      pthread_join(threads[i], NULL);
   }
   
}


void run_tests(int tnum){
    cout << "Running tests for " << tnum << "\n";
    
    cout << "Step tests\n";
    create_thread_step_test(tnum);
    run_threads(tnum);

    cout << "\nBlk tests\n";
    create_thread_blk_test(tnum);
    run_threads(tnum);
    
    if (tnum < 2) return;
    cout << "\nCyclic tests\n";
    create_thread_cyclic_test(tnum);
    run_threads(tnum);
}

