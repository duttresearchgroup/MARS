#include "custom_random.h"

namespace SASolverImpl {

unsigned long long int Pseudo_Random_Base<RND_INTEL>::g_seed = 555;

unsigned long Pseudo_Random_Base<RND_XORSHF>::x=123456789;
unsigned long Pseudo_Random_Base<RND_XORSHF>::y=362436069;
unsigned long Pseudo_Random_Base<RND_XORSHF>::z=521288629;


};
