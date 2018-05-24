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

#ifndef __arm_rt_pal_defs_h
#define __arm_rt_pal_defs_h

#ifdef __KERNEL__
    #include "../../linux/kernel_module/core.h"
#else
    #include <base/base.h>
#endif

#ifdef PLAT_DEF
//blackmagic to build an include using PLAT_DEF
#define _pIDENT(x) x
#define _pXSTR(x) #x
#define _pSTR(x) _pXSTR(x)
#define _pPATH(x,y) _pSTR(_pIDENT(x)_pIDENT(y))
#define _pFILE /defs.h
#include _pPATH(PLAT_DEF,_pFILE)

#else
#error "PLAT_DEF macro not defined"
#endif


#endif
