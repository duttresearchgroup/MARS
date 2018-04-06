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

#include <cstdio>
#include <cassert>
#include <vector>
#include <limits>
#include <sstream>
#include "time_util.h"
#include "stdlib.h"

#include "training.h"

//__ISLITE should be passed using the -D flag
#ifdef __ISLITE
    constexpr bool IS_LITE = true;
#else
    constexpr bool IS_LITE = false;
#endif

constexpr double TARGET_UBENCH_RT_MS = 100.0;

constexpr double CALIB_TOLERANCE_MS = 5.0;

constexpr int CALIB_MAX_ITERS = 10;

typedef void (*ubench_func)(int);

std::vector<ubench_func> benchname;
std::vector<int> benchIters;

void init_vectors()
{
    benchname.push_back(high_ilp_cache_good_int);
    benchIters.push_back(468000);

    benchname.push_back(high_ilp_cache_bad_int);
    benchIters.push_back(48000);

    benchname.push_back(low_ilp_cache_good_int);
    benchIters.push_back(229000);

    benchname.push_back(low_ilp_cache_bad_int);
    benchIters.push_back(190000);

    benchname.push_back(low_ilp_cache_good_float);
    benchIters.push_back(307000);

    benchname.push_back(low_ilp_cache_bad_float);
    benchIters.push_back(88000);

    benchname.push_back(high_ilp_cache_good_float);
    benchIters.push_back(256000);

    benchname.push_back(high_ilp_cache_bad_float);
    benchIters.push_back(180000);

    benchname.push_back(low_ilp_icache_bad);
    benchIters.push_back(190000);

    benchname.push_back(low_ilp_branches_deep);
    benchIters.push_back(350000);

    benchname.push_back(matrix_mult);
    benchIters.push_back(5);
}

template<typename dest, typename src>
dest round_closest(src a){
    return (dest)(a+0.5);
}

static bool calibrate(){
    // sees how many iterations we need, so the runtime of each bench
    // is about TARGET_UBENCH_RT_MS
    bool all_ok = true;
    for(unsigned i = 0; i < benchname.size();++i){
        double time = 0;
        double iters = benchIters[i];
        while(1){
            time = vitamins_bm_time_us();
            (benchname[i])(iters);
            time = ((double)vitamins_bm_time_us() - time)/1000.0;//to ms
            if(time > 1) break;
            iters *= 10; //try again with more if original iterations was not enough for at least 1ms
        }
        double newIters = (TARGET_UBENCH_RT_MS * iters) / time;
        printf("bench %d: time=%f ms , old iters=%d , new iter=%d\n",
                i, time, (int)iters, round_closest<int>(newIters));
        benchIters[i] = round_closest<int>(newIters);
        if(benchIters[i]==0) benchIters[i] = 1;
        if((time > (TARGET_UBENCH_RT_MS+CALIB_TOLERANCE_MS)) || (time < (TARGET_UBENCH_RT_MS-CALIB_TOLERANCE_MS)))
            all_ok = false;
    }
    return all_ok;
}

static void calibration(){
    printf("Calibration mode\n");
    for(int i = 0; i < CALIB_MAX_ITERS; ++i) if(calibrate()) break;
    //print the args to be passed when actually running shit
    printf("calibration_args ");
    for(unsigned i = 0; i < benchname.size();++i){
        printf("%d",benchIters[i]);
        if(i == (benchname.size()-1))
            printf("\n");
        else
            printf(" ");
    }
}

static inline void idle_wait_us(long int time){
    if(time <= 0) return;
    struct timespec req;
    req.tv_sec = time/1000000;
    req.tv_nsec = (time%1000000) * 1000;
    nanosleep(&req, NULL);
}

static inline void run_combination(int r, int *data)
{
    if((r > 1) && IS_LITE) exit(0);//runs only a few iterations for testing. exit now

    //auto time = vitamins_bm_time_us();
    for(int i = 0; i < r; ++i){
        int benchIdx = data[i];
        assert(benchIters[benchIdx]>=r);
        (benchname[benchIdx])(benchIters[benchIdx]/r);
    }
    //this sleep helps us to identify different runs when looking at
    //the traces
    idle_wait_us(TARGET_UBENCH_RT_MS*1000);
    //time = (vitamins_bm_time_us()-time)/1000;
    //for (int j=0; j<r; j++)
    //    printf("%d ",data[j]);
    //printf("   in %ld ms\n",time);

}

static void combinationUtil(int r, int index, int data[], int i)
{
    // Current cobination is ready, print it
    if (index == r)
    {
        run_combination(r,data);
        return;
    }

    // When no more elements are there to put in data[]
    if (i >= (int)benchname.size())
        return;

    // current is included, put next at next location
    data[index] = i;
    combinationUtil(r, index+1, data, i+1);

    // current is excluded, replace it with next (Note that
    // i+1 is passed, but index is not changed)
    combinationUtil(r, index, data, i+1);
}

//data must be an array of at least size r
static inline void runCombinations(int r, int *data)
{
    // Print all combination using temprary array 'data[]'
    combinationUtil(r, 0, data, 0);
}


static void init_iters(int argc, char* argv[])
{
    assert(argc == (int)(benchname.size()+1));
    for(unsigned i = 0; i < benchIters.size();++i){
        int arg_i = i+1;
        assert(arg_i < argc);
        std::istringstream ss(argv[arg_i]);
        if (!(ss >> benchIters[i])) {
            printf("Error parsing arg %d = %s\n",arg_i,argv[arg_i]);
            exit(-1);
        }
    }
}

static void run_ubench(int argc, char* argv[]){
    int *data = new int[benchIters.size()];

    init_iters(argc, argv);

    for(unsigned i = 1; i <= benchIters.size(); ++i)
        runCombinations(i,data);
}

int main(int argc, char* argv[])
{
    init_vectors();
    
    if(argc == 1){
        calibration();
    }
    else if(argc == (int)(benchname.size()+1)){
        printf("ubench mode (it might take a while!)\n");
        run_ubench(argc,argv);
        printf("Done\n");
    }
    else {
        printf("argc = %d, wrong number of arguments\n",argc);
    }

	return 0;
}

