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

#ifndef __arm_rt_models_binbased_funcs_h
#define __arm_rt_models_binbased_funcs_h

#include <core/core.h>
#include <runtime/common/traceparser.h>
#include <runtime/framework/sensing_interface.h>
#include <runtime/interfaces/common/sense_defs.h>

namespace BinBasedPred {

enum BinFuncID {
    procTimeShare,
    ipsTotal,
    ipsBusy,
    ipcBusy,
    power,
    memRate,
    branchRate,
    fpRate,
    brMisspred,
    brMisspredPerInstr,
    l1Dmiss,
    l1DmissPerInstr,
    l1ImissPerInstr,
    l1Imiss,
    dTLBmiss,
    iTLBmiss,
    dTLBmissPerInstr,
    iTLBmissPerInstr,
    globalLLCmiss,
    localLLCmiss,
    LLCmissPerInstr,
    branchAndCacheSum,
    branchAndCacheMult,
    LLCL1Dsum,
    L1DL1Isum,
    L1DL1ILLCsum,
    L1DL1ILLCsum2,
    L1DLLCmissPerInstr,
    SIZE_BinFuncID
};

template<BinFuncID ID>
struct BinFuncImpl{
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        assert_false("Implementation not defined");
        return 0;
    }
};

//Syntactic sugar for the sensing function. Works only within BinFuncImpl::op
#define bfiSense(ID) Interface::template sense<ID>(rsc,wid)
#define bfiSensePerfCnt(PFC) Interface::template sense<SEN_PERFCNT>(PFC,rsc,wid)
#define bfiOp(ID) BinFuncImpl<ID>::op<Interface>(rsc,wid)

template<> struct BinFuncImpl<procTimeShare> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSense(SEN_BUSYTIME_S) / bfiSense(SEN_TOTALTIME_S);
    }
};
template<> struct BinFuncImpl<ipsTotal> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_INSTR_EXE) / bfiSense(SEN_TOTALTIME_S);
    }
};
template<> struct BinFuncImpl<ipsBusy> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_INSTR_EXE) / bfiSense(SEN_TOTALTIME_S);
    }
};
template<> struct BinFuncImpl<ipcBusy> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_INSTR_EXE) / bfiSensePerfCnt(PERFCNT_BUSY_CY);
    }
};
template<> struct BinFuncImpl<power> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSense(SEN_POWER_W);
    }
};
template<> struct BinFuncImpl<memRate> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_INSTR_MEM) / bfiSensePerfCnt(PERFCNT_INSTR_EXE);
    }
};
template<> struct BinFuncImpl<branchRate> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_INSTR_BRANCHES) / bfiSensePerfCnt(PERFCNT_INSTR_EXE);
    }
};
template<> struct BinFuncImpl<fpRate> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_INSTR_FP) / bfiSensePerfCnt(PERFCNT_INSTR_EXE);
    }
};
template<> struct BinFuncImpl<brMisspred> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_BRANCH_MISPRED) / bfiSensePerfCnt(PERFCNT_INSTR_BRANCHES);
    }
};
template<> struct BinFuncImpl<brMisspredPerInstr> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_BRANCH_MISPRED) / bfiSensePerfCnt(PERFCNT_INSTR_EXE);
    }
};
template<> struct BinFuncImpl<l1Dmiss> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_L1DCACHE_MISSES) / bfiSensePerfCnt(PERFCNT_L1DCACHE_ACCESS);
    }
};
template<> struct BinFuncImpl<l1DmissPerInstr> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_L1DCACHE_MISSES) / bfiSensePerfCnt(PERFCNT_INSTR_EXE);
    }
};
template<> struct BinFuncImpl<l1Imiss> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_L1ICACHE_MISSES) / bfiSensePerfCnt(PERFCNT_L1ICACHE_ACCESS);
    }
};
template<> struct BinFuncImpl<l1ImissPerInstr> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_L1ICACHE_MISSES) / bfiSensePerfCnt(PERFCNT_INSTR_EXE);
    }
};
template<> struct BinFuncImpl<dTLBmiss> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_DTLB_MISSES) / bfiSensePerfCnt(PERFCNT_DTLB_ACCESS);
    }
};
template<> struct BinFuncImpl<iTLBmiss> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_ITLB_MISSES) / bfiSensePerfCnt(PERFCNT_ITLB_ACCESS);
    }
};
template<> struct BinFuncImpl<dTLBmissPerInstr> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_DTLB_MISSES) / bfiSensePerfCnt(PERFCNT_INSTR_EXE);
    }
};
template<> struct BinFuncImpl<iTLBmissPerInstr> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_ITLB_MISSES) / bfiSensePerfCnt(PERFCNT_INSTR_EXE);
    }
};
template<> struct BinFuncImpl<globalLLCmiss> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_LLCACHE_MISSES) / bfiSensePerfCnt(PERFCNT_INSTR_MEM);
    }
};
template<> struct BinFuncImpl<localLLCmiss> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_LLCACHE_MISSES) / bfiSensePerfCnt(PERFCNT_LLCACHE_ACCESS);
    }
};
template<> struct BinFuncImpl<LLCmissPerInstr> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiSensePerfCnt(PERFCNT_LLCACHE_MISSES) / bfiSensePerfCnt(PERFCNT_INSTR_EXE);
    }
};
template<> struct BinFuncImpl<branchAndCacheSum> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiOp(brMisspredPerInstr) + bfiOp(LLCmissPerInstr);
    }
};
template<> struct BinFuncImpl<branchAndCacheMult> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiOp(brMisspredPerInstr) * bfiOp(LLCmissPerInstr);
    }
};
template<> struct BinFuncImpl<LLCL1Dsum> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiOp(LLCmissPerInstr) + bfiOp(l1Dmiss);
    }
};
template<> struct BinFuncImpl<L1DL1Isum> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiOp(l1Dmiss)+bfiOp(l1Imiss);
    }
};
template<> struct BinFuncImpl<L1DL1ILLCsum> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiOp(l1Dmiss)+bfiOp(l1Imiss) + bfiOp(LLCmissPerInstr);
    }
};
template<> struct BinFuncImpl<L1DL1ILLCsum2> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiOp(l1DmissPerInstr)+bfiOp(l1ImissPerInstr) + bfiOp(LLCmissPerInstr);
    }
};
template<> struct BinFuncImpl<L1DLLCmissPerInstr> {
    static const std::string str;
    template<typename Interface,typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return bfiOp(l1DmissPerInstr) + bfiOp(LLCmissPerInstr);
    }
};

