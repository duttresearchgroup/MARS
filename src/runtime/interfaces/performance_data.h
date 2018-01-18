/*
 * sensing_module.h
 *
 *  Created on: Dec 8, 2016
 *      Author: tiago
 */

#ifndef __arm_rt_sensed_data_h
#define __arm_rt_sensed_data_h

#include <runtime/interfaces/common/sense_data_shared.h>
#include <runtime/framework/sensing_interface.h>

class PerformanceData {
    friend class LinuxSensingModule;
    friend class OfflineSensingModule;
    friend class SensingInterface;

  private:

    const perf_data_t *_raw_data;

	//special constructor should only be called from the friend class
	PerformanceData():_raw_data(nullptr){}

	PerformanceData(perf_data_t *raw_data) :_raw_data(raw_data){
#if defined(IS_LINUX_PLAT)
		if(!check_perf_data_cksum(raw_data)) arm_throw(SensingDataException,"Wrong checksum in mapped shared data");
#endif
	}

  public:

	inline const tracked_task_data_t& task(int idx) const {
		assert_false(idx >= _raw_data->created_tasks_cnt);
		return _raw_data->created_tasks[idx];
	}
	inline const tracked_task_data_t& task(tracked_task_data_t *t) const { return task(t->task_idx); }
	inline const tracked_task_data_t& task(tracked_task_data_t &t) const { return task(t.task_idx); }

	inline int numCreatedTasks() const { return _raw_data->created_tasks_cnt;}

	inline uint32_t numMinumumPeriods() { return _raw_data->num_of_minimum_periods;}
	inline uint32_t numCSWPeriods(int cpu) { return _raw_data->num_of_csw_periods[cpu];}

	inline uint32_t sensingStartTimeMS() const { return _raw_data->starttime_ms;}
	inline uint32_t sensingStopTimeMS() const { return _raw_data->stoptime_ms;}


	inline int numCpus() { return _raw_data->number_of_cpus;}

	inline uint32_t sysChecksum() const { return _raw_data->__sysChecksum; }

	inline int numMappedPerfcnts() const { return _raw_data->perfcnt_mapped_cnt;}

	inline bool perfCntAvailable(perfcnt_t cnt) { return _raw_data->perfcnt_to_idx_map[cnt] >= 0;}

	inline perfcnt_t perfcntFromIdx(int idx) const { return (perfcnt_t)(_raw_data->idx_to_perfcnt_map[idx]);}

	inline uint64_t currWindowTimeMS(int wid) const {
		assert_false(wid >= MAX_WINDOW_CNT);
		return _raw_data->sensing_windows[wid].curr_sample_time_ms;
	}
	inline uint64_t prevWindowTimeMS(int wid) const {
		assert_false(wid >= MAX_WINDOW_CNT);
		return _raw_data->sensing_windows[wid].prev_sample_time_ms;
	}
	inline uint32_t windowCount(int wid){
		assert_false(wid >= MAX_WINDOW_CNT);
		return _raw_data->sensing_windows[wid].num_of_samples;
	}

  private:

	inline const perf_window_data_t& swCurrData(int wid) const {
		assert_false(wid >= MAX_WINDOW_CNT);
		return _raw_data->sensing_windows[wid].curr;
	}
	inline const perf_window_data_t& swAggrData(int wid) const {
		assert_false(wid >= MAX_WINDOW_CNT);
		return _raw_data->sensing_windows[wid].aggr;
	}
	inline const perf_window_t& swRawData(int wid) const {
		assert_false(wid >= MAX_WINDOW_CNT);
		return _raw_data->sensing_windows[wid];
	}

    inline uint64_t getPerfcntVal(const perf_data_perf_counters_t &perfcnt, int perfcnt_id) const{
        return perfcnt.perfcnts[_raw_data->perfcnt_to_idx_map[perfcnt_id]];
    }
    inline uint64_t getPerfcntVal(const perf_data_perf_counters_t &perfcnt, perfcnt_t perfcnt_id) const{
        return perfcnt.perfcnts[_raw_data->perfcnt_to_idx_map[perfcnt_id]];
    }
};


#endif /* SENSING_MODULE_H_ */
