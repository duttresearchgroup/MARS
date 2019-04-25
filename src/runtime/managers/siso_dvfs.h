#ifndef __arm_rt_siso_dvfs_h
#define __arm_rt_siso_dvfs_h

#include <runtime/common/controllers.h>
#include <runtime/framework/policy.h>

class SISODVFSPolicy : public Policy {

    static constexpr int PERIOD_MS = 500;

  public:

    SISODVFSPolicy(int domain_id)
    : Policy(PERIOD_MS,"SISODVFSPolicy"),
    _ctrlTrace("ctrl_trace"),
    _ref_phase_cnt(0), _ref_time(0), _ref_phase(0),
    _samples(0),
    _p_ref(0),
    _domain_id(domain_id) {}

    ~SISODVFSPolicy() {
        for(auto iter : _execTracesCore){
            delete iter.second;
        }
    }

    void execute(int wid) override;

  protected:

    static const std::string OPT_REFS;
    using OPT_REFS_TYPE = std::string;
    static const std::string OPT_REF_INT;
    using OPT_REF_INT_TYPE = int;

    ExecutionTrace _ctrlTrace;
    std::unordered_map<int,ExecutionTrace*> _execTracesCore;

    ExecutionTrace::ExecutionTraceHandle& getHandleForCore(const core_info_t &core, int wid)
    {
        auto iter = _execTracesCore.find(core.position);
        if(iter != _execTracesCore.end())
            return iter->second->getHandle(wid);
        else{
            ExecutionTrace *execTrace = new ExecutionTrace("ctrl_trace.c"+std::to_string(core.position));
            _execTracesCore[core.position] = execTrace;
            return execTrace->getHandle(wid);
        }
    }

    int _ref_phase_cnt;
    int _ref_time;
    int _ref_phase;
    int _samples;

    virtual int curr_ref_phase();
    virtual void set_ctrl_ref();
    void controller_gains_all();
    virtual void init_ctrl();
    void parse_refs();
    void onRegister() override;

  private:

    std::vector<double> _p_ref;

    typedef Controllers::SISO<double,
                    Filters::Error<double,1>,
                    Filters::Offset<double,1>> SISOController;

  protected:

    SISOController _ctrl_f_ips;
    int _domain_id;

    uint32_t freqToMHz(unsigned int freq){
        unsigned int min = actuationRanges<ACT_FREQ_MHZ>(&(info()->freq_domain_list[_domain_id])).min;
        unsigned int max = actuationRanges<ACT_FREQ_MHZ>(&(info()->freq_domain_list[_domain_id])).max;
        unsigned int step = actuationRanges<ACT_FREQ_MHZ>(&(info()->freq_domain_list[_domain_id])).steps;

        for (unsigned int curr_freq = min; curr_freq < max; curr_freq += step) {
            if (curr_freq + step > freq) {
                return curr_freq;
            }
        }
        return max;
    }
};



#endif
