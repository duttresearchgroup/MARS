
#include <unistd.h>

#include "sensor.h"

PeriodicSensingManager::PeriodicSensingManager()
	:_thread(0), _threadStop(false), _isRunning(false), _sensingWindowSet(false)
{

}

PeriodicSensingManager::~PeriodicSensingManager()
{
	if(_isRunning)
		stopSensing();
}

void PeriodicSensingManager::__thread_func()
{

	pinfo("PeriodicSensing thread started\n");

	while(!_threadStop){
		usleep(MINIMUM_WINDOW_LENGHT_MS*1000);

		for(auto sensor : _sensors)
			sensor->doSampling();
	}
	pinfo("PeriodicSensing thread ended\n");
}

void* PeriodicSensingManager::_thread_func(void*arg)
{
	try {
		PeriodicSensingManager *s = reinterpret_cast<PeriodicSensingManager*>(arg);
		s->__thread_func();
	} arm_catch(ARM_CATCH_NO_EXIT);
	pthread_exit(nullptr);
	return nullptr;
}

void PeriodicSensingManager::attachSensor(PeriodicSensor* sensor)
{
	if(_sensingWindowSet)
		arm_throw(SensingException,"Cannot attach new sensors after sensing window is set");
	_sensors.push_back(sensor);
}

void PeriodicSensingManager::setSensingWindows(int numWindows)
{
	if(_sensors.size() == 0)
		pinfo("WARNING: no sensors attached \"%s\"\n",__PRETTY_FUNCTION__);
	for(auto s : _sensors)
		s->setSensingWindows(numWindows);
	_sensingWindowSet = true;
}

void PeriodicSensingManager::windowReady(int wid)
{
	for(auto s : _sensors)
		s->windowReady(wid);
}

void PeriodicSensingManager::startSensing()
{
	if(_isRunning)
		arm_throw(SensingException,"Sensing already running");

	int rc = pthread_create(&_thread, NULL, _thread_func, this);
	if (rc)
		arm_throw(SensingException,"Error code %d returned from pthread_create()",rc);

	_isRunning = true;
}

void PeriodicSensingManager::stopSensing()
{
	if(!_isRunning)
		arm_throw(SensingException,"Sensing was not running");

	_threadStop = true;
	pthread_join(_thread,nullptr);

	_isRunning = false;
}




