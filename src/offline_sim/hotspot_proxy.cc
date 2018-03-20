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

#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>
#include "core_legacy/core.h"
#include "offline_sim/exec_sim.h"
#include "offline_sim/mcpat_proxy.h"

struct core_area {
    std::string name;
    double area_m2;
    double power_w;
    double max_ratio=2;
    double min_ratio=0.5;
    int rotable=1;
    std::vector<core_area*> connections;
    core_area(std::string _name, double _area_m2,double _power_w): name(_name), area_m2(_area_m2), power_w(_power_w){}
};



static void print_power(std::vector<core_area*> &floorplan, std::ostream &out){

	for(auto core : floorplan){
		out << core->name << "\t";
	}

	out << "\n";

	for(auto core : floorplan){
		out << core->power_w << "\t";
	}

	out << "\n";
}

static void print_power_flp(std::vector<core_area*> &floorplan, std::ostream &out){
	for(auto core : floorplan){
		out << core->name << "\t";
		out << core->power_w << "\n";
	}
}

static void print_floorplan(std::vector<core_area*> &floorplan, std::ostream &out){

	out << "# comment lines begin with a '#'\n";
	out << "# comments and empty lines are ignored\n";
	out << "\n";
	out << "# Area and aspect ratios of blocks\n";
	out << "# Line Format: <unit-name>\t<area-in-m2>\t<min-aspect-ratio>\t<max-aspect-ratio>\t<rotable>\n";
	out << "\n";
	for(auto core : floorplan){
		out << core->name << "\t" << core->area_m2 << "\t" <<  core->min_ratio << "\t" << core->max_ratio << "\t" << core->rotable << "\n";
	}
	out << "\n";
	out << "# Connectivity information\n";
	out << "# Line format <unit1-name>\t<unit2-name>\t<wire_density>\n";
	out << "\n";

	for(auto core : floorplan)
		for(auto con : core->connections)
			out << core->name << "\t" << con->name << "\t" <<  1 << "\n";
}

static std::string make_name(std::string name, int idx){
	std::stringstream ss;
	ss << name << "_" << idx;
	return ss.str();
}

static McPAT::Processor *last_area_result = (McPAT::Processor*)0xDEADBEEF;
static std::string last_flp = "DEADBEEF";

