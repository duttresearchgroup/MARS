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
