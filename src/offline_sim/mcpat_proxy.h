#ifndef __mcpat_proxy_h
#define __mcpat_proxy_h

#include "core/core.h"
#include "offline_sim/exec_sim.h"
#include "external/mcpat/processor.h"

void mcpat_setup_platformPrivL2(int num_cores);
void mcpat_set_platformPrivL2_stats(simulation_t *sim);

void mcpat_setup_cores(simulation_t *sim);
void mcpat_set_cores_stats(simulation_t *sim,bool privL2);

McPAT::Processor* mcpat_run();

struct _McPATCoreResult {
    double area;
    double sub_leak_power;
    double gate_leak_power;
    double avg_dyn_power;
    double gated_sub_leak_power;
    double avg_leak_power;
    double total_avg_power;
    double peak_dyn_power;
    double total_peak_power;
};
struct McPATCoreResult {
	_McPATCoreResult core;
	bool priv_l2;
	_McPATCoreResult l2;
};

McPATCoreResult mcpat_privL2_core_result(McPAT::Processor *result, simulation_t *sim, int core);

McPAT::Processor* mcpat_privL2_core_for_area_only(simulation_t *sim);


#endif
