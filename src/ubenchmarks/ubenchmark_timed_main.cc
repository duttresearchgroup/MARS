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
        cout << "You must provide at least RUNTIME_US T0_IDLE_T_US T0_ITERATIONS T0_TYPE ... as arguments\n";
        exit(-1);
    }
    
  int NUM_THREADS = (argc-2)/3;
  
  long int run_time;

   istringstream ss1(argv[1]);
   if (!(ss1 >> run_time)) {
        cerr << "Invalid RUNTIME " << argv[1] << '\n';
        exit(-1);
   }
   
   long int thread_args[MAX_NUM_THREADS][5];
   
   for(int i=0; i < NUM_THREADS; i++ ){
      
      int arg_idx_idle = (i*3)+2;
      int arg_idx_iter = (i*3)+3;
      int arg_idx_type = (i*3)+4;
      
      istringstream ss_idle(argv[arg_idx_idle]), ss_iter(argv[arg_idx_iter]), ss_type(argv[arg_idx_type]);
      
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
       thread_args[i][4] = run_time;
   }

   util_ipc_test(run_time,NUM_THREADS, thread_args);
   
   return 0;
}
