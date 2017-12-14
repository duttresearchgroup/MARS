
#include <string>
#include <unistd.h>

#include <tclap/CmdLine.h>

#include <core/core.h>

#include "rt_config_params.h"

static const std::string UNSET_STR="unset";
static const int UNSET_NUM=-1;

static std::string daemon_file = UNSET_STR;
static std::string mode = UNSET_STR;
static int trace_core = UNSET_NUM;
static int map_overheadtest_core0 = UNSET_NUM;
static int map_overheadtest_core1 = UNSET_NUM;
static std::string predictor_filename = UNSET_STR;
static std::string idlepower_filename = UNSET_STR;
static std::string outdir = UNSET_STR;
static std::string sisotest_ctrlname = UNSET_STR;
static double sisotest_ref0 = UNSET_NUM;
static double sisotest_ref1 = UNSET_NUM;
static double sisotest_ref2 = UNSET_NUM;
static bool trace_counter_en[SIZE_PERFCNT] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static std::string sys_ctrl_gains = UNSET_STR;
static std::string sys_ctrl_hbtgt = UNSET_STR;
static std::string sys_ctrl_powertgt = UNSET_STR;
static std::string sys_ctrl_reftimes = UNSET_STR;

static char flag_fuck='a';
static inline std::string f(void){
	if(flag_fuck=='h')++flag_fuck;
	return std::string(1,flag_fuck++);
}

const std::string& rt_param_daemon_file()
{
	return daemon_file;
}

static void parse_sys_ctrl_refs();

