/*****************************************************************************
 *                                McPAT
 *                      SOFTWARE LICENSE AGREEMENT
 *            Copyright 2012 Hewlett-Packard Development Company, L.P.
 *                          All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.”
 *
 ***************************************************************************/

#ifndef MEMORYCTRL_H_
#define MEMORYCTRL_H_

#include "XML_Parse.h"
#include "cacti/parameter.h"
//#include "io.h"
#include "array.h"
//#include "Undifferentiated_Core_Area.h"
#include <vector>
#include "basic_components.h"

namespace McPAT {

class MCBackend : public CACTI::Component {
  public:
    CACTI::InputParameter l_ip;
    CACTI::uca_org_t local_result;
	enum MemoryCtrl_type mc_type;
    MCParam  mcp;
    statsDef tdp_stats;
    statsDef rtp_stats;
    statsDef stats_t;
    CACTI::powerDef power_t;
    MCBackend(CACTI::InputParameter* interface_ip_, const MCParam & mcp_, enum MemoryCtrl_type mc_type_);
    void compute();
	void computeEnergy(bool is_tdp=true);
    void displayEnergy(uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
    ~MCBackend(){};
};

class MCPHY : public CACTI::Component {
  public:
    CACTI::InputParameter l_ip;
    CACTI::uca_org_t local_result;
	enum MemoryCtrl_type mc_type;
    MCParam  mcp;
    statsDef       tdp_stats;
    statsDef       rtp_stats;
    statsDef       stats_t;
    CACTI::powerDef       power_t;
    MCPHY(CACTI::InputParameter* interface_ip_, const MCParam & mcp_, enum MemoryCtrl_type mc_type_);
    void compute();
	void computeEnergy(bool is_tdp=true);
    void displayEnergy(uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
    ~MCPHY(){};
};

class MCFrontEnd : public CACTI::Component {
  public:
	ParseXML *XML;
	CACTI::InputParameter interface_ip;
	enum MemoryCtrl_type mc_type;
	MCParam  mcp;
	selection_logic * MC_arb;
	ArrayST  * frontendBuffer;
	ArrayST  * readBuffer;
	ArrayST  * writeBuffer;

    MCFrontEnd(ParseXML *XML_interface,CACTI::InputParameter* interface_ip_, const MCParam & mcp_, enum MemoryCtrl_type mc_type_);
    void computeEnergy(bool is_tdp=true);
    void displayEnergy(uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
    ~MCFrontEnd();
};

class MemoryController : public CACTI::Component {
  public:
	ParseXML *XML;
	CACTI::InputParameter interface_ip;
	enum MemoryCtrl_type mc_type;
    MCParam  mcp;
	MCFrontEnd * frontend;
    MCBackend * transecEngine;
    MCPHY	 * PHY;
    Pipeline * pipeLogic;

    //clock_network clockNetwork;
    MemoryController(ParseXML *XML_interface,CACTI::InputParameter* interface_ip_, enum MemoryCtrl_type mc_type_);
    void set_mc_param();
    void computeEnergy(bool is_tdp=true);
    void displayEnergy(std::ostream &out,uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
    ~MemoryController();

  private:
  	template<typename T>
  	double results_area_MM2(T sub_ptr){
  		return sub_ptr->area.get_area()*1e-6;
  	}
  	template<typename T>
  	double results_peak_dyn_W(T sub_ptr){
  		return sub_ptr->power.total().dynamic*mcp.clockRate;
  	}
  	template<typename T>
  	double results_sub_leak_W(T sub_ptr){
  		bool long_channel = XML->sys.longer_channel_device;
  		return (long_channel ? sub_ptr->power.total().longer_channel_leakage : sub_ptr->power.total().leakage);
  	}
  	template<typename T>
  	double results_gated_sub_leak_W(T sub_ptr){
  		bool long_channel = XML->sys.longer_channel_device;
  		bool power_gating = XML->sys.power_gating;
  		if(power_gating)
  			return (sub_ptr->power.total().power_gated_leakage * (long_channel ? sub_ptr->power.total().longer_channel_leakage / sub_ptr->power.total().leakage : 1) );
  		else
  			return results_sub_leak_W(sub_ptr);
  	}
  	template<typename T>
  	double results_gate_leak_W(T sub_ptr){
  		return sub_ptr->power.total().gate_leakage;
  	}
  	template<typename T>
  	double results_rt_dyn_W(T sub_ptr){
  		return sub_ptr->rt_power.total().dynamic/mcp.executionTime;
  	}
  	template<typename T>
  	double results_avg_total_W(T sub_ptr){
  		return results_sub_leak_W(sub_ptr)+results_gate_leak_W(sub_ptr)+results_rt_dyn_W(sub_ptr);
  	}
  	template<typename T>
  	double results_peak_total_W(T sub_ptr){
  		return results_sub_leak_W(sub_ptr)+results_gate_leak_W(sub_ptr)+results_peak_dyn_W(sub_ptr);
  	}
  	template<typename T>
  	double results_energy_J(T sub_ptr){
  		return results_avg_total_W(sub_ptr)*mcp.executionTime;
  	}
  public:
  	double results_area_MM2() { return results_area_MM2(this); }
  	double results_peak_dyn_W() { return results_peak_dyn_W(this); }
  	double results_sub_leak_W() { return results_sub_leak_W(this); }
  	double results_gated_sub_leak_W() { return results_gated_sub_leak_W(this); }
  	double results_gate_leak_W() { return results_gate_leak_W(this); }
  	double results_rt_dyn_W() { return results_rt_dyn_W(this); }
  	double results_avg_total_W() { return results_avg_total_W(this); }
  	double results_peak_total_W() { return results_peak_total_W(this); }
  	double results_energy_J() { return results_energy_J(this); }
  	double results_mcpat_time() { return mcp.executionTime; }
};

};

#endif /* MEMORYCTRL_H_ */
