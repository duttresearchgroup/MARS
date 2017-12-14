#include "../../../common/pal/exynos5422/_common.h"
#include "../../../linux-module/pal/pal.h"

/*Performance monitor counter selection*/
#define PMN_COUNT 4

/*Performance counters individual enable*/
#define PMCNTEN_1 1/*bit for first counter in PMCNTEN*/
#define PMCNTEN_2 2/*bit for second counter in PMCNTEN*/
#define PMCNTEN_3 4/*bit for third counter in PMCNTEN*/
#define PMCNTEN_4 8/*bit for fourth counter in PMCNTEN*/
#define PMCNTEN_CYCLE_COUNTER (1<<31) /*bit for cycle counter in PMCNTEN*/

#define PMINTENCLR_ALL 0xFFFFFFFF /*bits to clear all interrupts*/
#define PMCR_E (1) /*bit to enable all counters*/

enum armv7_perf_types {
    ARMV7_PERFCTR_PMNC_SW_INCR          = 0x00,
    ARMV7_PERFCTR_L1_ICACHE_REFILL          = 0x01,
    ARMV7_PERFCTR_ITLB_REFILL           = 0x02,
    ARMV7_PERFCTR_L1_DCACHE_REFILL          = 0x03,
    ARMV7_PERFCTR_L1_DCACHE_ACCESS          = 0x04,
    ARMV7_PERFCTR_DTLB_REFILL           = 0x05,
    ARMV7_PERFCTR_MEM_READ              = 0x06,
    ARMV7_PERFCTR_MEM_WRITE             = 0x07,
    ARMV7_PERFCTR_INSTR_EXECUTED            = 0x08,
    ARMV7_PERFCTR_EXC_TAKEN             = 0x09,
    ARMV7_PERFCTR_EXC_EXECUTED          = 0x0A,
    ARMV7_PERFCTR_CID_WRITE             = 0x0B,

    /*
     * ARMV7_PERFCTR_PC_WRITE is equivalent to HW_BRANCH_INSTRUCTIONS.
     * It counts:
     *  - all (taken) branch instructions,
     *  - instructions that explicitly write the PC,
     *  - exception generating instructions.
     */
    ARMV7_PERFCTR_PC_WRITE              = 0x0C,
    ARMV7_PERFCTR_PC_IMM_BRANCH         = 0x0D,
    ARMV7_PERFCTR_PC_PROC_RETURN            = 0x0E,
    ARMV7_PERFCTR_MEM_UNALIGNED_ACCESS      = 0x0F,
    ARMV7_PERFCTR_PC_BRANCH_MIS_PRED        = 0x10,
    ARMV7_PERFCTR_CLOCK_CYCLES          = 0x11,
    ARMV7_PERFCTR_PC_BRANCH_PRED            = 0x12,

    /* These events are defined by the PMUv2 supplement (ARM DDI 0457A). */
    ARMV7_PERFCTR_MEM_ACCESS            = 0x13,
    ARMV7_PERFCTR_L1_ICACHE_ACCESS          = 0x14,
    ARMV7_PERFCTR_L1_DCACHE_WB          = 0x15,
    ARMV7_PERFCTR_L2_CACHE_ACCESS           = 0x16,
    ARMV7_PERFCTR_L2_CACHE_REFILL           = 0x17,
    ARMV7_PERFCTR_L2_CACHE_WB           = 0x18,
    ARMV7_PERFCTR_BUS_ACCESS            = 0x19,
    ARMV7_PERFCTR_MEM_ERROR             = 0x1A,
    ARMV7_PERFCTR_INSTR_SPEC            = 0x1B,
    ARMV7_PERFCTR_TTBR_WRITE            = 0x1C,
    ARMV7_PERFCTR_BUS_CYCLES            = 0x1D,

