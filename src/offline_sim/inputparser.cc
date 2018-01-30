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

#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cassert>
#include <thread>
#include <glob.h>
#include <mutex>

#include "semaphore.h"
#include "inputparser.h"

class CSVRow
{
  public:
    CSVRow() = default;

    CSVRow(const char* cstr) {
        std::stringstream ss(cstr);
        readNextRow(ss);
    }

    static const char separator = ';';

    std::string const& operator[](std::size_t index) const
    {
        assert(index < m_data.size());
        return m_data[index];
    }
    std::size_t size() const
    {
        return m_data.size();
    }
    void readNextRow(std::istream& str)
    {
    	line.clear();
        std::getline(str,line);

        std::stringstream   lineStream(line);
        std::string         cell;

        m_data.clear();
        while(std::getline(lineStream,cell,separator))
        {
            //std::cout << "\t" << cell << "\n";
            m_data.push_back(cell);
        }
    }

    std::vector<std::string>    m_data;
    std::string         line;
};

std::istream&
operator>>(std::istream& str,CSVRow& data)
{
    data.readNextRow(str);
    return str;
}


task_name_t
getTask(std::string &csvName, std::string origDir)
{
    std::vector<std::string> typeTokens = {"hugeCore","bigCore","mediumCore","littleCore","exynosA07Core","exynosA15Core"};
    std::string::size_type typePos = std::string::npos;
    for(auto t : typeTokens){
        auto aux = csvName.find(t);
        if(aux != std::string::npos){
            assert(typePos == std::string::npos);//must find only one match
            typePos = aux;
        }
    }
    assert((typePos != std::string::npos)&&"Invalid trace file name format");
    assert((csvName.at(typePos-1) == '_')&&"Invalid trace file name format");

    std::string name = csvName.substr(0,typePos-1);

    if(name.find(origDir) < std::string::npos) {
        name = name.substr(name.find(origDir)+origDir.length());
        if(name[0] == '/') name.erase(name.begin(), name.begin()+1);
        return name;
    }

    assert(false&&"Invalid trace file name format");
    return "";
}

