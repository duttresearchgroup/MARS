/*******************************************************************************
 * Copyright (C) 2018 Biswadip Maity <biswadip.maity@gmail.com>
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

#include <base/base.h>

#include <runtime/interfaces/linux/common/pal/sensing_setup.h>
#include <runtime/interfaces/sensing_module.h>
#include <runtime/interfaces/common/sensor.h>
#include <runtime/framework/sensing_interface.h>
#include <external/hookcuda/nvidia_counters.h>

#include <runtime/common/filesensor.h>

class INA3221 : public SensorBase<SEN_POWER_W,INA3221,SensingModule> {
  public:
	static constexpr const char* VDD_IN			= "/sys/bus/i2c/drivers/ina3221x/0-0041/iio:device1/in_power0_input";
	static constexpr const char* VDD_SYS_CPU	= "/sys/bus/i2c/drivers/ina3221x/0-0041/iio:device1/in_power1_input";
	static constexpr const char* VDD_SYS_DDR	= "/sys/bus/i2c/drivers/ina3221x/0-0041/iio:device1/in_power2_input";
	
	static constexpr const char* VDD_SYS_GPU	= "/sys/bus/i2c/drivers/ina3221x/0-0040/iio:device0/in_power0_input";
	static constexpr const char* VDD_SYS_SOC	= "/sys/bus/i2c/drivers/ina3221x/0-0040/iio:device0/in_power1_input";
	static constexpr const char* VDD_4V0_WIFI	= "/sys/bus/i2c/drivers/ina3221x/0-0040/iio:device0/in_power2_input";
	
  private:
	FileSensor fs;

  public:

	INA3221(const char *dev_path) :fs(dev_path)
	{
		
	}

	~INA3221()
	{
		
	}

	typename SensingTypeInfo<SEN_POWER_W>::ValType readSample()
	{
		return fs.read();
	}
};

class Tx2Temp : public SensorBase<SEN_TEMP_C,Tx2Temp,SensingModule> {
  public:
	static constexpr const char* BCPU_therm	= "/sys/devices/virtual/thermal/thermal_zone0/type";
	static constexpr const char* MCPU_therm	= "/sys/devices/virtual/thermal/thermal_zone1/type";
	static constexpr const char* GPU_therm	= "/sys/devices/virtual/thermal/thermal_zone2/type";
	
	static constexpr const char* PLL_therm		= "/sys/devices/virtual/thermal/thermal_zone3/type";
	static constexpr const char* Tboard_tegra	= "/sys/devices/virtual/thermal/thermal_zone4/type";
	static constexpr const char* Tdiode_tegra	= "/sys/devices/virtual/thermal/thermal_zone5/type";
	static constexpr const char* PMIC_Die 		= "/sys/devices/virtual/thermal/thermal_zone6/type";
	static constexpr const char* thermal_fan_est ="/sys/devices/virtual/thermal/thermal_zone7/type";

  private:
	FileSensor fs;

  public:

	Tx2Temp(const char *dev_path) :fs(dev_path)
	{
		
	}

	~Tx2Temp()
	{
		
	}

	typename SensingTypeInfo<SEN_POWER_W>::ValType readSample()
	{
		return fs.read();
	}
};


class Tx2GPUPerfCtr: public SensorBase<SEN_NV_GPU_PERFCNT, Tx2GPUPerfCtr, SensingModule> {
  private:
	// TODO: make semaphore and shm application specific
	static constexpr const char* SEM_METRICS_NAME		= "/sem-metrics";
	static constexpr const char* SEM_RESULT_NAME		= "/sem-results";
	static constexpr const char* SHARED_MEM_NAME		= "/shm-mars-hookcuda";

	struct shm_hookcuda *shm;
	sem_t *metrics_sem, *results_sem;
	int fd_shm;
	bool enabled, isMeasuring = false;
	nvidia_counters_t ctr;

  public:

	Tx2GPUPerfCtr()
	{
	    enabled = false;

	    if ((fd_shm = shm_open (SHARED_MEM_NAME, O_RDWR | O_CREAT, 0666)) == -1)
		arm_throw(SensingException, "shm_open");

	    if (ftruncate (fd_shm, sizeof (struct shm_hookcuda)) == -1)
		arm_throw(SensingException,"ftruncate");

	    if ((shm = (struct shm_hookcuda*)mmap (NULL, sizeof (struct shm_hookcuda), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
		arm_throw(SensingException, "mmap");

	    if ((metrics_sem = sem_open (SEM_METRICS_NAME, O_CREAT, 0666, 1)) == SEM_FAILED)
		arm_throw(SensingException, "sem_open");

	    if ((results_sem = sem_open (SEM_RESULT_NAME, O_CREAT, 0666, 1)) == SEM_FAILED)
		arm_throw(SensingException, "sem_open");

	    // Initialize the shared memory
	    if (shm->init != true)
	    {
		shm->num_metrics = 0;
		shm->exact_kernel_duration = 1;
		shm->num_kernels = 0;
		shm->init = true;
		memset(shm->kernels, 0, sizeof(kernel_data_t) * MAX_KERNEL_PER_POLICY_MANAGER);
	    }
	}

	int initialize(nvidia_counters_t type)
	{
	    if (type < 0 || type > NVIDIA_COUNTERS_MAX)
	    {
		arm_throw(SensingException, "error: not supporting coutner %d", type);
		return -1;
	    }
	    ctr = type;

	    return 0;
	}

	int enable(bool enable)
	{
	    int i;

	    if (enabled == enable)
		return 1;

	    sem_wait (metrics_sem);

	    for (i = 0 ; i < shm->num_metrics ; ++i)
	    {
		if (shm->metrics[i] == ctr)
		{
		    memmove(&shm->metrics[i], 
			    &shm->metrics[i+1], 
			    (shm->num_metrics-i)*sizeof(int));
		    i--; 
		}
	    }

	    if (enable && i == shm->num_metrics )
	    {
		// TODO: find a way to configure below params in user policies
		shm->verbose = 0;
		shm->sampling_mode = EVENT_COLLECTION_MODE_KERNEL;
		shm->num_metrics++;
		shm->metrics[i] = ctr;
	    }

	    if (sem_post (metrics_sem) == -1)
		arm_throw(SensingException, "sem_post: metrics_sem");

	    enabled = enable;
	    return 1;
	}

	bool isEnabled()
	{
	    return enabled;
	}

	~Tx2GPUPerfCtr()
	{

	    shm->init = false;
	    if (munmap (shm, sizeof (struct shm_hookcuda)) == -1)
		arm_throw (SensingException, "munmap");
	}

	// look for doSampling()
	typename SensingTypeInfo<SEN_NV_GPU_PERFCNT>::ValType readSample()
	{
	    int i;
	    GpuPerfCtrRes ret;

	    if (enabled)
	    {
		sem_wait (results_sem);
		ret.num_kernels = shm->num_kernels;
		memcpy(ret.kernels, shm->kernels, sizeof(kernel_data_t) * shm->num_kernels);
		memset(shm->kernels, 0, sizeof(kernel_data_t) * MAX_KERNEL_PER_POLICY_MANAGER);

		for (i = 0 ; i < ret.num_kernels ; ++i)
		{
		    strcpy(shm->kernels[i].name, ret.kernels[i].name);
		    shm->kernels[i].ptr = ret.kernels[i].ptr;
		}

		if (sem_post (results_sem) == -1)
		    printf("sem_post: results_sem\n");
	    }
	    return ret;
	}
};


//helper to encapsulate sensor creation/destruciton
struct SensorWrapper {
	INA3221 cpuPower;
	INA3221 gpuPower;
	Tx2GPUPerfCtr  gpuPerfCtr[NVIDIA_COUNTERS_MAX];

	SensorWrapper(SensingModule *m)
		:cpuPower(INA3221::VDD_SYS_CPU),
		 gpuPower(INA3221::VDD_SYS_GPU)
	{
		m->attachSensor(&cpuPower);
		m->attachSensor(&gpuPower);

		for (int i = 0 ; i < NVIDIA_COUNTERS_MAX; ++i)
		{
		    gpuPerfCtr[i].initialize((nvidia_counters_t)i);
		    m->attachSensor(&gpuPerfCtr[i]);
		}
	}

	double cpuPowerW(int wid) { return cpuPower.accData(wid)/cpuPower.samples(wid);}
	double cpuAggPowerW(int wid) { return cpuPower.accDataAgg(wid)/cpuPower.samplesAgg(wid);}

	// Will not be used currently
	double gpuPowerW(int wid) { return gpuPower.accData(wid)/gpuPower.samples(wid);}
	double gpuAggPowerW(int wid) { return gpuPower.accDataAgg(wid)/gpuPower.samplesAgg(wid);}
	GpuPerfCtrRes getGpuPerfCtr(nvidia_counters_t type, int wid) { 
	    GpuPerfCtrRes ret;

	    if (gpuPerfCtr[type].isEnabled())
		return gpuPerfCtr[type].accData(wid);
	    else
		arm_throw(SensingException, "nvidia counter %d not enabled", (int)type);
	    return ret;
	}

	GpuPerfCtrRes getGpuAggPerfCtr(nvidia_counters_t type, int wid) { 
	    GpuPerfCtrRes ret;

	    if (gpuPerfCtr[type].isEnabled())
		return gpuPerfCtr[type].accDataAgg(wid);
	    else
		arm_throw(SensingException, "nvidia counter %d not enabled", (int)type);
	    return ret;
	}

	int enableGpuPerfCnt(nvidia_counters_t type, bool enable) { 
	    return gpuPerfCtr[type].enable(enable);
	}
};

static SensorWrapper *sensors = nullptr;

template<>
void pal_sensing_setup<SensingModule>(SensingModule *m){
	if(sensors != nullptr)
		arm_throw(PALException, "Sensors already created!");

	sem_unlink("/sem-metrics");
	sem_unlink("/sem-results");
	shm_unlink("/shm-mars-hookcuda");

	sensors = new SensorWrapper(m);
}

template<>
void pal_sensing_teardown<SensingModule>(SensingModule *m){
	if(sensors == nullptr)
		arm_throw(PALException, "Sensors not created!");
	delete sensors;
	sensors = nullptr;
}


/*
 * Sensing interface functions implementation
 */

