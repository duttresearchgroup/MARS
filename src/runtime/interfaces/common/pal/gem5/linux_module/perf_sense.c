/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * Copyright (C) 2018 Bryan Donyanavard <bdonyana@uci.edu>
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


#include "../../../../linux/kernel_module/pal.h"
//@donny TODO: need to hook this into gem5 stats to get counters
#define PMN_COUNT 31

#define PMCNTEN_CYCLE_COUNTER (1<<31) /*bit for cycle counter in PMCNTEN*/

#define PMINTENCLR_ALL 0xFFFFFFFF /*bits to clear all interrupts*/
#define PMCR_E (1) /*bit to enable all counters*/

enum armv8_perf_types {
    ARMV8_PERFCTR_PMNC_SW_INCR          = 0x00,
    ARMV8_PERFCTR_L1_ICACHE_REFILL          = 0x01,
    ARMV8_PERFCTR_ITLB_REFILL           = 0x02,
    ARMV8_PERFCTR_L1_DCACHE_REFILL          = 0x03,
    ARMV8_PERFCTR_L1_DCACHE_ACCESS          = 0x04,
    ARMV8_PERFCTR_DTLB_REFILL           = 0x05,
    ARMV8_PERFCTR_MEM_READ              = 0x06,
    ARMV8_PERFCTR_MEM_WRITE             = 0x07,
    ARMV8_PERFCTR_INSTR_EXECUTED            = 0x08,
    ARMV8_PERFCTR_EXC_TAKEN             = 0x09,
    ARMV8_PERFCTR_EXC_EXECUTED          = 0x0A, //EXC_RETURN
    ARMV8_PERFCTR_CID_WRITE             = 0x0B,

    /*
     * ARMV8_PERFCTR_PC_WRITE is equivalent to HW_BRANCH_INSTRUCTIONS.
     * It counts:
     *  - all (taken) branch instructions,
     *  - instructions that explicitly write the PC,
     *  - exception generating instructions.
     */
    ARMV8_PERFCTR_PC_WRITE              = 0x0C,
    ARMV8_PERFCTR_PC_IMM_BRANCH         = 0x0D,
    ARMV8_PERFCTR_PC_PROC_RETURN            = 0x0E,
    ARMV8_PERFCTR_MEM_UNALIGNED_ACCESS      = 0x0F,
    ARMV8_PERFCTR_PC_BRANCH_MIS_PRED        = 0x10,
    ARMV8_PERFCTR_CLOCK_CYCLES          = 0x11,
    ARMV8_PERFCTR_PC_BRANCH_PRED            = 0x12,

    ARMV8_PERFCTR_MEM_ACCESS            = 0x13,
    ARMV8_PERFCTR_L1_ICACHE_ACCESS          = 0x14,
    ARMV8_PERFCTR_L1_DCACHE_WB          = 0x15,
    ARMV8_PERFCTR_L2_CACHE_ACCESS           = 0x16,
    ARMV8_PERFCTR_L2_CACHE_REFILL           = 0x17,
    ARMV8_PERFCTR_L2_CACHE_WB           = 0x18,
    ARMV8_PERFCTR_BUS_ACCESS            = 0x19,
    ARMV8_PERFCTR_MEM_ERROR             = 0x1A,
    ARMV8_PERFCTR_INSTR_SPEC            = 0x1B,
    ARMV8_PERFCTR_TTBR_WRITE            = 0x1C,
    ARMV8_PERFCTR_BUS_CYCLES            = 0x1D,

	//usead a "place holder" actually
    ARMV8_PERFCTR_CPU_CYCLES            = 0xFF
};

static inline enum armv8_perf_types to_arm8_perfcnt(int cpu,perfcnt_t perf_cnt)
{
	switch (perf_cnt) {
		case PERFCNT_BUSY_CY: return ARMV8_PERFCTR_CPU_CYCLES;
		case PERFCNT_INSTR_EXE: return ARMV8_PERFCTR_INSTR_EXECUTED;
		case PERFCNT_INSTR_BRANCHES: return ARMV8_PERFCTR_PC_WRITE;
		case PERFCNT_INSTR_MEM: return ARMV8_PERFCTR_MEM_ACCESS;
		case PERFCNT_INSTR_MEM_RD: return ARMV8_PERFCTR_MEM_READ;
		case PERFCNT_INSTR_MEM_WR: return ARMV8_PERFCTR_MEM_WRITE;
		case PERFCNT_BRANCH_MISPRED: return ARMV8_PERFCTR_PC_BRANCH_MIS_PRED;
		case PERFCNT_DTLB_MISSES: return ARMV8_PERFCTR_DTLB_REFILL;
		case PERFCNT_ITLB_MISSES: return ARMV8_PERFCTR_ITLB_REFILL;
		case PERFCNT_L1DCACHE_ACCESS: return ARMV8_PERFCTR_L1_DCACHE_ACCESS;
		case PERFCNT_L1DCACHE_MISSES: return ARMV8_PERFCTR_L1_DCACHE_REFILL;
		case PERFCNT_L1ICACHE_ACCESS: return ARMV8_PERFCTR_L1_ICACHE_ACCESS;
		case PERFCNT_L1ICACHE_MISSES: return ARMV8_PERFCTR_L1_ICACHE_REFILL;
		case PERFCNT_LLCACHE_ACCESS: return ARMV8_PERFCTR_L2_CACHE_ACCESS;
		case PERFCNT_LLCACHE_MISSES: return ARMV8_PERFCTR_L2_CACHE_REFILL;
		case PERFCNT_BUS_ACCESS: return ARMV8_PERFCTR_BUS_ACCESS;
		case PERFCNT_BUS_CY: return ARMV8_PERFCTR_BUS_CYCLES;
		default:
			BUG_ON("Counter is not available");
			break;
	}
	BUG_ON("Counter is not available");
	return 0;
}