bool init_rt_config_params(int argc, const char * argv[]){
	// Wrap everything in a try block.  Do this every time,
	// because exceptions will be thrown for problems.
	try {
		if(argc < 2) return true; //no args provided, use the defaults

		daemon_file = std::string(argv[0]);

		//preparse the module arg format to tclap format
		std::vector<std::string> argvector;
		std::vector<const char*> argvector_p;

		for(int i = 0; i < argc; ++i){
			if(i==0) {
				argvector.push_back(argv[i]);
			}
			else{
				std::string aux(argv[i]);
				auto pos = aux.find_first_of('=');
				if(pos != std::string::npos){
					argvector.push_back("--");
					argvector.back().append(aux.substr(0,pos));

					argvector.push_back(aux.substr(pos+1,aux.size()));
				}
				else{
					argvector.push_back("--");
					argvector.back().append(aux);
				}
			}
		}
		for(const auto &s : argvector) argvector_p.push_back(s.c_str());

		TCLAP::CmdLine cmd("Daemon command line arguments. No matter what this says, the arguments should be in the same format accepted by linux modules",
				' ',//delimiter
				"v0.00001"//version
				);

		TCLAP::ValueArg<std::string> p_mode(f(),"mode","Module mode",false,mode,"string");
		cmd.add(p_mode);

		TCLAP::ValueArg<int> p_trace_core(f(),"trace_core","Core to be traced(required if on TRACING mode)",false,trace_core,"int");
		cmd.add(p_trace_core);

		TCLAP::ValueArg<int> p_map_overheadtest_core0(f(),"map_overheadtest_core0","First core to be used in overhead test (required if on MAPPING_OVERHEAD mode)",false,map_overheadtest_core0,"int");
		cmd.add(p_map_overheadtest_core0);

		TCLAP::ValueArg<int> p_map_overheadtest_core1(f(),"map_overheadtest_core1","Second core to be used in overhead test (required if on MAPPING_OVERHEAD mode)",false,map_overheadtest_core1,"int");
		cmd.add(p_map_overheadtest_core1);

		TCLAP::ValueArg<std::string> p_predictor_filename(f(),"predictor_filename","Path to the predictor file (required if on MAPPING mode)",false,predictor_filename,"string");
		cmd.add(p_predictor_filename);

		TCLAP::ValueArg<std::string> p_idlepower_filename(f(),"idlepower_filename","Path to the idle power file (required if on MAPPING mode)",false,idlepower_filename,"string");
		cmd.add(p_idlepower_filename);

		TCLAP::ValueArg<std::string> p_outdir(f(),"outdir","Path to the directory that stores output files",false,outdir,"string");
		cmd.add(p_outdir);

		TCLAP::ValueArg<std::string> p_sisotest_ctrlname(f(),"sisotest_ctrlname","Controller name",false,sisotest_ctrlname,"string");
		cmd.add(p_sisotest_ctrlname);

		TCLAP::ValueArg<double> p_sisotest_ref0(f(),"sisotest_ref0","Controller name",false,sisotest_ref0,"double");
		cmd.add(p_sisotest_ref0);

		TCLAP::ValueArg<double> p_sisotest_ref1(f(),"sisotest_ref1","Controller name",false,sisotest_ref1,"double");
		cmd.add(p_sisotest_ref1);

		TCLAP::ValueArg<double> p_sisotest_ref2(f(),"sisotest_ref2","Controller name",false,sisotest_ref2,"double");
		cmd.add(p_sisotest_ref2);

		TCLAP::ValueArg<std::string> p_sys_ctrl_gains(f(),"sys_ctrl_gains","Controller gains",false,sys_ctrl_gains,"string");
		cmd.add(p_sys_ctrl_gains);
		TCLAP::ValueArg<std::string> p_sys_ctrl_hbtgt(f(),"sys_ctrl_hbtgt","Controller HB reference",false,sys_ctrl_hbtgt,"string");
		cmd.add(p_sys_ctrl_hbtgt);
		TCLAP::ValueArg<std::string> p_sys_ctrl_powertgt(f(),"sys_ctrl_powertgt","Controller power reference",false,sys_ctrl_powertgt,"string");
		cmd.add(p_sys_ctrl_powertgt);
		TCLAP::ValueArg<std::string> p_sys_ctrl_reftimes(f(),"sys_ctrl_reftimes","Controller power reference",false,sys_ctrl_reftimes,"string");
		cmd.add(p_sys_ctrl_reftimes);

		std::vector<TCLAP::SwitchArg*> p_trace_counters;
		std::vector<int> p_trace_counters_idx;

#define add_trace(name,idx)\
		TCLAP::SwitchArg p_##name(f(),#name,"",trace_counter_en[idx]);\
		p_trace_counters.push_back(&p_##name);\
		cmd.add(p_##name);\
		p_trace_counters_idx.push_back(idx);

		add_trace(cpuBusyCy,PERFCNT_BUSY_CY);
		add_trace(totalInstr,PERFCNT_INSTR_EXE);
		add_trace(branchInstr,PERFCNT_INSTR_BRANCHES);
		add_trace(memRdWrInstr,PERFCNT_INSTR_MEM);
		add_trace(memRdInstr,PERFCNT_INSTR_MEM_RD);
		add_trace(memWrInstr,PERFCNT_INSTR_MEM_WR);
		add_trace(brMisspred,PERFCNT_BRANCH_MISPRED);
		add_trace(dtlbMiss,PERFCNT_DTLB_MISSES);
		add_trace(itlbMiss,PERFCNT_ITLB_MISSES);
		add_trace(l1dcAccess,PERFCNT_L1DCACHE_ACCESS);
		add_trace(l1dcMiss,PERFCNT_L1DCACHE_MISSES);
		add_trace(l1icAccess,PERFCNT_L1ICACHE_ACCESS);
		add_trace(l1icMiss,PERFCNT_L1ICACHE_MISSES);
		add_trace(llcAccess,PERFCNT_LLCACHE_ACCESS);
		add_trace(llcMiss,PERFCNT_LLCACHE_MISSES);
		add_trace(busAccess,PERFCNT_BUS_ACCESS);
		add_trace(busCy,PERFCNT_BUS_CY);

		cmd.parse(argvector_p.size(), argvector_p.data());

		mode = p_mode.getValue();
		trace_core = p_trace_core.getValue();
		map_overheadtest_core0 = p_map_overheadtest_core0.getValue();
		map_overheadtest_core1 = p_map_overheadtest_core1.getValue();
		predictor_filename = p_predictor_filename.getValue();
		idlepower_filename = p_idlepower_filename.getValue();
		outdir = p_outdir.getValue();
		sisotest_ctrlname = p_sisotest_ctrlname.getValue();
		sisotest_ref0 = p_sisotest_ref0.getValue();
		sisotest_ref1 = p_sisotest_ref1.getValue();
		sisotest_ref2 = p_sisotest_ref2.getValue();
		sys_ctrl_gains = p_sys_ctrl_gains.getValue();
		sys_ctrl_hbtgt = p_sys_ctrl_hbtgt.getValue();
		sys_ctrl_powertgt = p_sys_ctrl_powertgt.getValue();
		sys_ctrl_reftimes = p_sys_ctrl_reftimes.getValue();
		for(unsigned i = 0; i < p_trace_counters.size(); ++i){
			trace_counter_en[p_trace_counters_idx[i]] = p_trace_counters[i]->getValue();
		}

		parse_sys_ctrl_refs();
	}
	catch (TCLAP::ArgException &e)  {
		pinfo("Error %s %s\n", e.error().c_str(), e.argId().c_str());
		return false;
	}
	return true;
}

const std::string& rt_param_model_predictor_file(void){
	return predictor_filename;
}

const std::string& rt_param_model_idlepower_file(void){
	return idlepower_filename;
}

const std::string& rt_param_mode(void){
	return mode;
}
int rt_param_trace_core(void){
	return trace_core;
}
int rt_param_overheadtest_core0(void){
	return map_overheadtest_core0;
}
int rt_param_overheadtest_core1(void){
	return map_overheadtest_core1;
}

bool rt_param_trace_perfcnt(perfcnt_t perfcnt) { return trace_counter_en[perfcnt]; }

const std::string& rt_param_outdir(void) {
	return outdir;
}

void rt_param_print(){
	int i;
    pinfo("\tmode = %s\n", rt_param_mode().c_str());
    pinfo("\toutdir = %s\n", rt_param_outdir().c_str());
    pinfo("\ttrace_core = %d\n", rt_param_trace_core());
    pinfo("\tmap_overheadtest_core0 = %d\n", rt_param_overheadtest_core0());
    pinfo("\tmap_overheadtest_core1 = %d\n", rt_param_overheadtest_core1());
    pinfo("\tidlepower_filename = %s\n", rt_param_model_idlepower_file().c_str());
    pinfo("\tpredictor_filename = %s\n", rt_param_model_predictor_file().c_str());
    pinfo("\tadditional perfcnts:\n");
    for(i = 0; i < SIZE_PERFCNT; ++i){
    	if(rt_param_trace_perfcnt((perfcnt_t)i)) {
    		if((i == PERFCNT_BUSY_CY) || (i == PERFCNT_INSTR_EXE))
    			pinfo("\t\t%s(always enabled)\n",perfcnt_str((perfcnt_t)i));
    		else
    			pinfo("\t\t%s\n",perfcnt_str((perfcnt_t)i));
    	}
    }
}

const std::string& rt_param_sisotest_ctrlname()
{
	return sisotest_ctrlname;
}
double rt_param_sisotest_ref0()
{
	return sisotest_ref0;
}
double rt_param_sisotest_ref1()
{
	return sisotest_ref1;
}
double rt_param_sisotest_ref2()
{
	return sisotest_ref2;
}

static int sys_ctrl_total_refs_cnt = 0;
static int sys_ctrl_sub_refs_cnt = 0;
static std::vector<double> sys_ctrl_refs_perf;
static std::vector<double> sys_ctrl_refs_pow;
static std::vector<int> sys_ctrl_refs_times;

template<typename T>
static std::vector<T> splitstr(const std::string &s, char delim){
	std::stringstream stream(s);
	std::vector<T> vals;
	while(true) {
	   T aux;
	   stream >> aux;
	   if(!stream)break;
	   vals.push_back(aux);
	   stream.ignore(1,delim);
	}
	return vals;
}

static void parse_sys_ctrl_refs()
{
	if((sys_ctrl_hbtgt != UNSET_STR) ||(sys_ctrl_powertgt != UNSET_STR) || (sys_ctrl_reftimes != UNSET_STR)){
		assert_true(sys_ctrl_hbtgt != UNSET_STR);
		assert_true(sys_ctrl_powertgt != UNSET_STR);
		assert_true(sys_ctrl_reftimes != UNSET_STR);

		sys_ctrl_refs_perf = splitstr<double>(sys_ctrl_hbtgt,',');
		sys_ctrl_refs_pow = splitstr<double>(sys_ctrl_powertgt,',');
		sys_ctrl_refs_times = splitstr<int>(sys_ctrl_reftimes,',');

		assert_true(sys_ctrl_refs_perf.size() == sys_ctrl_refs_pow.size());

		sys_ctrl_total_refs_cnt = sys_ctrl_refs_times.size();
		sys_ctrl_sub_refs_cnt = sys_ctrl_refs_perf.size();
	}
}

const std::string& rt_param_sys_ctrl_gains() { return sys_ctrl_gains;}
double* rt_param_sys_ctrl_refs_perf() { return sys_ctrl_refs_perf.data();}
double* rt_param_sys_ctrl_refs_power() { return sys_ctrl_refs_pow.data();}
int* rt_param_sys_ctrl_refs_enable_timeS(){ return sys_ctrl_refs_times.data(); }
int rt_param_sys_ctrl_total_refs_count() { return sys_ctrl_total_refs_cnt; }
int rt_param_sys_ctrl_sub_refs_count() { return sys_ctrl_sub_refs_cnt; }