template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
SensingInterfaceImpl::Impl::sense<SEN_POWER_W,power_domain_info_t>(const power_domain_info_t *rsc, int wid)
{
			switch (rsc->domain_id) {
				case 0:
				case 1:
			// We have just 1 domain
					return sensors->cpuPowerW(wid);
				default:
					break;
			}
	arm_throw(SensingException,"Invalid power domain id %d",rsc->domain_id);
	return 0;
}

template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
SensingInterfaceImpl::Impl::senseAgg<SEN_POWER_W,power_domain_info_t>(const power_domain_info_t *rsc, int wid)
{
	switch (rsc->domain_id) {
	case 0:
	case 1:
		return sensors->cpuAggPowerW(wid);
	default:
		break;
	}
	arm_throw(SensingException,"Invalid power domain id %d",rsc->domain_id);
	return 0;
}

template <>
typename SensingTypeInfo<SEN_NV_GPU_PERFCNT>::ValType
SensingInterfaceImpl::Impl::sense<SEN_NV_GPU_PERFCNT, NullResource>(typename SensingTypeInfo<SEN_NV_GPU_PERFCNT>::ParamType p, const NullResource *rsc, int wid)
{
    return sensors->getGpuPerfCtr(p, wid);
}


template<>
typename SensingTypeInfo<SEN_NV_GPU_PERFCNT>::ValType
SensingInterfaceImpl::Impl::senseAgg<SEN_NV_GPU_PERFCNT, NullResource>(typename SensingTypeInfo<SEN_NV_GPU_PERFCNT>::ParamType p, const NullResource *rsc, int wid)
{
    return sensors->getGpuAggPerfCtr(p, wid);
}

