#ifndef __arm_rt_perfcnts_h
#define __arm_rt_perfcnts_h

//perf. counters id definitions


typedef enum {
	PERFCNT_BUSY_CY=0,
	PERFCNT_INSTR_EXE,
	PERFCNT_INSTR_BRANCHES,
	PERFCNT_INSTR_FP,
	PERFCNT_INSTR_MEM,
	PERFCNT_INSTR_MEM_RD,
	PERFCNT_INSTR_MEM_WR,
	PERFCNT_BRANCH_MISPRED,
	PERFCNT_DTLB_ACCESS,
	PERFCNT_ITLB_ACCESS,
	PERFCNT_DTLB_MISSES,
	PERFCNT_ITLB_MISSES,
	PERFCNT_L1DCACHE_ACCESS,
	PERFCNT_L1DCACHE_HITS,
	PERFCNT_L1DCACHE_MISSES,
	PERFCNT_L1ICACHE_ACCESS,
	PERFCNT_L1ICACHE_HITS,
	PERFCNT_L1ICACHE_MISSES,
	PERFCNT_LLCACHE_ACCESS,
	PERFCNT_LLCACHE_HITS,
	PERFCNT_LLCACHE_MISSES,
	PERFCNT_BUS_ACCESS,
	PERFCNT_BUS_CY,
	SIZE_PERFCNT
} perfcnt_t;

static inline const char* perfcnt_str(perfcnt_t cnt){
	switch (cnt) {
	case PERFCNT_BUSY_CY: return "cpuBusyCy";
	case PERFCNT_INSTR_EXE: return "totalInstr";
	case PERFCNT_INSTR_BRANCHES: return "branchInstr";
	case PERFCNT_INSTR_MEM: return "memRdWrInstr";
	case PERFCNT_INSTR_MEM_RD: return "memRdInstr";
	case PERFCNT_INSTR_MEM_WR: return "memWrInstr";
	case PERFCNT_BRANCH_MISPRED: return "brMisspred";
	case PERFCNT_DTLB_MISSES: return "dtlbMiss";
	case PERFCNT_ITLB_MISSES: return "itlbMiss";
	case PERFCNT_L1DCACHE_ACCESS: return "l1dcAccess";
	case PERFCNT_L1DCACHE_MISSES: return "l1dcMiss";
	case PERFCNT_L1ICACHE_ACCESS: return "l1icAccess";
	case PERFCNT_L1ICACHE_MISSES: return "l1icMiss";
	case PERFCNT_LLCACHE_ACCESS: return "llcAccess";
	case PERFCNT_LLCACHE_MISSES: return "llcMiss";
	case PERFCNT_BUS_ACCESS: return "busAccess";
	case PERFCNT_BUS_CY: return "busCy";
	default: return "invalid";
	}
}

#endif

