/*******************************************************************************
 * Copyright (C) 2018 Biswadip Maity <biswadip.maity@gmail.com>
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
#include "../defs.h"
#define PMN_COUNT MAX_PERFCNTS

#define PMCNTEN_CYCLE_COUNTER (1<<31) /*bit for cycle counter in PMCNTEN*/

/** -- Initialization & boilerplate ---------------------------------------- */
#define ARMV8_PMEVTYPER_EVTCOUNT_MASK  0x3ff
#define ARMV8_PMCR_MASK         0x3f
#define ARMV8_PMCR_E            (1 << 0) /* Enable all counters */
#define ARMV8_PMCR_P            (1 << 1) /* Reset all counters */
#define ARMV8_PMCR_C            (1 << 2) /* Cycle counter reset */
#define PMINTENCLR_ALL          0xFFFFFFFF /*bits to clear all interrupts*/
/*****************************************************************************/

// ARM Specific event types
enum armv8_perf_types {
    ARMV8_PERFCTR_PMNC_SW_INCR          = 0x00,
    ARMV8_PERFCTR_L1_ICACHE_REFILL      = 0x01,
    ARMV8_PERFCTR_ITLB_REFILL           = 0x02,
    ARMV8_PERFCTR_L1_DCACHE_REFILL      = 0x03,
    ARMV8_PERFCTR_L1_DCACHE_ACCESS      = 0x04,
    ARMV8_PERFCTR_DTLB_REFILL           = 0x05,
    ARMV8_PERFCTR_MEM_READ              = 0x06,
    ARMV8_PERFCTR_MEM_WRITE             = 0x07,
    ARMV8_PERFCTR_INSTR_EXECUTED        = 0x08,
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
    ARMV8_PERFCTR_PC_PROC_RETURN        = 0x0E,
    ARMV8_PERFCTR_MEM_UNALIGNED_ACCESS  = 0x0F,
    ARMV8_PERFCTR_PC_BRANCH_MIS_PRED    = 0x10,
    ARMV8_PERFCTR_CLOCK_CYCLES          = 0x11,
    ARMV8_PERFCTR_PC_BRANCH_PRED        = 0x12,

    ARMV8_PERFCTR_MEM_ACCESS            = 0x13,
    ARMV8_PERFCTR_L1_ICACHE_ACCESS      = 0x14,
    ARMV8_PERFCTR_L1_DCACHE_WB          = 0x15,
    ARMV8_PERFCTR_L2_CACHE_ACCESS       = 0x16,
    ARMV8_PERFCTR_L2_CACHE_REFILL       = 0x17,
    ARMV8_PERFCTR_L2_CACHE_WB           = 0x18,
    ARMV8_PERFCTR_BUS_ACCESS            = 0x19,
    ARMV8_PERFCTR_MEM_ERROR             = 0x1A,
    ARMV8_PERFCTR_INSTR_SPEC            = 0x1B,
    ARMV8_PERFCTR_TTBR_WRITE            = 0x1C,
    ARMV8_PERFCTR_BUS_CYCLES            = 0x1D,

	//usead a "place holder" actually
    ARMV8_PERFCTR_CPU_CYCLES            = 0xFF
};

// Maps the generic MARS perf events to platform specific event types
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

// Stores the map of the ARM event type to a specific counter (out of the available ones)
// If an event is assigned to a counter, it will contain -1
// Assumes ARMV8_PERFCTR_CPU_CYCLES is the last one
static int cnt_to_pmn_map[ARMV8_PERFCTR_CPU_CYCLES+1];

// Stores which counter is currently tracking an event in a particular CPU
static int pmn_to_cnt_map[MAX_NR_CPUS][PMN_COUNT];

// Number of event types that are being tracked other than busy cycles
static int pmn_enabled_counters = 0;

int plat_max_enabled_perfcnts(void)
{
	//PMN_COUNT + the busy cycles which is always enabled
	return PMN_COUNT+1;
}

int plat_enabled_perfcnts(void){
	//pmn_enabled_counters + the busy cycles which is always enabled
	return pmn_enabled_counters+1;
}

//def at vitmins header
void plat_reset_perfcnts()
{
    int i,cpu=0, count=0;
    for(i = 0; i < ARMV8_PERFCTR_CPU_CYCLES; ++i) cnt_to_pmn_map[i] = -1;
    for_each_online_cpu(cpu){
        for(i = 0; i < PMN_COUNT; ++i) pmn_to_cnt_map[cpu][i] = -1;
        count++;
    }
    
    BUG_ON(count!=MAX_NR_CPUS);
    pmn_enabled_counters = 0;
}

