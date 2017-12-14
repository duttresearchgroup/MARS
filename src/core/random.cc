#include "core.h"

//we don't need a super uniform distribution, so this simple stuff is fine

static unsigned long long int g_seed = 555;

void
vitamins_random_seed(unsigned int seed)
{
    g_seed = seed;
}

int
vitamins_random()
{
    g_seed = (214013*(g_seed+2531011));
    return (int)((g_seed>>32)&0x7FFFFFFF);
}

int
vitamins_random_range(int min, int max) {
    if(min == max) return min;
    return min + (vitamins_random() % (max-min));
}
