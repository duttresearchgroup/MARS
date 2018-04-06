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

#include "bin_based_predictor_funcs.h"

namespace BinBasedPred {

#define makebinstr(id) const std::string BinFuncImpl<id>::str = #id

makebinstr(procTimeShare);
makebinstr(ipsTotal);
makebinstr(ipsBusy);
makebinstr(ipcBusy);
makebinstr(power);
makebinstr(memRate);
makebinstr(branchRate);
makebinstr(fpRate);
makebinstr(brMisspred);
makebinstr(brMisspredPerInstr);
makebinstr(l1Dmiss);
makebinstr(l1DmissPerInstr);
makebinstr(l1ImissPerInstr);
makebinstr(l1Imiss);
makebinstr(dTLBmiss);
makebinstr(iTLBmiss);
makebinstr(dTLBmissPerInstr);
makebinstr(iTLBmissPerInstr);
makebinstr(globalLLCmiss);
makebinstr(localLLCmiss);
makebinstr(LLCmissPerInstr);
makebinstr(branchAndCacheSum);
makebinstr(branchAndCacheMult);
makebinstr(LLCL1Dsum);
makebinstr(L1DL1Isum);
makebinstr(L1DL1ILLCsum);
makebinstr(L1DL1ILLCsum2);
makebinstr(L1DLLCmissPerInstr);

};