static
std::string
hotspot_floorplaner(
        McPAT::Processor *mcpat_result, simulation_t *sim,
        std::string &floorplan_conf_file, std::string &powertrace_file,std::string &floorplan_power_file,std::string &floorplan_file,std::string &config,int sharedL2fromPriv=2)
{
    std::vector<core_area*> floorplan;
    core_area *bus = new core_area("noc",1.0/1000,0.05); bus->max_ratio = 20, bus->min_ratio = 0.05;
    floorplan.push_back(bus);

    std::vector<core_area*> floorplan_buff;

    int coreL2Ratio = sharedL2fromPriv ? mcpat_result->numCore/sharedL2fromPriv : 0;
    double l2_area = 0;
    double l2_area_pow = 0;
    int l2_cnt = 0;
    for(int i = 0; i < mcpat_result->numCore; ++i){
        McPATCoreResult rst = mcpat_privL2_core_result(mcpat_result,sim,i);
        assert(rst.priv_l2);
        //std::cout << "Core " << i << " stats\n";
        //std::cout << "\tcore_MM2=" << rst.core.area << "\n";
        //std::cout << "\tl2_MM2=" << rst.l2.area << "\n";
        floorplan_buff.push_back(new core_area(make_name("core",i),std::sqrt(rst.core.area)/1000, sim->get_core_data(i).total_power()));
        l2_area += rst.l2.area;
        l2_area_pow += sim->get_core_data(i).total_power();
        if(coreL2Ratio && (((i+1) % coreL2Ratio) == 0)){
        	//std::cout << "L2 " << l2_cnt << " stats\n";
        	//std::cout << "\tl2_MM2=" << l2_area << "\n";
        	core_area *l2 = new core_area(make_name("l2",l2_cnt++),std::sqrt(l2_area)/1000, l2_area_pow/coreL2Ratio);
        	l2->connections.push_back(bus);
        	l2->max_ratio = 4;
        	floorplan.push_back(l2);
        	for(auto c : floorplan_buff){
        		c->connections.push_back(l2);
        		floorplan.push_back(c);
        	}
        	floorplan_buff.clear();

        	l2_area = 0;
        	l2_area_pow = 0;
        }
        else if(!coreL2Ratio){
        	for(auto c : floorplan_buff){
        		c->connections.push_back(bus);
        		floorplan.push_back(c);
        	}
        	floorplan_buff.clear();
        }
    }
    assert(floorplan_buff.size() == 0);
    for(auto c : floorplan) assert((c->connections.size()>0) || (bus == c));

    {
    	std::ofstream powertrace_out(powertrace_file);
    	print_power(floorplan,powertrace_out);
    }

    if(mcpat_result != last_area_result){
    	{
    		std::ofstream powertrace_flp_out(floorplan_power_file);
    		print_power_flp(floorplan,powertrace_flp_out);

    		std::ofstream floorplan_out(floorplan_conf_file);
    		print_floorplan(floorplan,floorplan_out);
    	}

    	//std::cout << "\n";
    	//print_floorplan(floorplan,std::cout);

    	//std::cout << "\n";
    	//print_power(floorplan,std::cout);

    	//std::cout << "\n";
    	//print_power_flp(floorplan,std::cout);

    	assert(floorplan_buff.size() == 0);
    	for(auto c : floorplan) delete c;

    	std::stringstream ss;
    	ss << "src/hotspot/hotfloorplan -f " << floorplan_conf_file << " -p " << floorplan_power_file << " -c " << config << " -o " << floorplan_file;
    	std::cout << ss.str() << "\n";
    	assert(system(ss.str().c_str()) == 0);

    	last_area_result = mcpat_result;
    	last_flp = floorplan_file;

    	return floorplan_file;
    }
    else{
    	return last_flp;
    }
}

static void hotspot_cmd(std::string &floorplan, std::string &powertrace, std::string &config,std::string &output)
{
	std::stringstream ss;
	ss << "src/hotspot/hotspot -f " << floorplan << " -p " << powertrace << " -c " << config << " > " << output;
	std::cout << ss.str() << "\n";
	assert(system(ss.str().c_str()) == 0);
}

void run_hotspot(McPAT::Processor *mcpat_result, simulation_t *sim,std::vector<double> &core_temperatures)
{
    assert(mcpat_result->numCore == sim->core_list_size());
    assert(mcpat_result->numCore == (int)core_temperatures.size());

    std::stringstream ss;
    ss << getpid();
    std::string pid = ss.str();

    std::string floorplan_desc_file = "/tmp/hotspot"+pid+".desc";
    std::string floorplan_power_file = "/tmp/hotspot"+pid+".p";
    std::string floorplan_file = "/tmp/hotspot"+pid+".flp";
    std::string powertrace_file = "/tmp/hotspot"+pid+".ptrace";
    std::string config_file = "src/hotspot/bigLittle.config";
    std::string output_file = "/tmp/hotspot"+pid+".out";

    floorplan_file = hotspot_floorplaner(mcpat_result,sim,floorplan_desc_file,powertrace_file,floorplan_power_file,floorplan_file,config_file);
    hotspot_cmd(floorplan_file,powertrace_file,config_file,output_file);

    //parse output
    std::string getcontent;
    std::ifstream openfile(output_file);
    int curr_core = 0;
    assert(openfile.is_open());

    while(! openfile.eof()) {
        std::getline(openfile, getcontent);

        std::stringstream curr_core_s;
        curr_core_s << "core_" << curr_core;

        if(getcontent.find(curr_core_s.str()) == 0){
            //std::cout << getcontent << std::endl;
            std::istringstream(getcontent.substr(curr_core_s.str().size())) >> core_temperatures[curr_core];
            ++curr_core;
        }
    }




}
