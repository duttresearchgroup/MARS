#include <iostream>
#include <sstream>
#include <stdlib.h>
using namespace std;

#include "ubenchmark.h"

int main (int argc, char *argv[])
{

    
    if((argc < 5)) {
        cout << "You must provide at least T0_NUM_PERIODS T0_PERIOD_US T0_COMPUTE_ITERATIONS T0_TYPE ... as arguments\n";
        exit(-1);
    }
    
  int NUM_THREADS = (argc-1)/4;
  
   
   long int thread_args[MAX_NUM_THREADS][5];
   
   for(int i=0; i < NUM_THREADS; ++i ){
      
      int arg_idx_numPeriods = (i*4)+1;
      int arg_idx_idle = (i*4)+2;
      int arg_idx_iter = (i*4)+3;
      int arg_idx_type = (i*4)+4;
      
      istringstream ss_numPeriods(argv[arg_idx_numPeriods]);
      istringstream ss_idle(argv[arg_idx_idle]);
      istringstream ss_iter(argv[arg_idx_iter]); 
      istringstream ss_type(argv[arg_idx_type]);
      
      thread_args[i][0] = i;
      if (!(ss_idle >> thread_args[i][1])) {
        cerr << "Invalid thread idle time " << argv[arg_idx_idle] << '\n';
        exit(-1);
      }
      if (!(ss_iter >> thread_args[i][2])) {
        cerr << "Invalid thread iteratio " << argv[arg_idx_iter] << '\n';
        exit(-1);
       }
      if (!(ss_type >> thread_args[i][3])) {
        cerr << "Invalid thread type " << argv[arg_idx_type] << '\n';
        exit(-1);
       }       
      if (!(ss_numPeriods >> thread_args[i][4])) {
        cerr << "Invalid number of periods " << argv[arg_idx_numPeriods] << '\n';
        exit(-1);
       }       
   }

   util_ipc_test(0,NUM_THREADS, thread_args);
   
   return 0;
}
