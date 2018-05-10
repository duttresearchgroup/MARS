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

#ifndef __arm_rt_sensed_data_h
#define __arm_rt_sensed_data_h

#include <runtime/interfaces/common/sense_data_shared.h>
#include <runtime/framework/sensing_interface.h>

// Global per-thread ptr to performance so sensing interfaces can
// more easily be implemented
class PerformanceData;
extern thread_local const PerformanceData* __localData;

class PerformanceData {
    friend class LinuxSensingModule;
    friend class OfflineSensingModule;
    friend class SensingInterfaceImpl;

  private:

    const perf_data_t *_raw_data;

    PerformanceData() :_raw_data(nullptr) {}

	PerformanceData(perf_data_t *raw_data)
        :_raw_data(raw_data)
    {
	}

	static void localData(PerformanceData* data)
	{
	    __localData = data;
	}

  public:

	static const PerformanceData& localData()
	{
	    assert_true(__localData != nullptr);
	    return *__localData;
	}


	// TODO need to include a task iterator that skips tasks with core_id = -1

	inline const tracked_task_data_t& task(int idx) const {
		assert_false(idx >= _raw_data->created_tasks_cnt);
		return _raw_data->created_tasks[idx];
	}

	inline int numCreatedTasks(int wid) const {
	    assert_false(wid >= MAX_WINDOW_CNT);
	    return _raw_data->sensing_windows[wid].created_tasks_cnt;
	}

	// This may change at any time, so it's dangerous to use in the middle of a window
	inline int numCreatedTasks() const { return _raw_data->created_tasks_cnt;}

	inline uint32_t numMinumumPeriods() { return _raw_data->num_of_minimum_periods;}
	inline uint32_t numCSWPeriods(int cpu) { return _raw_data->num_of_csw_periods[cpu];}

	inline uint32_t sensingStartTimeMS() const { return _raw_data->starttime_ms;}
	inline uint32_t sensingStopTimeMS() const { return _raw_data->stoptime_ms;}


	inline int numCpus() { return _raw_data->number_of_cpus;}

	inline uint32_t sysChecksum() const { return _raw_data->__sysChecksum; }

	inline int numMappedPerfcnts() const { return _raw_data->perfcnt_mapped_cnt;}

	inline bool perfCntAvailable(perfcnt_t cnt) const { return _raw_data->perfcnt_to_idx_map[cnt] >= 0;}

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
