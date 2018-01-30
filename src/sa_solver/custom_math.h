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

#ifndef __custom_math_h
#define __custom_math_h

#define PI 3.14159265

#define MATH_SQRT_LOOP_ITERATIONS 6
#define MATH_FAST_EXP_PRECISION 0.000001
#define MATH_FAST_EXP_LOOP_ITERATIONS 10

namespace SASolverImpl {

class Math {
public:

	static double fast_exp_tunnable(double const & x){
		//using Taylor series
		double eps = MATH_FAST_EXP_PRECISION;
		double elem = 1;
		double sum = 0;
		bool negative = false;
		int i = 1;
		sum = 0.0;
		double x1 = x;
		if (x < 0) {
			negative = true;
			x1 = -x;
		}
		do {
			sum += elem;
			elem *= x1 / i;
			i++;
			if (sum > 1E305) break;
			if(i >= MATH_FAST_EXP_LOOP_ITERATIONS) break;
		} while (elem >= eps);
		if (negative) {
			return 1.0 / sum;
		} else {
			return sum;
		}
	}

	template <typename T>
	static T fast_exp_any_tunnable(T const & x){
		//using Taylor series
		T eps = MATH_FAST_EXP_PRECISION;
		T elem(1);
		T sum(0);
		bool negative = false;
		int i = 1;
		sum = 0.0;
		T x1 = x;
		if (x < 0) {
			negative = true;
			x1 = -x;
		}
		do {
			sum += elem;
			elem *= x1 / i;
			i++;
			if (sum > 1E305) break;
			if(i >= MATH_FAST_EXP_LOOP_ITERATIONS) break;
		} while (elem >= eps);
		if (negative) {
			return T(1) / sum;
		} else {
			return sum;
		}
	}

	//this one seems the best, but may be machine dependent
	static	float fast_exp (float x)
	{
		volatile union {
			float f;
			unsigned int i;
		} cvt;

		//exp(x) = 2^i * 2^f; i = floor (log2(e) * x), 0 <= f <= 1
		float t = x * 1.442695041f;
		float fi = round_nearest<int>(t);
		float f = t - fi;
		int i = (int)fi;
		cvt.f = (0.3371894346f * f + 0.657636276f) * f + 1.00172476f; //compute 2^f
		cvt.i += (i << 23);                                          //scale by 2^i
		return cvt.f;
	}

	//improves performance a little bit compared to fast_exp (but reduces precision)
	static double fast_exp2(double x){
		return (40320+x*(40320+x*(20160+x*(6720+x*(1680+x*(336+x*(56+x*(8+x))))))))*2.4801587301e-5;
	}


	int int_abs(int x) { return (x<0) ? (x*(-1)) : x;}
	static double double_abs(double x) { return (x<0) ? (x*(-1.0)) : x;}


	template <typename T>
	static T fast_sin(const T& theta) {
		T x = theta;
		return x - ((x * x * x) / T(6)) + ((x * x * x * x * x) / T(120)) - ((x * x * x * x * x * x * x) / T(5040));
	}


	static double fast_sin2(double x) {
		const double B = 4/PI;
		const double C = -4/(PI*PI);

		double y = B * x + C * x * double_abs(x);

#ifdef EXTRA_PRECISION
		//  const float Q = 0.775;
		const double P = 0.225;

		y = P * (y * double_abs(y) - y) + y;   // Q * y + P * y * abs(y)
#endif
		return y;
	}

	//comment this just to remove warnings
	/*
static float fast_rsqrt( float number ) {
    //Very fast inverse square root implementation from quake III
    //Witchcraft ???
    long i;
    float x2, y;
    const float threehalfs = 1.5F;

    x2 = number * 0.5F;
    y  = number;
    i  = * ( long * ) &y;                       // evil floating point bit level hacking
    i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
    y  = * ( float * ) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
    y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

    return y;
}

static float fast_sqrt(const float & x){
    return 1/fast_rsqrt(x);
}*/

	static double sqrt(double num) {
		double x = num;
		for(int i = 0; i < MATH_SQRT_LOOP_ITERATIONS; i++) {
			x = (((x * x) + num) / (2 * x));
		}
		return x;
	}

	template<typename T>
	static T sqrt_any(T num) {
		T x = num;
		for(unsigned char i = 0; i < MATH_SQRT_LOOP_ITERATIONS; i++) {
			x = (((x * x) + num) / (2 * x));
		}
		return x;
	}

	template<typename INT_TYPE, typename FLOAT_TYPE>
	static INT_TYPE round_nearest(FLOAT_TYPE c){ // You can also call a 'double' instead of a 'float' or an 'int' which is pointless

		/// for example, let's say "c = 2.9" ///

		INT_TYPE a = c; // a = 2.9, because 'a' is an 'int', it will take on the value 2 and nothing after the decimal

		c = c - a; // c = 2.9 - 2, now 'c' will equal .9

		int b = c; // b = .9, also because 'b' is an 'int' it will do the same thing 'a' did, so "b = 0"

		if (c > FLOAT_TYPE(.5)){ // "if (.9 > .5)" This statement checks to see which whole number 'c' is currently closest to via 0 or 1

			b = 1; // if it's over .5 then it's closer to 1
		}
		else{
			b = 0; // else it must be closer to 0
		}
		c = a + b; // "c = 2 + 1" Now 'c' is at the value 3

		return c; // return 3
	}

};

};

#endif