template<BinFuncID ID>
struct BinFuncImplWrapper : private BinFuncImpl<ID> {
    using BinFuncImpl<ID>::str;

    template<typename ResourceT>
    static double op(const ResourceT *rsc, int wid){
        return BinFuncImpl<ID>::template op<SensingInterface, ResourceT>(rsc,wid);
    }

    static double op(const TraceParser::CSVData &data){
        TraceSensingInterace::setTrace(data);
        return BinFuncImpl<ID>::template op<TraceSensingInterace, void>(nullptr,0);
    }
};



struct BinFunc {

    typedef double (*bin_func_trace)(const TraceParser::CSVData &data);
    typedef double (*bin_func_task)(const tracked_task_data_t *tsk, int wid);

    BinFuncID id;
    const std::string *str;

    bin_func_trace func_trace;
    bin_func_task func_task;

    double operator()(const tracked_task_data_t *tsk, int wid){
        return (func_task)(tsk,wid);
    }
    double operator()(const TraceParser::CSVData *data){
        return (func_trace)(*data);
    }
    double operator()(const tracked_task_data_t &tsk, int wid){
        return (func_task)(&tsk,wid);
    }
    double operator()(const TraceParser::CSVData &data){
        return (func_trace)(data);
    }

    //trick to unroll the check loop using templates
    template <int First, int Last>
    struct static_for
    {
        void operator()(BinFunc &self) const
        {
            if (First < Last)
            {
                if((BinFuncID)First == self.id){
                    self.str = &(BinFuncImplWrapper<(BinFuncID)First>::str);
                    self.func_trace = &(BinFuncImplWrapper<(BinFuncID)First>::op);
                    self.func_task = &(BinFuncImplWrapper<(BinFuncID)First>::template op<tracked_task_data_t>);
                    return;
                }
                else
                    static_for<First+1, Last>()(self);
            }
        }
    };

    template <int N>
    struct static_for<N, N>
    {
        void operator()(BinFunc &self) const
        { }
    };

    BinFunc(BinFuncID id)
        :id(id), str(nullptr), func_trace(nullptr), func_task(nullptr)
    {
        static_for<0, SIZE_BinFuncID>()(*this);
    }

};


};

#endif
