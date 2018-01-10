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

#include <core/core.h>

#include <runtime/interfaces/common/pal/sensing_setup.h>
#include <runtime/interfaces/sensing_module.h>
#include <runtime/interfaces/linux/sensor.h>
#include <runtime/framework/sensing_interface.h>

class INA231 : public SensorBase<SEN_POWER_W,INA231> {
  public:
	static constexpr const char* SENSOR_BIGC = "/dev/sensor_arm";
	static constexpr const char* SENSOR_LITTLEC = "/dev/sensor_kfc";
	static constexpr const char* SENSOR_MEM = "/dev/sensor_mem";
	static constexpr const char* SENSOR_G3D = "/dev/sensor_g3d";

  private:
	typedef struct ina231_iocreg__t {
		char name[20];
		unsigned int enable;
		unsigned int cur_uV;
		unsigned int cur_uA;
		unsigned int cur_uW;
	} ina231_iocreg_t;

	typedef struct sensor__t {
	    int  fd;
	    ina231_iocreg_t data;
	} sensor_t;

	#define INA231_IOCGREG      _IOR('i', 1, ina231_iocreg_t *)
	#define INA231_IOCSSTATUS   _IOW('i', 2, ina231_iocreg_t *)
	#define INA231_IOCGSTATUS   _IOR('i', 3, ina231_iocreg_t *)

	const char* dev_path;
	sensor_t sensor;

  public:

	INA231(const char *dev_path) :dev_path(dev_path)
	{
		open_sensor(dev_path,&sensor);
		read_sensor_status(&sensor);
		enable_sensor(&sensor,1);
	}

	~INA231()
	{
		enable_sensor(&sensor,0);
		close_sensor(&sensor);
	}

	typename sensing_type_val<SEN_POWER_W>::type readSample()
	{
		read_sensor(&sensor);
		return (double)(sensor.data.cur_uW / 1000) / 1000;
	}

  private:
	int open_sensor(const char *node, sensor_t *sensor)
	{
		if ((sensor->fd = open(node, O_RDWR)) < 0)
			arm_throw(INA231Exception, "Cannot open %s",node);
		return sensor->fd;
	}

	void enable_sensor(sensor_t *sensor, unsigned char enable)
	{
	    if (sensor->fd > 0) {
	        sensor->data.enable = enable ? 1 : 0;
	        if (ioctl(sensor->fd, INA231_IOCSSTATUS, &sensor->data) < 0)
	            arm_throw(INA231Exception,"IOCTL error for sen %s, errno %d",dev_path,errno);

	    }
	}

	int read_sensor_status(sensor_t *sensor)
	{
	    if (sensor->fd > 0) {
	        if (ioctl(sensor->fd, INA231_IOCGSTATUS, &sensor->data) < 0)
				arm_throw(INA231Exception,"IOCTL error for sen %s, errno %d",sensor->data.name,errno);
	    }
	    return 0;
	}

	void read_sensor(sensor_t *sensor)
	{
	    if (sensor->fd > 0) {
	        if (ioctl(sensor->fd, INA231_IOCGREG, &sensor->data) < 0)
	        	arm_throw(INA231Exception,"IOCTL error for sen %s, errno %d",sensor->data.name,errno);
	    }
	}

	void close_sensor(sensor_t *sensor)
	{
	    if (sensor->fd > 0)
	        close(sensor->fd);
	}
};

//helper to encapsulate sensor creation/destruciton
struct SensorWrapper {
	INA231 littleClusterPower;
	INA231 bigClusterPower;

	SensorWrapper(SensingModule *m)
		:littleClusterPower(INA231::SENSOR_LITTLEC),
		 bigClusterPower(INA231::SENSOR_BIGC)
	{
		m->attachSensor(&littleClusterPower);
		m->attachSensor(&bigClusterPower);
	}

	double bigPowerW(int wid) { return bigClusterPower.accData(wid)/bigClusterPower.samples(wid);}
	double lilPowerW(int wid) { return littleClusterPower.accData(wid)/littleClusterPower.samples(wid);}

	double bigAggPowerW(int wid) { return bigClusterPower.accDataAgg(wid)/bigClusterPower.samplesAgg(wid);}
	double lilAggPowerW(int wid) { return littleClusterPower.accDataAgg(wid)/littleClusterPower.samplesAgg(wid);}
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
typename sensing_type_val<SEN_POWER_W>::type
SensingInterface::sense<SEN_POWER_W,power_domain_info_t>(const power_domain_info_t *rsc, int wid)
{
	switch (rsc->domain_id) {
		case 0:
			return sensors->bigPowerW(wid);
		case 1:
			return sensors->lilPowerW(wid);
		default:
			break;
	}
	arm_throw(SensingException,"Invalid power domain id %d",rsc->domain_id);
	return 0;
}

template<>
typename sensing_type_val<SEN_POWER_W>::type
SensingInterface::senseAgg<SEN_POWER_W,power_domain_info_t>(const power_domain_info_t *rsc, int wid)
{
	switch (rsc->domain_id) {
		case 0:
			return sensors->bigAggPowerW(wid);
		case 1:
			return sensors->lilAggPowerW(wid);
		default:
			break;
	}
	arm_throw(SensingException,"Invalid power domain id %d",rsc->domain_id);
	return 0;
}

