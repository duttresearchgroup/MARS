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

#ifndef NOC_H_
#define NOC_H_
#include "XML_Parse.h"
#include "logic.h"
#include "cacti/parameter.h"
#include "array.h"
#include "interconnect.h"
#include "basic_components.h"
#include "cacti/router.h"

namespace McPAT {

class NoC :public CACTI::Component {
  public:

	ParseXML *XML;
	int  ithNoC;
	CACTI::InputParameter interface_ip;
	double link_len;
	double executionTime;
	double scktRatio, chip_PR_overhead, macro_PR_overhead;
	CACTI::Router * router;
	interconnect * link_bus;
	NoCParam  nocdynp;
	CACTI::uca_org_t local_result;
	statsDef       tdp_stats;
	statsDef       rtp_stats;
	statsDef       stats_t;
	CACTI::powerDef       power_t;
	CACTI::Component      link_bus_tot_per_Router;
	bool link_bus_exist;
	bool router_exist;
	std::string name, link_name;
	double M_traffic_pattern;
	NoC(ParseXML *XML_interface, int ithNoC_, CACTI::InputParameter* interface_ip_, double M_traffic_pattern_ = 0.6,double link_len_=0);
	void set_noc_param();
	void computeEnergy(bool is_tdp=true);
	void displayEnergy(std::ostream &out, uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
	void init_link_bus(double link_len_);
	void init_router();
	void computeEnergy_link_bus(bool is_tdp=true);
	void displayEnergy_link_bus(std::ostream &out, uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
	~NoC();

  private:
  	template<typename T>
  	double results_area_MM2(T sub_ptr){
  		return sub_ptr->area.get_area()*1e-6;
  	}
  	template<typename T>
  	double results_peak_dyn_W(T sub_ptr){
  		return sub_ptr->power.total().dynamic*nocdynp.clockRate;
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
  		return sub_ptr->rt_power.total().dynamic/nocdynp.executionTime;
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
  		return results_avg_total_W(sub_ptr)*nocdynp.executionTime;
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
  	double results_mcpat_time() { return nocdynp.executionTime; }

};

};

#endif /* NOC_H_ */
