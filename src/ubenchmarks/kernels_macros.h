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

#ifndef VITAMINS_UBENCH_KERNELS_MACROS
#define VITAMINS_UBENCH_KERNELS_MACROS

extern unsigned long long int _vitamins_bm_rnd_seed;
inline void vitamins_bm_rnd_seed(unsigned int seed) {
	_vitamins_bm_rnd_seed = seed;
}
inline int vitamins_bm_rnd(void) {
    _vitamins_bm_rnd_seed = (214013*(_vitamins_bm_rnd_seed+2531011));
	return (int)((_vitamins_bm_rnd_seed>>32)&0x7FFFFFFF);
}


inline int vitamins_bm_mem_rnd_idx(int workbufferS){
    return vitamins_bm_rnd() % workbufferS;
}

int _vitamins_bm_low_ilp_int(int mod);
float _vitamins_bm_low_ilp_float(float mod);

#define _vitamins_bm_low_ilp_body(func) \
    if(mod > 5) return vitamins_bm_rnd();\
    if(vitamins_bm_rnd() % 2) {\
        if(vitamins_bm_rnd() % 2) {\
            if(vitamins_bm_rnd() % 2){\
                return func(mod+1);\
            }\
            else{\
                return func(mod+1);\
            }\
        }\
        else {\
            if(vitamins_bm_rnd() % 2){\
                return func(mod+1);   \
            }\
            else{\
                return func(mod+1);\
            }\
        }\
    }\
    else {\
        if(vitamins_bm_rnd() % 2) {\
            if(vitamins_bm_rnd() % 2){\
                return func(mod+1);   \
            }\
            else{\
                return func(mod+1);\
            }        \
        }\
        else {\
            if(vitamins_bm_rnd() % 2){\
                return func(mod+1);    \
            }\
            else{\
                return func(mod+1);\
            }        \
        }\
    }
        
#define _vitamins_bm_high_ilp_end \
        i0 += i0+1;\
        *f0 += *f0+1;\
        i1 += i1+2;\
		*f1 -= *f1+2;\
		i2 += i2+3;\
		*f2 -= *f2+3;\
		i3 += i3+4;\
		*f3 -= *f3+4;\
	    i4 += i4+1;\
		*f4 += *f4+1;\
		i5 += i5+2;\
		*f5 -= *f5+2;\
		i6 += i6+3;\
		*f6 -= *f6+3;\
		i7 += i7+4;\
		*f7 -= *f7+4;\
		*out += (int)(i0+*f0+i1+*f1+i2+*f2+i3+*f3);\
		*out += (int)(i4+*f4+i5+*f5+i6+*f6+i7+*f7);\
		++i0; ++i1; ++i2; ++i3;\
		++*f0; ++*f1; ++*f2; ++*f3;\
		++i4; ++i5; ++i6; ++i7;\
		++*f4; ++*f5; ++*f6; ++*f7;\
	}
	

#define _vitamins_bm_high_ilp_int_begin \
    int i = 0;\
    int time = vitamins_bm_rnd();\
    int i0=time%2, i1=time%2, i2=time%2, i3=time%2;\
    int i4=time%2, i5=time%2, i6=time%2, i7=time%2;

#define _vitamins_bm_high_ilp_float_begin \
    int i = 0;\
    int time = vitamins_bm_rnd();\
    float i0=time%2, i1=time%2, i2=time%2, i3=time%2;\
    float i4=time%2, i5=time%2, i6=time%2, i7=time%2;

#endif
