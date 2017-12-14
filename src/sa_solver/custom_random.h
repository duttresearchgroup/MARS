#ifndef __custom_random_h
#define __custom_random_h

namespace SASolverImpl {

enum Pseudo_Random_Type{
	RND_LIBC, //best distribution
	RND_INTEL, //5x faster
	RND_XORSHF,//not working properly
	RND_DEFAULT = RND_INTEL
};

template<Pseudo_Random_Type t>
class Pseudo_Random_Base;

template<>
class Pseudo_Random_Base<RND_INTEL> {
	static unsigned long long int g_seed;
public:
	static void seed(unsigned int seed) {
		g_seed = seed;
	}

	static int random() {
		g_seed = (214013*(g_seed+2531011));
		return (int)((g_seed>>32)&0x7FFFFFFF);

	}
};


template<>
class Pseudo_Random_Base<RND_XORSHF> {
	static unsigned long x,y,z;

	static unsigned long xorshf96() {          //period 2^96-1
		unsigned long t;
		x ^= x << 16;
		x ^= x >> 5;
		x ^= x << 1;

		t = x;
		x = y;
		y = z;
		z = t ^ x ^ y;

		return z;
	}
public:
	static void seed(int seed) {
		x=123456789+(unsigned int)seed;
		y=362436069+(unsigned int)seed;
		z=521288629+(unsigned int)seed;
	}

	static int random() {
		return (int)xorshf96();

	}
};


template<Pseudo_Random_Type t>
class Pseudo_Random : public Pseudo_Random_Base<t>{
public:
	using Pseudo_Random_Base<t>::random;
	using Pseudo_Random_Base<t>::seed;

    static int random(int min, int max) {
    	if(min == max) return min;
    	return min + (Pseudo_Random_Base<t>::random() % (max-min));
    }

    //max .000 precision
    static double random(double min, double max) {
    	if(min == max) return min;
    	int int_min = (int)(min*1000000);
    	int int_max = (int)(max*1000000);

    	return random(int_min, int_max) / 1000000.0;
    }
};

};

#endif