void plat_enable_perfcnt(perfcnt_t perfcnt)
{
    int cpu, count=0;

    if(perfcnt == PERFCNT_BUSY_CY) return;//always enabled

    BUG_ON((pmn_enabled_counters >= (PMN_COUNT-1))&&"No more physical counters available");

    for_each_online_cpu(cpu){
        enum armv8_perf_types plat_cnt = to_arm8_perfcnt(cpu,perfcnt);
        cnt_to_pmn_map[plat_cnt] = pmn_enabled_counters;
        pmn_to_cnt_map[cpu][pmn_enabled_counters] = plat_cnt;
        count++;
    }

    BUG_ON(count!=MAX_NR_CPUS);
    pmn_enabled_counters += 1;
}

static inline uint32_t armv8pmu_pmcr_read(void)
{
	uint64_t val=0;
	asm volatile("mrs %0, pmcr_el0" : "=r" (val));
	return (uint32_t)val;
}
static inline void armv8pmu_pmcr_write(uint32_t val)
{
	val &= ARMV8_PMCR_MASK;
	isb();
	asm volatile("msr pmcr_el0, %0" : : "r" ((uint64_t)val));
}

static inline void enable_event(uint32_t counterId, int evtCount)
{
    uint32_t r = 0;
    BUG_ON(evtCount > 31);

    evtCount &= ARMV8_PMEVTYPER_EVTCOUNT_MASK;
	isb();
	
    switch (counterId)
    {
        case 0: asm volatile("msr pmevtyper0_el0, %0" : : "r" (evtCount));
            break;
        case 1: asm volatile("msr pmevtyper1_el0, %0" : : "r" (evtCount));
            break;
        case 2: asm volatile("msr pmevtyper2_el0, %0" : : "r" (evtCount));
            break;
        case 3: asm volatile("msr pmevtyper3_el0, %0" : : "r" (evtCount));
            break;
        case 4: asm volatile("msr pmevtyper4_el0, %0" : : "r" (evtCount));
            break;
        case 5: asm volatile("msr pmevtyper5_el0, %0" : : "r" (evtCount));
            break;
        default:
            BUG_ON("Invalid counter ID");
    }
	
	/*   Performance Monitors Count Enable Set register bit 30:1 disable, 31,1 enable */
	asm volatile("mrs %0, pmcntenset_el0" : "=r" (r));
	asm volatile("msr pmcntenset_el0, %0" : : "r" (r|(1<<counterId)));

    return;
}

void start_perf_sense(int cpu) {
    int i = 0;

    /* PMINTENCLR: Performance Monitors Interrupt Enable Clear register*/
    /* Cycle counter overflow interrupt request is disabled */
    asm volatile("msr pmintenclr_el1, %0" : : "r" ((uint64_t)(PMINTENCLR_ALL)));

    /*   Performance Monitors Count Enable Set register bit 30:0 disable, 31 enable */
    asm volatile("msr pmcntenset_el0, %0" : : "r" (PMCNTEN_CYCLE_COUNTER));

    for(i = 0; i < pmn_enabled_counters; ++i) 
        enable_event(i, pmn_to_cnt_map[cpu][i]);

    /* PMU start*/
    armv8pmu_pmcr_write(armv8pmu_pmcr_read() | ARMV8_PMCR_E);

    return;
}

void stop_perf_sense(int cpu) {
    
    /*  Performance Monitors Count Enable Set register bit 31:0 disable, 1 enable */
    asm volatile("msr pmcntenset_el0, %0" : : "r" (0));
    
    /*  Program PMU and disable all counters */
    armv8pmu_pmcr_write(armv8pmu_pmcr_read() |~ARMV8_PMCR_E);
    
    return;
}

static inline uint32_t read_perfcnt_cyActive(void)
{
    uint32_t r = 0;
	asm volatile("mrs %0, pmccntr_el0" : "=r" (r)); 
    return r;
}

static inline uint32_t read_perfcnt_others(int counterId)
{
    uint32_t r = 0;
    isb();
	
    switch (counterId)
    {
        case 0: asm volatile("mrs %0, pmevcntr0_el0" : "=r" (r)); 
            break;
        case 1: asm volatile("mrs %0, pmevcntr1_el0" : "=r" (r)); 
            break;
        case 2: asm volatile("mrs %0, pmevcntr2_el0" : "=r" (r)); 
            break;
        case 3: asm volatile("mrs %0, pmevcntr3_el0" : "=r" (r)); 
            break;
        case 4: asm volatile("mrs %0, pmevcntr4_el0" : "=r" (r)); 
            break;
        case 5: asm volatile("mrs %0, pmevcntr5_el0" : "=r" (r)); 
            break;
        default:
            BUG_ON("Invalid counter ID");
    }

	return r;
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