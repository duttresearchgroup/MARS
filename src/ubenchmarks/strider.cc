#include <iostream>
#include <sstream>
#include <cassert>
#include <stdlib.h>
#include "time_util.h"

int buffer_size;
int int_buffer_size;
long int max_iters = 0;
const int max_stride = 1024;
int stride = 1;

int *buffer1;
int *buffer2;
volatile int *result;

double buffer_op(){
    long int exec_start = vitamins_bm_time_us();
    int i = 0;
    for(long int iter  = 0; iter < max_iters; ++iter){
        buffer2[i] = buffer1[i];
        *result += buffer2[i];
        i = (i + stride) % int_buffer_size;
    }

    return (double)(vitamins_bm_time_us()-exec_start)/1000000;
}

template<typename T>
T parseval(const char *str){
    T val;
    std::istringstream ss1(str);
    ss1 >> val;
    return val;
}

void training(){
    printf("Training\n");
    
    double best_stride = buffer_op();
    int best = stride;
    int best_prev = stride;
    
   // int iters_no_best = 0;
    
    for(;stride <= max_stride; stride *= 2){
        double curr = buffer_op();
        printf("\t run with stride = %d time = %f\n",stride,curr);
        if(curr >= best_stride){
            best_prev = best;
            best_stride = curr;
            best = stride;
            printf("%d is the best stride with time %f\n",stride,curr);
        }
        //else iters_no_best += 1;
        //if(iters_no_best > 2) break;
    }
    
    while(true){
        stride = (best + best_prev) / 2;
        double curr = buffer_op();
        printf("\t run with stride = %d time = %f\n",stride,curr);
        if(curr >= best_stride){
            best_prev = best;
            best_stride = curr;
            best = stride;
            printf("%d is the best stride with time %f\n",stride,curr);
        }
        else break;
    }
    
    printf("Training done. Best stride was %d\n",best);
    stride = best;
 
}

void running(){
    printf("Running with stride = %d\n",stride);
    double curr = buffer_op();
    printf("Finished in %f secs\n",curr);

}

int main (int argc, char *argv[])
{
    bool train = parseval<bool>(argv[1]);
  
    max_iters = parseval<long int>(argv[2])*1000*1000;
    buffer_size = parseval<int>(argv[3]);

    int_buffer_size = (buffer_size / sizeof(int))/2;
    
    buffer1 = new int[int_buffer_size];
    buffer2 = new int[int_buffer_size];
    result = new int;
    

    for(int i = 0; i < int_buffer_size; ++i){
        buffer1[i] = i;
    }
    *result = 0;
    
    assert(int_buffer_size > (max_stride+1));
    
    if(train) training();
    else {
        stride = parseval<int>(argv[4]);
        running();
    }
    
       


    
    
    delete[] buffer1;
    delete[] buffer2;
    delete result;


    return *result;
    
}
