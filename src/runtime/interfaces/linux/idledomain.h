#ifndef __arm_rt_idledomain_h
#define __arm_rt_idledomain_h

#include <core/core.h>
#include <sched.h>

#include <map>
#include <sstream>
#include "../performance_data.h"

class IdleDomain
{
	const sys_info_t &_sys;
	const freq_domain_info_t &_domain;
	int _currCoreCnt;
	cpu_set_t *_level_masks;
	cpu_set_t _filter_mask;
	std::map<pid_t,int> _task_map;

	static std::map<pid_t,cpu_set_t> _task_map_orig_mask;

	std::string mask2str(const cpu_set_t &mask){
		std::stringstream ss;
		for (int i = 0; i < _sys.core_list_size; i++)
			if (CPU_ISSET(i, &mask)) ss << "1";
			else ss << "0";
		return ss.str();
	}

	bool maskCmp(const cpu_set_t &mask1,const cpu_set_t &mask2){
		for (int i = 0; i < _sys.core_list_size; i++)
			if(CPU_ISSET(i, &mask1) != CPU_ISSET(i, &mask2)) return false;
		return true;
	}

	void init_masks()
	{
		//all bits = 0, except for the cores in this domain
		CPU_ZERO( &_filter_mask );
		for(int i = 0; i < _sys.core_list_size; ++i)
			if(_sys.core_list[i].freq->domain_id == _domain.domain_id)
				CPU_SET( i, &_filter_mask );

		//for each different core count, creates a mask with
		//all bits = 1, except for the cores that will be disabled in this domain
		_level_masks = new cpu_set_t[_domain.core_cnt+1];
		for(int i = 0; i <= _domain.core_cnt; ++i){
			CPU_ZERO(&(_level_masks[0]));
			for(int core = 0; core < _sys.core_list_size; ++core) CPU_SET( core, &(_level_masks[i]) );

			int unset_core_cnt = 0;
			for(int core = 0; core < _sys.core_list_size; ++core){
				if(unset_core_cnt == (_domain.core_cnt-i)) break;
				if(_sys.core_list[core].freq->domain_id == _domain.domain_id){
					CPU_CLR( core, &(_level_masks[i]) );
					unset_core_cnt+=1;
				}

			}
		}
	}

public:
	IdleDomain(const sys_info_t *sys, freq_domain_info_t &domain)
		:_sys(*sys),_domain(domain), _currCoreCnt(domain.core_cnt), _level_masks(nullptr){ init_masks();}

	~IdleDomain(){ delete[] _level_masks; }


	//attempt limits the number of active cores in the domain by changing the core affinity mask of all
	//tasks that executed in this domain during a sensing window
	//this overwrites the task core affinity mask for all cores in this domain
	//minimum value for cores is 1
	//maxmum value is _domain.core_cnt
	//return the actual value set
	int idleCores(int cores, const PerformanceData& data, int wid, const int cores_min=1)
	{
		assert_true(cores_min>=0);
		_currCoreCnt = (cores < cores_min) ? cores_min : ((cores > _domain.core_cnt) ? _domain.core_cnt : cores);

		for(int i = 0; i < data.numCreatedTasks(); ++i){
			pid_t task_pid = data.task(i).this_task_pid;

			if(_task_map_orig_mask.find(task_pid)==_task_map_orig_mask.end()){
				cpu_set_t affmask;
				sched_getaffinity(task_pid,sizeof( cpu_set_t ), &affmask);
				_task_map_orig_mask[task_pid] = affmask;
			}

			if(_task_map.find(task_pid)==_task_map.end())
				_task_map[task_pid] = _domain.core_cnt;

			if(_task_map[task_pid] != _currCoreCnt){
				//we only care about the original state for this cluster
				cpu_set_t orig;
				CPU_AND(&orig,&(_task_map_orig_mask[task_pid]),&_filter_mask);

				cpu_set_t affmask;
				sched_getaffinity(task_pid,sizeof( cpu_set_t ), &affmask);

				CPU_OR(&affmask,&affmask,&orig);
				CPU_AND(&affmask,&affmask,&(_level_masks[_currCoreCnt]));
				sched_setaffinity( task_pid, sizeof( cpu_set_t ), &affmask );
				_task_map[task_pid] = _currCoreCnt;
			}
		}
		return _currCoreCnt;
	}


	//just returns the current value
	int idleCores()
	{
		return _currCoreCnt;
	}

	bool atDomain(const freq_domain_info_t &domain){
		return _domain.domain_id == domain.domain_id;
	}
};


#endif

