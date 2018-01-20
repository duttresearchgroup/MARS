#ifndef VITAMINS_TIME_UTIL_H
#define VITAMINS_TIME_UTIL_H

#include <sys/time.h>
#include <stddef.h>


inline long int vitamins_bm_time_us(){
	struct timeval now;
	gettimeofday(&now, NULL);
	long int us_now = now.tv_sec * 1000000 + now.tv_usec;
	return us_now;
}

#endif
