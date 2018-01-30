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