//current map of counter to physical register
//assumes ARMV8_PERFCTR_CPU_CYCLES is the last one
static int cnt_to_pmn_map[ARMV8_PERFCTR_CPU_CYCLES+1];
static int pmn_to_cnt_map[8][PMN_COUNT];//A7 and A17 will used different counters so we have one of this per cpu
static int pmn_enabled_counters = 0;


static inline void enable_event(int idx, int type)
{
    BUG_ON(idx > 31);
    asm volatile("mcr p15, 0, %0, c9, c12, 5"::"r"(idx));
    asm volatile("isb");
    asm volatile("mcr p15, 0, %0, c9, c13, 1"::"r"(type));
}

void start_perf_sense(int cpu) {
    int i;
	/* PMCNTENSET: Performance Monitors Count Enable Set register */
    uint32_t pmcntenset;
    /* PMINTENCLR: Performance Monitors Interrupt Enable Clear register*/
    uint32_t pminten;

    /* disable interrupt generation on any counter*/
    pminten = PMINTENCLR_ALL;
    asm volatile("mcr p15, 0, %0, c9, c14, 2"::"r"(pminten));

    /*enable enabled counters and cycle counter */
    pmcntenset = PMCNTEN_CYCLE_COUNTER;
    for(i = 0; i < pmn_enabled_counters; ++i) pmcntenset |= (1<<i);
    asm volatile("mcr p15, 0, %0, c9, c12, 1"::"r"(pmcntenset));
    asm volatile("isb");

    for(i = 0; i < pmn_enabled_counters; ++i) enable_event(i, pmn_to_cnt_map[cpu][i]);

    /* enable the Performance monitor unit */
    asm volatile("mcr p15, 0, %0, c9, c12, 0"::"r"(PMCR_E));}

void stop_perf_sense(int cpu) {
    /* Disable the Performance monitor unit */
    asm volatile("mcr p15, 0, %0, c9, c12, 0"::"r"(0));
    asm volatile("isb");
}

void plat_enable_perfcnt(perfcnt_t perfcnt)
{
    int cpu;

    if(perfcnt == PERFCNT_BUSY_CY) return;//always enabled

    BUG_ON((pmn_enabled_counters >= PMN_COUNT)&&"No more physical counters available");

    for_each_online_cpu(cpu){
        enum armv8_perf_types plat_cnt = to_arm8_perfcnt(cpu,perfcnt);
        cnt_to_pmn_map[plat_cnt] = pmn_enabled_counters;
        pmn_to_cnt_map[cpu][pmn_enabled_counters] = plat_cnt;
    }
    BUG_ON(cpu!=8);
    pmn_enabled_counters += 1;
}

void plat_reset_perfcnts()
{
    int i,cpu;
    for(i = 0; i < ARMV8_PERFCTR_CPU_CYCLES; ++i) cnt_to_pmn_map[i] = -1;
    for_each_online_cpu(cpu){
        for(i = 0; i < PMN_COUNT; ++i) pmn_to_cnt_map[cpu][i] = -1;
    }
    BUG_ON(cpu!=8);
    pmn_enabled_counters = 0;
}

int plat_enabled_perfcnts(void){
    // pmn_enabled_counters + the busy cycles which is always enabled
    return pmn_enabled_counters+1;
}

int plat_max_enabled_perfcnts(void)
{
    // PMN_COUNT + the busy cycles which is always enabled
    return PMN_COUNT+1;
}

static inline uint32_t read_perfcnt_cyActive(void)
{
    uint32_t pmccntr;
    /*read cycle counter*/
    asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(pmccntr) );
    return pmccntr;
}

static inline uint32_t read_perfcnt_others(int idx)
{
    uint32_t ret;
    asm volatile("mcr p15, 0, %0, c9, c12, 5"::"r"(idx));
    asm volatile("isb");
    asm volatile("mrc p15, 0, %0, c9, c13, 2":"=r"(ret));

    return ret;
}

uint32_t read_perfcnt(int cpu,perfcnt_t perfcnt)
{
    //always enabled, so use its specific func
    if(perfcnt == PERFCNT_BUSY_CY) return read_perfcnt_cyActive();
    else{
        int plat_cnt = to_arm8_perfcnt(cpu,perfcnt);
        BUG_ON((cnt_to_pmn_map[plat_cnt]<0)&&"Invalid counter");
        return read_perfcnt_others(cnt_to_pmn_map[plat_cnt]);
    }
}


