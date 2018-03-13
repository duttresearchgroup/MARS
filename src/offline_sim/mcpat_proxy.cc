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

#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

#include "mcpat_proxy.h"
#include "core/core.h"
#include "offline_sim/exec_sim.h"

#include "external/mcpat/XML_Parse.h"
#include "external/mcpat/processor.h"

//#define MCPAT_STATIC_INIT

void
mcpat_setup_platformPrivL2(int num_cores)
{
#ifdef MCPAT_STATIC_INIT
	McPAT::root_system &root = McPAT::ParseXML::get().sys;
    #include "mcpat_proxy/platformPrivL2.h"
#else
	assert(false&&"Error: compile with MCPAT_STATIC_INIT defined");
#endif
}
void
mcpat_set_platformPrivL2_stats(simulation_t *sim)
{
    //TODO add some stuff
}

static
void
_mcpat_setup_core(core_arch_t core_type, int core_idx)
{
#ifdef MCPAT_STATIC_INIT
	int i = core_idx;
    McPAT::root_system &root = McPAT::ParseXML::get().sys;
    switch (core_type) {
        case COREARCH_GEM5_BIG_BIG:
            #include "mcpat_proxy/COREARCH_GEM5_BIG_BIG.h"
            break;
        case COREARCH_GEM5_BIG_LITTLE:
            #include "mcpat_proxy/COREARCH_GEM5_BIG_LITTLE.h"
            break;
        case COREARCH_GEM5_LITTLE_BIG:
            #include "mcpat_proxy/COREARCH_GEM5_LITTLE_BIG.h"
            break;
        case COREARCH_GEM5_LITTLE_LITTLE:
            #include "mcpat_proxy/COREARCH_GEM5_LITTLE_LITTLE.h"
            break;
        default:
            assert("Unsupported core"&&false);
            break;
    }
#else
	assert(false&&"Error: compile with MCPAT_STATIC_INIT defined");
#endif
}

void
mcpat_setup_cores(simulation_t *sim)
{
    for(auto core : sim->core_list_vector()){
        _mcpat_setup_core(core.info->arch,core.info->position);
    }
}

void
mcpat_set_cores_stats(simulation_t *sim,bool privL2)
{
    McPAT::root_system &root = McPAT::ParseXML::get().sys;

    root.number_of_cores = sim->core_list_size();
    if(privL2) root.number_of_L2s = sim->core_list_size();

    for(int i = 0; i < sim->core_list_size(); ++i){
        auto core_data = sim->get_core_data(i);

        root.core[i].vdd = core_data.total_avg_voltage(sim->core_list()[i].info->arch);
        root.core[i].clock_rate = core_data.total_avg_freqMHz();

       // double total_time_on = core_data.total_time_active() + core_data.total_time_idle();

        //root.core[i].total_instructions = core_data.total_instructions();
        //root.core[i].total_cycles = total_time_on / (1.0/((double)(root.core[i].clock_rate)*1000000));
        //root.core[i].busy_cycles = (core_data.total_time_active()*root.core[i].pipeline_duty_cycle) / (1.0/((double)(root.core[i].clock_rate)*1000000));
        //root.core[i].idle_cycles = (core_data.total_time_active()*(1-root.core[i].pipeline_duty_cycle)) / (1.0/((double)(root.core[i].clock_rate)*1000000));

        //TODO add more stuff and fix

    }
}

McPAT::Processor*
mcpat_run()
{
    const int print_ident = 2;
    const int print_level = 5;

    McPAT::ParseXML &p1= McPAT::ParseXML::get();

    McPAT::Processor *proc = 0;
    try {
        proc = new McPAT::Processor(&p1);
        std::stringstream ss;
        ss << getpid();
        std::string pid = ss.str();
        std::string outfile = "/tmp/mcpat"+pid+".flp";
        std::ofstream out(outfile);
        proc->displayEnergy(out,print_ident, print_level);
        std::cout << "McPAT report -> " << outfile << "\n";
    }
    catch (std::exception& e)
    {
        std::cout << "error/warning: McPAT triggered exception "<< e.what() << "\n";
        proc = 0;
    }

    return proc;
}

template<typename Result, typename Component>
void set_stuff(Result &r, Component *c, double time_active, double time_sleep, double total_time_on, double total_time_off){
    r.area = c->results_area_MM2();

    r.sub_leak_power = c->results_sub_leak_W();
    r.gate_leak_power = c->results_gate_leak_W();

    r.avg_dyn_power = c->results_rt_dyn_W();

    r.gated_sub_leak_power = c->results_gated_sub_leak_W();

    r.avg_leak_power = r.gate_leak_power
                       + (r.sub_leak_power * (time_active/total_time_on))
                       + (r.gated_sub_leak_power * (time_sleep/total_time_on));

    r.peak_dyn_power = c->results_peak_dyn_W();
    r.total_peak_power = c->results_peak_total_W();

    r.total_avg_power = r.avg_dyn_power + r.avg_leak_power;
    r.total_avg_power *= total_time_on / (total_time_on+total_time_off);
    r.total_avg_power *= 1.3;
}

McPATCoreResult mcpat_privL2_core_result(McPAT::Processor *result, simulation_t *sim, int core_pos)
{
    McPATCoreResult r;
    McPAT::Core *core = result->cores[core_pos];
    McPAT::SharedCache *l2 = result->l2array[core_pos];
    auto core_data = sim->get_core_data(core_pos);

    double time_active = core_data.total_time_active();
    double time_sleep = core_data.total_time_idle();
    double total_time_on = core_data.total_time_active() + core_data.total_time_idle();
    double total_time_off = core_data.total_time_off();

    set_stuff(r.core,core,time_active,time_sleep, total_time_on, total_time_off);
    r.priv_l2 = true;
    set_stuff(r.l2,l2,time_active,time_sleep, total_time_on, total_time_off);
    return r;
}

McPAT::Processor* mcpat_privL2_core_for_area_only(simulation_t *sim)
{
    mcpat_setup_platformPrivL2(sim->core_list_size());
    mcpat_setup_cores(sim);
    mcpat_set_platformPrivL2_stats(sim);

    McPAT::root_system &root = McPAT::ParseXML::get().sys;

    root.number_of_cores = sim->core_list_size();
    root.number_of_L2s = sim->core_list_size();

    for(int i = 0; i < sim->core_list_size(); ++i){
        auto core_data = sim->get_core_data(i);

        root.core[i].vdd = vitamins_freq_to_mVolt_map(sim->core_list()[i].info->arch,vitamins_dvfs_get_maximum_freq(&(sim->core_list()[i])))/1000.0;
        root.core[i].clock_rate = freqToValMHz_d(vitamins_dvfs_get_maximum_freq(&(sim->core_list()[i])));

    }

    McPAT::Processor *proc = mcpat_run();

    return proc;
}
