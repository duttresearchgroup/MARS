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
#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include "XML_Parse.h"
#include "cacti/area.h"
#include "cacti/decoder.h"
#include "cacti/parameter.h"
#include "array.h"
#include "cacti/arbiter.h"
#include <vector>
#include "basic_components.h"
#include "core.h"
#include "memoryctrl.h"
#include "cacti/router.h"
#include "sharedcache.h"
#include "noc.h"
#include "iocontrollers.h"
#include <iostream>

namespace McPAT {

class Processor : public CACTI::Component
{
  public:
	ParseXML *XML;
	std::vector<Core *> cores;
	std::vector<SharedCache *> l2array;
	std::vector<SharedCache *> l3array;
	std::vector<SharedCache *> l1dirarray;
	std::vector<SharedCache *> l2dirarray;
	std::vector<NoC *>  nocs;
    MemoryController * mc;
    NIUController    * niu;
    PCIeController   * pcie;
    FlashController  * flashcontroller;
    CACTI::InputParameter interface_ip;
    ProcParam procdynp;
    //wire	globalInterconnect;
    //clock_network globalClock;
    CACTI::Component core, l2, l3, l1dir, l2dir, noc, mcs, cc, nius, pcies,flashcontrollers;
    int  numCore, numL2, numL3, numNOC, numL1Dir, numL2Dir;
    Processor(ParseXML *XML_interface);
    void compute();
    void set_proc_param();
    void displayEnergy(std::ostream &out, uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
    void displayDeviceType(std::ostream &out, int device_type_, uint32_t indent = 0);
    void displayInterconnectType(std::ostream &out, int interconnect_type_, uint32_t indent = 0);
    ~Processor();
};

};

#endif /* PROCESSOR_H_ */