template <>
int
SensingInterfaceImpl::Impl::enableSensor<SEN_NV_GPU_PERFCNT, NullResource>(typename SensingTypeInfo<SEN_NV_GPU_PERFCNT>::ParamType p, const NullResource *rsc, bool enable)
{
    return sensors->enableGpuPerfCnt(p, enable);
}

// We need this specialization only to support the implementation of
// BinFuncImpl<power>
// However this fuction should never be actually called
template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
SensingInterfaceImpl::Impl::sense<SEN_POWER_W,tracked_task_data_t>(const tracked_task_data_t *rsc, int wid)
{
    arm_throw(SensingException,"This function should never be called");
    return 0;
}

// We need this specialization only to allow the reflective code to compile
// However this fuction should never be actually called
template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
SensingInterfaceImpl::Impl::sense<SEN_POWER_W,freq_domain_info_t>(const freq_domain_info_t *rsc, int wid)
{
    arm_throw(SensingException,"This function should never be called");
    return 0;
}
template<>
typename SensingTypeInfo<SEN_POWER_W>::ValType
SensingInterfaceImpl::Impl::senseAgg<SEN_POWER_W,freq_domain_info_t>(const freq_domain_info_t *rsc, int wid)
{
    arm_throw(SensingException,"This function should never be called");
    return 0;
}