    /* ARMv7 Cortex-A15 specific event types */
    ARMV7_A15_PERFCTR_L1_DCACHE_ACCESS_READ   =      0x40,
    ARMV7_A15_PERFCTR_L1_DCACHE_ACCESS_WRITE   =     0x41,
    ARMV7_A15_PERFCTR_L1_DCACHE_REFILL_READ     =    0x42,
    ARMV7_A15_PERFCTR_L1_DCACHE_REFILL_WRITE    =    0x43,
    ARMV7_A15_PERFCTR_DTLB_REFILL_L1_READ       =    0x4C,
    ARMV7_A15_PERFCTR_DTLB_REFILL_L1_WRITE      =    0x4D,
    ARMV7_A15_PERFCTR_L2_CACHE_ACCESS_READ      =    0x50,
    ARMV7_A15_PERFCTR_L2_CACHE_ACCESS_WRITE     =    0x51,
    ARMV7_A15_PERFCTR_L2_CACHE_REFILL_READ      =    0x52,
    ARMV7_A15_PERFCTR_L2_CACHE_REFILL_WRITE     =    0x53,
    ARMV7_A15_PERFCTR_PC_WRITE_SPEC             =    0x76,

	//usead a "place holder" actually
    ARMV7_PERFCTR_CPU_CYCLES            = 0xFF
};

static inline enum armv7_perf_types to_arm7_perfcnt(int cpu,perfcnt_t perf_cnt)
{
	switch (perf_cnt) {
		case PERFCNT_BUSY_CY: return ARMV7_PERFCTR_CPU_CYCLES;
		case PERFCNT_INSTR_EXE: return ARMV7_PERFCTR_INSTR_EXECUTED;
		case PERFCNT_INSTR_BRANCHES: return core_is_big(cpu) ? ARMV7_A15_PERFCTR_PC_WRITE_SPEC : ARMV7_PERFCTR_PC_WRITE;
		case PERFCNT_INSTR_MEM: return ARMV7_PERFCTR_MEM_ACCESS;
		case PERFCNT_INSTR_MEM_RD: return core_is_big(cpu) ? ARMV7_A15_PERFCTR_L1_DCACHE_ACCESS_READ :  ARMV7_PERFCTR_MEM_READ;
		case PERFCNT_INSTR_MEM_WR: return core_is_big(cpu) ? ARMV7_A15_PERFCTR_L1_DCACHE_ACCESS_WRITE : ARMV7_PERFCTR_MEM_WRITE;
		case PERFCNT_BRANCH_MISPRED: return ARMV7_PERFCTR_PC_BRANCH_MIS_PRED;
		case PERFCNT_DTLB_MISSES: return ARMV7_PERFCTR_DTLB_REFILL;
		case PERFCNT_ITLB_MISSES: return ARMV7_PERFCTR_ITLB_REFILL;
		case PERFCNT_L1DCACHE_ACCESS: return ARMV7_PERFCTR_L1_DCACHE_ACCESS;
		case PERFCNT_L1DCACHE_MISSES: return ARMV7_PERFCTR_L1_DCACHE_REFILL;
		case PERFCNT_L1ICACHE_ACCESS: return ARMV7_PERFCTR_L1_ICACHE_ACCESS;
		case PERFCNT_L1ICACHE_MISSES: return ARMV7_PERFCTR_L1_ICACHE_REFILL;
		case PERFCNT_LLCACHE_ACCESS: return ARMV7_PERFCTR_L2_CACHE_ACCESS;
		case PERFCNT_LLCACHE_MISSES: return ARMV7_PERFCTR_L2_CACHE_REFILL;
		case PERFCNT_BUS_ACCESS: return ARMV7_PERFCTR_BUS_ACCESS;
		case PERFCNT_BUS_CY: return ARMV7_PERFCTR_BUS_CYCLES;
		default:
			BUG_ON("Counter is not available");
			break;
	}
	BUG_ON("Counter is not available");
	return 0;
}

