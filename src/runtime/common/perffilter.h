#ifndef __arm_rt_perffilter_h
#define __arm_rt_perffilter_h

#include <runtime/interfaces/performance_data.h>

class PerfFilter
{

	static const int STABLE_SAMPLES = 5;

	double _currBeatsRef;
	double _currBeats;
	double _currBeatsFiltered;
	double _currIPS;
	double _currIPSFiltered;
	double _currBeatsToIPSMap;
	double _avgWeight;
	int _samples;

public:
	PerfFilter()
		:_currBeatsRef(0),_currBeats(0),_currBeatsFiltered(0),_currIPS(0),_currIPSFiltered(0),_currBeatsToIPSMap(1),_avgWeight(0),
		 _samples(0){ }

	bool stable() { return _samples >= STABLE_SAMPLES;}

	double beatsRef() { return _currBeatsRef;}
	void beatsRef(double ref) { _currBeatsRef = ref;}

	double ips() { return _currIPS;}
	double ipsFiltered() { return _currIPSFiltered;}

	double ipsRef() { return stable() ? _currBeatsToIPSMap*_currBeatsRef : ips();}

	double beats() { return _currBeats;}
	double beatsFiltered() { return _currBeatsFiltered;}

	double normBeats() { return _currBeats/_currBeatsRef;}
	double normBeatsFiltered() { return _currBeatsFiltered/_currBeatsRef;}

	//curr ips normalized to the ips ref obtained from the beats ref
	double normPerf() { return ips()/ipsRef();}
	double normPerfFiltered() { return ipsFiltered()/ipsRef();}

	void beatsFilter(double val) { _avgWeight = val;}
	//sets filter based on the window length
	void beatsFilter(int window_length_ms) { _avgWeight = (window_length_ms) > 1000 ? 0 : 1-(((double)window_length_ms*1) / 1000.0);}

	//sums-up the beats of all tasks and their respective ips
	//updates a beats->ips mapping using an average filter
	void sampleTasks(const PerformanceData& data, int wid)
	{
		_currBeats = 0;
		uint64_t instr = 0;
		//pinfo("\n");
		for(int i = 0; i < data.numCreatedTasks(); ++i){
			if(data.task(i).num_beat_domains > 0){
				assert_true(data.task(i).num_beat_domains==1);
				if (data.swCurrData(wid).tasks[i].beats[0] < (data.swRawData(wid).curr_sample_time_ms - data.swRawData(wid).prev_sample_time_ms)*1000.0)
					_currBeats += data.swCurrData(wid).tasks[i].beats[0];
				instr += data.getPerfcntVal(data.swCurrData(wid).tasks[i].perfcnt, PERFCNT_INSTR_EXE);
				//pinfo("%d - beats\n",data.task(i).this_task_pid);
			}
			else if(data.task(i).parent_has_beats){
				instr += data.getPerfcntVal(data.swCurrData(wid).tasks[i].perfcnt, PERFCNT_INSTR_EXE);
				//pinfo("%d - parent beats\n",data.task(i).this_task_pid);
			}
		}
		double time = (data.swRawData(wid).curr_sample_time_ms - data.swRawData(wid).prev_sample_time_ms)/1000.0;
		if(time>0)
			_currIPS = instr/time;
		else
			_currIPS = 0;

		_currBeatsFiltered = (_avgWeight*_currBeatsFiltered) + ((1-_avgWeight)*_currBeats);
		_currIPSFiltered = (_avgWeight*_currIPSFiltered) + ((1-_avgWeight)*_currIPS);

		_currBeatsToIPSMap = _currIPSFiltered / _currBeatsFiltered;

		_samples += 1;
	}


};


#endif