#define CORE_CACHE_TYPE_HELPER(coreStr,coreType)\
    (csvName.find(#coreStr) != std::string::npos){\
        if(csvName.find("hugeCache") != std::string::npos) return coreType##_HUGE;\
        else if(csvName.find("bigCache") != std::string::npos) return coreType##_BIG;\
        else if(csvName.find("mediumCache") != std::string::npos) return coreType##_MEDIUM;\
        else if(csvName.find("littleCache") != std::string::npos) return coreType##_LITTLE;\
        else assert(false&&"Unknown cache type");\
    }

#define EXYNOS_CORE_TYPE_HELPER(coreStr,coreType)\
    (csvName.find(#coreStr) != std::string::npos){\
    if(csvName.find("hugeCache") != std::string::npos) return coreType##_HUGE;\
    else if(csvName.find("bigCache") != std::string::npos) return coreType##_BIG;\
    else if(csvName.find("mediumCache") != std::string::npos) return coreType##_MEDIUM;\
    else if(csvName.find("littleCache") != std::string::npos) return coreType##_LITTLE;\
    else if(csvName.find("exynosA07Cache") != std::string::npos) return coreType##_LITTLE;\
    else if(csvName.find("exynosA15Cache") != std::string::npos) return coreType##_BIG;\
    else return coreType;\
}

core_arch_t
getArch(std::string &csvName)
{
    //TODO please find a better way to do this !
    if CORE_CACHE_TYPE_HELPER(hugeCore,COREARCH_GEM5_HUGE)
    else if CORE_CACHE_TYPE_HELPER(bigCore,COREARCH_GEM5_BIG)
    else if CORE_CACHE_TYPE_HELPER(mediumCore,COREARCH_GEM5_MEDIUM)
    else if CORE_CACHE_TYPE_HELPER(littleCore,COREARCH_GEM5_LITTLE)
    else if EXYNOS_CORE_TYPE_HELPER(exynosA15Core,COREARCH_Exynos5422_BIG)
    else if EXYNOS_CORE_TYPE_HELPER(exynosA07Core,COREARCH_Exynos5422_LITTLE)
    else assert(false&&"Unknown core type");
    return (core_arch_t)0;
}

#define FREQ_IF_HELPER(freq)\
    (csvName.find("_"#freq".csv") != std::string::npos) return COREFREQ_##freq##MHZ;
#define FREQ_IF_HELPER_V(freqStr,freqVal)\
    (csvName.find("_"#freqStr".csv") != std::string::npos) return COREFREQ_##freqVal##MHZ;


core_freq_t
getFreq(std::string &csvName)
{
    if FREQ_IF_HELPER_V(max,2000)
    else if FREQ_IF_HELPER_V(min,0500)
    else if FREQ_IF_HELPER(0500)
    else if FREQ_IF_HELPER(0600)
    else if FREQ_IF_HELPER(0700)
    else if FREQ_IF_HELPER(0800)
    else if FREQ_IF_HELPER(0900)
    else if FREQ_IF_HELPER(1000)
    else if FREQ_IF_HELPER(1100)
    else if FREQ_IF_HELPER(1200)
    else if FREQ_IF_HELPER(1300)
    else if FREQ_IF_HELPER(1400)
    else if FREQ_IF_HELPER(1500)
    else if FREQ_IF_HELPER(1600)
    else if FREQ_IF_HELPER(1700)
    else if FREQ_IF_HELPER(1800)
    else if FREQ_IF_HELPER(1900)
    else if FREQ_IF_HELPER(2000)
    else if FREQ_IF_HELPER(3000)
    else assert(false&&"Unknown core frequency");
    return (core_freq_t)0;
}

#define assignString(row,val) \
do{\
    if(row.length()==0) val = 0;\
    else                istringstream(row) >> val;\
}while(0)


const CSVRow HEADER_GEM5=CSVRow("final_tick;system.cpu.issueWidth;system.cpu.freq_mhz;system.cpu.ipc_active;system.cpu.ipc_total;system.cpu.avg_dyn_power;system.cpu.avg_leak_power;system.cpu.gated_sub_leak_power;system.cpu.gate_leak_power;system.cpu.committedInsts;system.cpu.quiesceCycles;system.cpu.idleCycles;system.cpu.busyCycles;system.cpu.commit.refs;system.cpu.commit.fp_insts;system.cpu.commit.branches;system.cpu.commit.branchMispredicts;system.cpu.itb.fetch_accesses;system.cpu.itb.fetch_misses;system.cpu.dtb.data_accesses;system.cpu.dtb.data_misses;system.cpu.icache.overall_hits::total;system.cpu.icache.overall_misses::total;system.cpu.dcache.overall_hits::total;system.cpu.dcache.overall_misses::total;system.l2.overall_hits::total;system.l2.overall_misses::total;system.l2.total_avg_power;system.l2.sub_leak_power;system.l2.gate_leak_power");
double
gem5_parse_csv_data(std::ifstream &file, std::vector<task_data_t> &dataVector, core_arch_t arch, core_freq_t freq)
{
    CSVRow row;
    while(file >> row)
    {
        using std::istringstream;
        assert(row.size() == 30);


        task_data_t data;

        assignString(row[0], data.curr_time_original);
        data.data_format = task_data_t::CSV_PRED_AND_SIM;

        //assignString(row[1], data.data.archID);
        data.data.conf_arch = arch;
        uint32_t freqAux; assignString(row[2], freqAux);
        assert(freqAux == freqToValMHz_i(freq));
        data.data.conf_freq = freq;

        assignString(row[3], data.data.ipcActive);
        assignString(row[4], data.data.ipcTotal);
        assignString(row[5], data.data.avgDynPower);
        assignString(row[6], data.data.avgLeakPower);
        assignString(row[7], data.data.gatedSubThrLeakPower);
        assignString(row[8], data.data.gateLeakPower);
        assignString(row[9], data.data.commitedInsts);
        assignString(row[10], data.data.quiesceCycles);
        assignString(row[11], data.data.idleCycles);
        assignString(row[12], data.data.busyCycles);
        assignString(row[13], data.data.commitedMemRefs);
        assignString(row[14], data.data.commitedFPInsts);
        assignString(row[15], data.data.commitedBranches);
        assignString(row[16], data.data.branchMispredicts);
        assignString(row[17], data.data.itlbAccesses);
        assignString(row[18], data.data.itlbMisses);
        assignString(row[19], data.data.dtlbAccesses);
        assignString(row[20], data.data.dtlbMisses);
        assignString(row[21], data.data.iCacheHits);
        assignString(row[22], data.data.iCacheMisses);
        assignString(row[23], data.data.dCacheHits);
        assignString(row[24], data.data.dCacheMisses);
        assignString(row[25], data.data.l2CacheHits);
        assignString(row[26], data.data.l2CacheMisses);
        assignString(row[27], data.data.l2TotalAvgPower);
        assignString(row[28], data.data.l2SubThrLeakPower);
        assignString(row[29], data.data.l2GateLeakPower);

        data.data.totalPower = data.data.avgDynPower + data.data.avgLeakPower + data.data.l2TotalAvgPower;
        data.data.totalActiveCycles = data.data.idleCycles + data.data.busyCycles;

        dataVector.push_back(data);
    }

    //first and last samples are usually "contaminated"
    dataVector.erase(dataVector.begin());
    dataVector.pop_back();

    //obtain the distance between samples
    double skip = 0;
    int skipCnt = 0;
    for(unsigned int i = 1; i < dataVector.size(); ++i) {
        skip += dataVector[i].curr_time_original - dataVector[i-1].curr_time_original;
        skipCnt += 1;
    }
    skip = skip/skipCnt;

    //ajust the time and the instruction acc
    double offset = dataVector[0].curr_time_original - skip;
    int64_t instrAcc = 0;
    for(unsigned int i = 0; i < dataVector.size(); ++i) {
        dataVector[i].curr_time = dataVector[i].curr_time_original - offset;
        instrAcc += dataVector[i].data.commitedInsts;
        dataVector[i].curr_instr = instrAcc;
        //std::cout << "["<<i<<"] "<< dataVector[i].curr_time << " " << dataVector[i].curr_instr <<  "\n";
    }
    return skip;
}

const CSVRow HEADER_EXYNOS=CSVRow("instructions;branch_mispredictions;l1_cache_misses;l2_cache_misses;active_cycles;power");
double
exynos_parse_csv_data(std::string &fileName,std::ifstream &file, std::vector<task_data_t> &dataVector,core_arch_t arch, core_freq_t freq)
{
    CSVRow row;
    int samples = 0;
    double instructions_acc = 0;
    double active_cycles_acc = 0;
    double branch_mispredictions_acc = 0;
    double l1_cache_misses_acc = 0;
    double l2_cache_misses_acc = 0;
    double power_acc = 0;
    while(file >> row)
    {
        using std::istringstream;
        assert(row.size() == 6);

        uint64_t instructions = 0;
        uint64_t active_cycles = 0;
        uint64_t branch_mispredictions = 0;
        uint64_t l1_cache_misses = 0;
        uint64_t l2_cache_misses = 0;
        uint64_t power = 0;

        assignString(row[0], instructions);
        assignString(row[1], branch_mispredictions);
        assignString(row[2], l1_cache_misses);
        assignString(row[3], l2_cache_misses);
        assignString(row[4], active_cycles);
        assignString(row[5], power);

        //skip junk samples
        const uint64_t junk = 0xFFFFFFFFFF;
        if((instructions == 0) || (instructions > junk)
        		|| (active_cycles > junk)
        		|| (branch_mispredictions > junk)
        		|| (l1_cache_misses > junk)
        		|| (l2_cache_misses > junk)
        		|| (power > junk))
        	continue;

        if((instructions != 0) && ((power == 0) || (active_cycles == 0))){
        	//drop the entire thing
        	samples = 0;
        	break;
        }

        ++samples;
        instructions_acc += instructions;
        branch_mispredictions_acc += branch_mispredictions;
        l1_cache_misses_acc += l1_cache_misses;
        l2_cache_misses_acc += l2_cache_misses;
        active_cycles_acc += active_cycles;
        power_acc += power;
    }
    power_acc /= samples;

    task_data_t data;
    data.data_format = task_data_t::CSV_PRED_ONLY;
    data.curr_instr = instructions_acc;
    data.curr_time = active_cycles_acc/(freqToValMHz_d(freq)*1000000);

    data.curr_time_original = data.curr_time;

    data.data.conf_arch = arch;
    data.data.conf_freq = freq;
    data.data.ipcActive = data.curr_instr/active_cycles_acc;
    data.data.ipcTotal = data.data.ipcActive;
    data.data.avgDynPower = -1;
    data.data.avgLeakPower = -1;
    data.data.gatedSubThrLeakPower = -1;
    data.data.gateLeakPower = -1;
    data.data.l2TotalAvgPower = -1;
    data.data.l2SubThrLeakPower = -1;
    data.data.l2GateLeakPower = -1;
    data.data.totalPower = power_acc/1000000;//was uW
    data.data.commitedInsts = instructions_acc;
    data.data.quiesceCycles = 0;
    data.data.idleCycles = -1;
    data.data.busyCycles = -1;
    data.data.totalActiveCycles = active_cycles_acc;
    data.data.commitedMemRefs = -1;
    data.data.commitedFPInsts = -1;
    data.data.commitedBranches = -1;
    data.data.branchMispredicts = branch_mispredictions_acc;
    data.data.itlbAccesses = -1;
    data.data.itlbMisses = -1;
    data.data.dtlbAccesses = -1;
    data.data.dtlbMisses = -1;
    data.data.iCacheHits = -1;
    data.data.iCacheMisses = -1;
    data.data.dCacheHits = -1;
    data.data.dCacheMisses = l1_cache_misses_acc;
    data.data.l2CacheHits = -1;
    data.data.l2CacheMisses = l2_cache_misses_acc;

    //badness checks before adding
    if((samples == 0) ||
       ((arch==COREARCH_Exynos5422_BIG) && (data.data.ipcActive > 8)) ||
       ((arch==COREARCH_Exynos5422_BIG) && (data.data.totalPower > 16)) ||
       ((arch==COREARCH_Exynos5422_LITTLE) && (data.data.ipcActive > 4)) ||
       ((arch==COREARCH_Exynos5422_LITTLE) && (data.data.totalPower > 3)) ||
       //based on the idle power in the lowest frequency
       ((arch==COREARCH_Exynos5422_BIG) && (data.data.totalPower < 0.25)) ||
       ((arch==COREARCH_Exynos5422_LITTLE) && (data.data.ipcActive < 0.075))){
    	std::cout << "File " << fileName << " is bad\n";
    	return 0;
    }

    dataVector.push_back(data);

    return 0;
}


const CSVRow HEADER_EXYNOS_FORRSP=CSVRow("timestamp;total_time_s;busy_time_s;total_ips;busy_ips;util;power_w;freq_mhz;totalInstr;cpuBusyCy;brMisspred;l1dcMiss;llcMiss;nivcsw;nvcsw;beats0");
double
exynos_forrsp_parse_csv_data(std::string &fileName,std::ifstream &file, std::vector<task_data_t> &dataVector,core_arch_t arch, core_freq_t freq)
{
    double timestamp_prev = 0;
	CSVRow row;
    while(file >> row)
    {
        using std::istringstream;
        //std::cout << row.line << "\n";
        //std::cout << row.size() << " "  << HEADER_EXYNOS_FORRSP.size() << "\n";
        //the last col may be empty (we don't use it anyway)
        assert((row.size() == HEADER_EXYNOS_FORRSP.size()) || (row.size() == HEADER_EXYNOS_FORRSP.size()-1));

        double timestamp = 0;
        double total_time = 0;
        double instructions = 0;
        double active_cycles = 0;
        double branch_mispredictions = 0;
        double l1_cache_misses = 0;
        double l2_cache_misses = 0;
        double power = 0;
        double freqAux;

        assignString(row[0], timestamp);
        assignString(row[1], total_time);
        assignString(row[8], instructions);
        assignString(row[10], branch_mispredictions);
        assignString(row[11], l1_cache_misses);
        assignString(row[12], l2_cache_misses);
        assignString(row[9], active_cycles);
        assignString(row[6], power);
        assignString(row[7], freqAux);

        if((instructions != 0) && ((power == 0) || (active_cycles == 0))){
        	//drop the entire thing
        	dataVector.clear();
        	break;
        }

        //at least one instruction and one cycle
        if(instructions == 0) {
        	instructions += 1;
        	active_cycles += 1;
        	freqAux = freqToValMHz_d(freq);
        }
        if(total_time == 0){
        	total_time = timestamp - timestamp_prev;
        	freqAux = freqToValMHz_d(freq);
        }

		timestamp_prev = timestamp;

        double total_cycles = total_time * freqToValMHz_d(freq)*1000000;

        if(active_cycles > total_cycles){
        	if(active_cycles > (total_cycles*1.1)){
        		printf("FUCK: active_cycles > total_cycles\n");
        		printf("\t exynos_forrsp_parse_csv_data(file=%s, freq=%d, arch=%s\n)",fileName.c_str(),freqToValMHz_i(freq),archToString(arch));
        		printf("\t timestamp = %f\n", timestamp);
        		printf("\t total_time = %f\n", total_time);
        		printf("\t total_cycles = %f\n", total_cycles);
        		printf("\t activ_cycles = %f\n", active_cycles);
        		printf("\t freq = %f\n", freqToValMHz_d(freq));
        		printf("\t freq_aux = %f\n", freqAux);
        		printf("\t instr = %f\n", instructions);
        	}
        	active_cycles = total_cycles;
        }

        task_data_t data;
        data.data_format = task_data_t::CSV_PRED_AND_SIM;

        data.curr_time_original = timestamp;

        data.data.conf_arch = arch;
        data.data.conf_freq = freq;

        data.data.ipcActive = instructions/active_cycles;
        data.data.ipcTotal = instructions/total_cycles;

        /*power values are just placeholders*/
        data.data.avgDynPower = power;
        data.data.avgLeakPower = 0;
        data.data.gatedSubThrLeakPower = 0;
        data.data.gateLeakPower = 0;
        data.data.l2TotalAvgPower = 0;
        data.data.l2SubThrLeakPower = 0;
        data.data.l2GateLeakPower = 0;
        data.data.totalPower = power;

        data.data.commitedInsts = instructions;
        data.data.quiesceCycles = total_cycles - active_cycles;
        data.data.idleCycles = 0;
        data.data.busyCycles = active_cycles;
        data.data.totalActiveCycles = active_cycles;
        data.data.commitedMemRefs = 1;
        data.data.commitedFPInsts = 1;
        data.data.commitedBranches = 1;
        data.data.branchMispredicts = branch_mispredictions;
        data.data.itlbAccesses = 0;
        data.data.itlbMisses = 0;
        data.data.dtlbAccesses = 0;
        data.data.dtlbMisses = 0;
        data.data.iCacheHits = 0;
        data.data.iCacheMisses = 0;
        data.data.dCacheHits = 0;
        data.data.dCacheMisses = l1_cache_misses;
        data.data.l2CacheHits = 0;
        data.data.l2CacheMisses = l2_cache_misses;

        dataVector.push_back(data);
    }

    if(dataVector.size()==0) return 0;

    //obtain the distance between samples
    double skip = 0;
    int skipCnt = 0;
    for(unsigned int i = 1; i < dataVector.size(); ++i) {
    	skip += dataVector[i].curr_time_original - dataVector[i-1].curr_time_original;
    	skipCnt += 1;
    }
    skip = skip/skipCnt;

    //ajust the time and the instruction acc
    double offset = dataVector[0].curr_time_original - skip;
    int64_t instrAcc = 0;
    for(unsigned int i = 0; i < dataVector.size(); ++i) {
    	dataVector[i].curr_time = dataVector[i].curr_time_original - offset;
    	instrAcc += dataVector[i].data.commitedInsts;
    	dataVector[i].curr_instr = instrAcc;
    	//std::cout << "["<<i<<"] "<< dataVector[i].curr_time << " " << dataVector[i].curr_instr <<  "\n";
    }
    return skip;
}



double
parse_csv_data(std::string &csvFile, std::vector<task_data_t> &dataVector,core_arch_t arch, core_freq_t freq)
{
    std::ifstream file(csvFile);

    CSVRow row;
    file >> row;

    if((row.size() == HEADER_GEM5.size()) && (row[0] == HEADER_GEM5[0])){
        return gem5_parse_csv_data(file,dataVector,arch,freq);
    }
    else if((row.size() == HEADER_EXYNOS.size()) && (row[0] == HEADER_EXYNOS[0])){
        return exynos_parse_csv_data(csvFile,file,dataVector,arch,freq);
    }
    else if((row.size() == HEADER_EXYNOS_FORRSP.size()) && (row[0] == HEADER_EXYNOS_FORRSP[0])){
    	return exynos_forrsp_parse_csv_data(csvFile,file,dataVector,arch,freq);
    }
    else{
        std::cout << csvFile << " Is either incomplete or has wrong format!\n";
        return 0;
    }
}

struct rate_check {
    double sampleRate;
    std::string &csvFile;
};
std::vector<rate_check> sampleRates;
std::mutex sampleRateLock;
Semaphore sampleDone(0);

Semaphore *threadDone = nullptr;

void
parse_csv_data_thread(char *file, std::vector<task_data_t> &dataVector, core_arch_t arch, core_freq_t freq)
{
    std::string csvFile(file);
    double sampleRate = parse_csv_data(csvFile,dataVector,arch,freq);

    sampleRateLock.lock();
    rate_check rc = {sampleRate,csvFile};
    sampleRates.push_back(rc);
    sampleRateLock.unlock();

    sampleDone.notify();

    threadDone->notify();
}

std::mutex parseLock;
std::vector<std::thread *> threads;

struct glob_files {
    std::string file;
    std::string origDir;
};

inline void glob(const std::string& pat, std::string& origDir, std::vector<glob_files> &ret){
    using namespace std;
    glob_t glob_result;
    glob(pat.c_str(),GLOB_TILDE,NULL,&glob_result);
    for(unsigned int i=0;i<glob_result.gl_pathc;++i){
        ret.push_back({string(glob_result.gl_pathv[i]),origDir});
    }
    globfree(&glob_result);
}

input_data_t*
parse_csvs(std::vector<std::string> dirs)
{
	std::lock_guard<std::mutex> guard(parseLock);

    if(std::thread::hardware_concurrency() > 0) threadDone = new Semaphore(std::thread::hardware_concurrency());
    else                                        threadDone = new Semaphore(1);
    //threadDone = new Semaphore(1);

    input_data_t *input_data = new input_data_t;

    std::vector<glob_files> files;
    for(auto origDir : dirs){
        std::string srcDir = origDir;
        srcDir += "/*.csv";
        glob(srcDir,origDir,files);
    }

    std::cout << "Parsing "<< files.size() << " CSV files using "<<threadDone->getCount()<<" threads\n";


    //for (int i = 1; i < argc; ++i) {
    for(auto csv_file : files){
        //std::string csv_file = argv[i];
        char *aux = const_cast<char*>(csv_file.file.c_str());

        //std::cout << "CSV file: " << csv_file << "\n";

        task_name_t tname = getTask(csv_file.file, csv_file.origDir);
        core_arch_t arch = getArch(csv_file.file);
        core_freq_t freq = getFreq(csv_file.file);

        //std::cout << "task_name=" << tname << " arch=" << archToString(arch) << " freq=" << freqToString(freq) << "\n";

        if(input_data->samples.find(tname) == input_data->samples.end()){
            input_data->samples[tname] = std::map<core_arch_t,std::map<core_freq_t, std::vector<task_data_t>* > >();
        }
        if(input_data->samples[tname].find(arch) == input_data->samples[tname].end()){
            input_data->samples[tname][arch] = std::map<core_freq_t, std::vector<task_data_t>* >();
        }
        //no duplicates
        assert(input_data->samples[tname][arch].find(freq) == input_data->samples[tname][arch].end());

        input_data->samples[tname][arch][freq] = new std::vector<task_data_t>();

        //std::cout << "Parsing data\n";


        threadDone->wait();
        threads.push_back(new std::thread(parse_csv_data_thread, aux, std::ref(*(input_data->samples[tname][arch][freq])),arch,freq));

        //std::cout << "Done\n\n";
    }


    //check the result
    const double maxRateDiff = 0.00009;
    double avgRate = 0;
    double avgCnt = 0;

    for(auto csv_file : files){
        sampleDone.wait();

        sampleRateLock.lock();
        rate_check sampleRate = sampleRates.back();
        sampleRates.pop_back();
        sampleRateLock.unlock();

        if(sampleRate.sampleRate <= 0) continue;

        avgRate += sampleRate.sampleRate;
        avgCnt += 1;
        double currAvg = avgRate/avgCnt;

        //std::cout << "Rate for file " << sampleRate.csvFile << " = " << sampleRate.sampleRate << " sec  avgRate= "<<currAvg<<" sec\n";

        if(currAvg > sampleRate.sampleRate) assert((currAvg-sampleRate.sampleRate)<=maxRateDiff);
        else                     assert((sampleRate.sampleRate-currAvg)<=maxRateDiff);
    }

    for (auto t : threads){
        if(t->joinable()) t->join();
        delete t;
    }
    threads.clear();

    //std::cout << "rate= " << avgRate/avgCnt << " sec\n";

    //std::cout << "Summary: \n";
    //for(auto bench : input_data->samples){
    //    std::cout << "\t" << bench.first << "\n";
    //    for(auto arch : bench.second){
    //        std::cout << "\t\t" << archToString(arch.first) << "\n";
    //        for(auto freq : arch.second){
    //            std::cout << "\t\t\t" << freqToString(freq.first) << ", " << freq.second.size() << "samples\n";
    //        }
    //    }
    //}
    input_data->sampleRate = avgRate/avgCnt;

    return input_data;

}