//current map of counter to physical register
//assumes ARMV7_PERFCTR_CPU_CYCLES is the last one
static int cnt_to_pmn_map[ARMV7_PERFCTR_CPU_CYCLES+1];
static int pmn_to_cnt_map[8][PMN_COUNT];//A7 and A17 will used different counters so we have one of this per cpu
const static int pmn_to_enable_bits[PMN_COUNT] = {PMCNTEN_1, PMCNTEN_2, PMCNTEN_3, PMCNTEN_4};
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
	int i,cpu;
	for(i = 0; i < ARMV7_PERFCTR_CPU_CYCLES; ++i) cnt_to_pmn_map[i] = -1;
	for_each_online_cpu(cpu){
		for(i = 0; i < PMN_COUNT; ++i) pmn_to_cnt_map[cpu][i] = -1;
	}
	BUG_ON(cpu!=8);
	pmn_enabled_counters = 0;
}

void plat_enable_perfcnt(perfcnt_t perfcnt)
{
	int cpu;

	if(perfcnt == PERFCNT_BUSY_CY) return;//always enabled

	BUG_ON((pmn_enabled_counters >= PMN_COUNT)&&"No more physical counters available");

	for_each_online_cpu(cpu){
		enum armv7_perf_types plat_cnt = to_arm7_perfcnt(cpu,perfcnt);
		cnt_to_pmn_map[plat_cnt] = pmn_enabled_counters;
		pmn_to_cnt_map[cpu][pmn_enabled_counters] = plat_cnt;
	}
	BUG_ON(cpu!=8);
	pmn_enabled_counters += 1;
}


static inline void enable_event(int idx, int type)
{
    BUG_ON(idx > 4);
    asm volatile("mcr p15, 0, %0, c9, c12, 5"::"r"(idx));
    asm volatile("isb");
    asm volatile("mcr p15, 0, %0, c9, c13, 1"::"r"(type));
}

static inline void enable_pmu(int cpu,void *info)
{
    int i;
	/* PMCNTENSET: Performance Monitors Count Enable Set register */
    uint32_t pmcntenset;
    /* PMINTENCLR: Performance Monitors Interrupt Enable Clear register*/
    uint32_t pminten;
    /* PMCCNTR: cycle counter register*/
    //uint32_t pmccntr;

    /* disable interrupt generation on any counter*/
    pminten = PMINTENCLR_ALL;
    asm volatile("mcr p15, 0, %0, c9, c14, 2"::"r"(pminten));

    /*enable enabled counters and cycle counter */
    pmcntenset = PMCNTEN_CYCLE_COUNTER;
    for(i = 0; i < pmn_enabled_counters; ++i) pmcntenset |= pmn_to_enable_bits[i];
    asm volatile("mcr p15, 0, %0, c9, c12, 1"::"r"(pmcntenset));
    asm volatile("isb");

    for(i = 0; i < pmn_enabled_counters; ++i) enable_event(i, pmn_to_cnt_map[cpu][i]);

    /* enable the Performance monitor unit */
    asm volatile("mcr p15, 0, %0, c9, c12, 0"::"r"(PMCR_E));


    /* Enable user-mode access to counters. */
//  asm volatile("mcr p15, 0, %0, c9, c14, 0" :: "r"(1));
}

static inline void disable_pmu(void *info)
{
    /* Disable the Performance monitor unit */
    asm volatile("mcr p15, 0, %0, c9, c12, 0"::"r"(0));
    asm volatile("isb");
}


void start_perf_sense(int cpu) {
    enable_pmu(cpu,NULL);
}

void stop_perf_sense(int cpu) {
    disable_pmu(NULL);
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
		int plat_cnt = to_arm7_perfcnt(cpu,perfcnt);
		BUG_ON((cnt_to_pmn_map[plat_cnt]<0)&&"Invalid counter");
		return read_perfcnt_others(cnt_to_pmn_map[plat_cnt]);
	}
}


