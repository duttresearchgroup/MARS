#ifndef __hotspot_proxy_h
#define __hotspot_proxy_h

#include "exec_sim.h"
#include "mcpat_proxy.h"

void run_hotspot(McPAT::Processor *mcpat_result, simulation_t *sim, std::vector<double> &core_temperatures);


#endif
