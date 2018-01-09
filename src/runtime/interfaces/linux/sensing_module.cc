/*
 * sensing_module.cc
 *
 *  Created on: Dec 8, 2016
 *      Author: tiago
 */

#include "sensing_module.h"

#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdexcept>
#include <system_error>

#include <runtime/interfaces/common/sense_data_shared.h>
#include <runtime/interfaces/common/sensing_window_defs.h>
#include <runtime/interfaces/common/user_if_shared.h>
#include <runtime/interfaces/common/pal/sensing_setup.h>

#include <core/core.h>

#include <runtime/common/rt_config_params.h>

bool LinuxSensingModule::_attached = false;

LinuxSensingModule::LinuxSensingModule()
	:_module_file_if(0), _module_shared_mem_raw_ptr(nullptr),
	 _sensingRunning(false),
	 _numCreatedWindows(0)
{
	if(_attached) arm_throw(LinuxSensingModuleException,"There can be only one connection with the sensing module");

	_module_file_if = open(MODULE_SYSFS_PATH, O_RDWR);
    if(_module_file_if < 0)
    	arm_throw(LinuxSensingModuleException,"Vitamins module not inserted errno=%d",errno);

    resgisterAsDaemonProc();

    _module_shared_mem_raw_ptr = mmap(NULL, sizeof(sensed_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, _module_file_if, 0);

    if(_module_shared_mem_raw_ptr == MAP_FAILED) arm_throw(LinuxSensingModuleException,"mmap error");

    _sensed_data = SensedData(reinterpret_cast<sensed_data_t*>(_module_shared_mem_raw_ptr));

    //setup the platform sensors
    pal_sensing_setup(this);

    _attached = true;

}

LinuxSensingModule::~LinuxSensingModule()
{
    _attached = false;

	if(_sensingRunning)
    	sensingStop();

	if(munmap(_module_shared_mem_raw_ptr,sizeof(sensed_data_t)) < 0)
    	pinfo("LinuxSensingModule::~LinuxSensingModule: munmap failed with errno=%d!\n",errno);

	if(!unresgisterAsDaemonProc())
		pinfo("LinuxSensingModule::~LinuxSensingModule: IOCTLCMD_UNREGISTER_DAEMON failed with errno=%d!\n",errno);

    if(close(_module_file_if) < 0)
    	pinfo("LinuxSensingModule::~LinuxSensingModule: close failed with errno=%d!\n",errno);

    //unsetup the platform sensors
    pal_sensing_teardown(this);
}

void LinuxSensingModule::forceDetach()
{
	munmap(_module_shared_mem_raw_ptr,sizeof(sensed_data_t));
    close(_module_file_if);
}

void LinuxSensingModule::sensingStart()
{
	if(_sensingRunning)
		arm_throw(LinuxSensingModuleException,"Sensing already runing");

	if(ioctl(_module_file_if, IOCTLCMD_SENSING_START,0) !=0)
		arm_throw(LinuxSensingModuleException,"IOCTLCMD_SENSING_START failed errno=%d",errno);

	// Start the other sensors
	_psensingManager.setSensingWindows(_numCreatedWindows);
	_psensingManager.startSensing();

	_sensingRunning = true;
}

void LinuxSensingModule::sensingStop()
{
	if(_sensingRunning){
		_psensingManager.stopSensing();

		if(ioctl(_module_file_if, IOCTLCMD_SENSING_STOP,0) != 0)
			arm_throw(LinuxSensingModuleException,"IOCTLCMD_SENSING_STOP failed errno=%d",errno);
	}
	else
		pinfo("LinuxSensingModule::sensingStop: sensing was not running!\n");
	_sensingRunning = false;
}

int LinuxSensingModule::createSensingWindow(int period_ms)
{
	int wid = ioctl(_module_file_if, IOCTLCMD_SENSE_WINDOW_CREATE,period_ms);
	//returned id must be either a positive integer or one of the special window IDs
	if(wid < 0){
		if(wid & WINDOW_ID_MASK){
			switch (wid) {
				case WINDOW_EXIT:
					arm_throw(LinuxSensingModuleException,"IOCTLCMD_SENSE_WINDOW_CREATE returned WINDOW_EXIT errno=%d",errno);
				case WINDOW_INVALID_PERIOD:
					arm_throw(LinuxSensingModuleException,"Sensing period must be multiple of MINIMUM_WINDOW_LENGHT_MS errno=%d",errno);
				case WINDOW_MAX_NWINDOW:
					arm_throw(LinuxSensingModuleException,"Maximum number of sensing windows created errno=%d",errno);
				case WINDOW_EXISTS:
					arm_throw(LinuxSensingModuleException,"Window for the specified period already exists errno=%d",errno);
				default:
					arm_throw(LinuxSensingModuleException,"IOCTLCMD_SENSE_WINDOW_CREATE failed errno=%d",errno);
			}
		}
		arm_throw(LinuxSensingModuleException,"IOCTLCMD_SENSE_WINDOW_CREATE failed errno=%d",errno);
	}
	else{
		_numCreatedWindows += 1;
		return wid;
	}
}

int LinuxSensingModule::nextSensingWindow()
{
	//blocks until a window is ready
	int wid = ioctl(_module_file_if, IOCTLCMD_SENSE_WINDOW_WAIT_ANY,0);

	//returned id must be either a positive integer or the special WINDOW_EXIT IDs
	if(wid < 0){
		if(wid == WINDOW_EXIT) return wid;
		else arm_throw(LinuxSensingModuleException,"IOCTLCMD_SENSE_WINDOW_WAIT_ANY failed errno=%d",errno);
	}
	else {
		_psensingManager.windowReady(wid);
		return wid;
	}
}

bool LinuxSensingModule::isPerfCntAvailable(perfcnt_t cnt)
{
	//printk("vitamins_is_perfcnt_available(%d)=%d %d",perfcnt,perfcnt_to_idx_map[perfcnt],perfcnt_to_idx_map[perfcnt] >= 0);
	return _sensed_data.perfCntAvailable(cnt);
}

void LinuxSensingModule::enablePerTaskSensing()
{
	if(_sensingRunning)
		arm_throw(LinuxSensingModuleException,"Cannot do enablePerTaskSensing with sensing running errno=%d",errno);

	if(ioctl(_module_file_if, IOCTLCMD_ENABLE_PERTASK_SENSING,1) < 0)
		arm_throw(LinuxSensingModuleException,"IOCTLCMD_ENABLE_PERTASK_SENSING failed errno=%d",errno);
}

void LinuxSensingModule::pinAllTasksToCPU(int cpu)
{
	if(_sensingRunning)
		arm_throw(LinuxSensingModuleException,"Cannot do pinAllTasksToCPU with sensing running errno=%d",errno);

	if(ioctl(_module_file_if, IOCTLCMD_ENABLE_PINTASK,cpu) < 0)
		arm_throw(LinuxSensingModuleException,"IOCTLCMD_ENABLE_PINTASK failed errno=%d",errno);
}

void LinuxSensingModule::tracePerfCounter(perfcnt_t perfcnt)
{
	if(_sensingRunning)
		arm_throw(LinuxSensingModuleException,"Cannot do tracePerfCounter with sensing running errno=%d",errno);

	if(ioctl(_module_file_if, IOCTLCMD_PERFCNT_ENABLE,perfcnt) < 0)
		arm_throw(LinuxSensingModuleException,"IOCTLCMD_PERFCNT_ENABLE failed errno=%d",errno);
}

void LinuxSensingModule::tracePerfCounterResetAll()
{
	if(_sensingRunning)
		arm_throw(LinuxSensingModuleException,"Cannot do tracePerfCounter with sensing running errno=%d",errno);

	if(ioctl(_module_file_if, IOCTLCMD_PERFCNT_RESET) < 0)
		arm_throw(LinuxSensingModuleException,"IOCTLCMD_PERFCNT_DISABLE failed errno=%d",errno);

}

void LinuxSensingModule::cleanUpCreatedTasks()
{
	for(int i = 0; i < _sensed_data.numCreatedTasks(); ++i) {
		if(_sensed_data._raw_data->created_tasks[i].tsk_model != nullptr)
			delete _sensed_data._raw_data->created_tasks[i].tsk_model;
	}
	const_cast<sensed_data_t*>(_sensed_data._raw_data)->created_tasks_cnt = 0;
}


void LinuxSensingModule::resgisterAsDaemonProc()
{
	if(ioctl(_module_file_if, IOCTLCMD_REGISTER_DAEMON,SECRET_WORD) !=0)
		arm_throw(LinuxSensingModuleException,"IOCTLCMD_REGISTER_DAEMON failed errno=%d",errno);
}

bool LinuxSensingModule::unresgisterAsDaemonProc()
{
	return ioctl(_module_file_if, IOCTLCMD_UNREGISTER_DAEMON,0) == 0;
}

void LinuxSensingModule::attachSensor(PeriodicSensor *sensor)
{
	_psensingManager.attachSensor(sensor);
}
