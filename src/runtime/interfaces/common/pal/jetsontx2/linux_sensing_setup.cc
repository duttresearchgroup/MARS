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

//helper to encapsulate sensor creation/destruciton
struct SensorWrapper {
	INA3221 cpuPower;
	INA3221 gpuPower;

	SensorWrapper(SensingModule *m)
		:cpuPower(INA3221::VDD_SYS_CPU),
		 gpuPower(INA3221::VDD_SYS_GPU)
	{
		m->attachSensor(&cpuPower);
		m->attachSensor(&gpuPower);
	}

	double cpuPowerW(int wid) { return cpuPower.accData(wid)/cpuPower.samples(wid);}
	double cpuAggPowerW(int wid) { return cpuPower.accDataAgg(wid)/cpuPower.samplesAgg(wid);}

	// Will not be used currently
	double gpuPowerW(int wid) { return gpuPower.accData(wid)/gpuPower.samples(wid);}
	double gpuAggPowerW(int wid) { return gpuPower.accDataAgg(wid)/gpuPower.samplesAgg(wid);}
};

static SensorWrapper *sensors = nullptr;

template<>
void pal_sensing_setup<SensingModule>(SensingModule *m){
	if(sensors != nullptr)
		arm_throw(PALException, "Sensors already created!");
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
