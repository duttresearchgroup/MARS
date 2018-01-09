#include "../../../common/pal/exynos5422/_common.h"
#include "../../../linux-module/pal/pal.h"

#define DISABLE_POWER_SENSING

#ifndef DISABLE_POWER_SENSING
//Defined at drivers/hardkernel/ina231-i2c.c
unsigned int ina231_i2c_get_power_uW(int sensor_id);
void ina231_i2c_enable_id(int sensor_id);
void ina231_i2c_disable_id(int sensor_id);

static inline void ina231_i2c_enable_sensor(int sensor_id)
{
    ina231_i2c_enable_id(sensor_id);
}
static inline void ina231_i2c_disable_sensor(int sensor_id)
{
    ina231_i2c_disable_id(sensor_id);
}

//if only per-cluster or per-chip sensor is available,
//then the measurement correspond to the entire cluster/chip this core belongs to
void power_sense_start(power_domain_info_t *domain)
{
    ina231_i2c_enable_sensor(domain->domain_id);
}

//return the scaled powert in Watts
uint32_t power_sense_end(power_domain_info_t *domain)
{
    int sensor = domain->domain_id;
    ina231_i2c_disable_sensor(sensor);
    //return ((int32_t)CONV_INTany_scaledINTany((int64_t)ina231_i2c_get_power_uW(sensor))) / 1000000;
    return ina231_i2c_get_power_uW(sensor);
//    return ina231_i2c_get_current_power_uW(sensor);
}
#else

//if only per-cluster or per-chip sensor is available,
//then the measurement correspond to the entire cluster/chip this core belongs to
void power_sense_start(power_domain_info_t *domain)
{
	static bool firstCall = true;
	if(firstCall){
		pinfo("WARNING: power_sense_start - power sensing disabled!\n");
		firstCall = false;
	}
}

//return the scaled powert in Watts
uint32_t power_sense_end(power_domain_info_t *domain)
{
	static bool firstCall = true;
	if(firstCall){
		pinfo("WARNING: power_sense_end - power sensing disabled - returning 1W!\n");
		firstCall = false;
	}
	return 1000000;
}

#endif
