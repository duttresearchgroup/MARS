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

#ifndef __arm_rt_sensor_h
#define __arm_rt_sensor_h

#include <vector>
#include <pthread.h>
#include <semaphore.h>

#include <base/base.h>
#include <runtime/framework/types.h>
#include <runtime/interfaces/common/sensing_window_defs.h>

template<typename SensingModule> class PeriodicSensingManager;

template<typename SensingModule>
class PeriodicSensor {
	friend class PeriodicSensingManager<SensingModule>;

  protected:

	virtual void doSampling() = 0;
	virtual void setSensingWindows(int numWindows) = 0;
	virtual void windowReady(int wid) = 0;
  public:
	virtual ~PeriodicSensor(){}
};

template<typename SensingModule>
class PeriodicSensingManager {
  private:
    SensingModule &_sensing_module;
	pthread_t		_thread;
	volatile bool	_threadStop;
	bool _isRunning;

	bool _sensingWindowSet;

	std::vector<PeriodicSensor<SensingModule>*> _sensors;

	void __thread_func()
	{
	    pinfo("PeriodicSensing thread started\n");

	    while(!_threadStop){
	        _sensing_module.sleepMS(MINIMUM_WINDOW_LENGTH_MS);


	        for(auto sensor : _sensors)
	            sensor->doSampling();
	    }
	    pinfo("PeriodicSensing thread ended\n");
	}
	static void* _thread_func(void*arg)
	{
	    try {
	        PeriodicSensingManager *s = reinterpret_cast<PeriodicSensingManager*>(arg);
	        s->__thread_func();
	    } arm_catch(ARM_CATCH_NO_EXIT);
	    pthread_exit(nullptr);
	    return nullptr;
	}

  public:

	PeriodicSensingManager(SensingModule &module)
      :_sensing_module(module),
       _thread(0), _threadStop(false), _isRunning(false), _sensingWindowSet(false)
    { }

	~PeriodicSensingManager()
	{
	    if(_isRunning)
	        stopSensing();
	}

	void attachSensor(PeriodicSensor<SensingModule>* sensor)
	{
	    if(_sensingWindowSet)
	        arm_throw(SensingException,"Cannot attach new sensors after sensing window is set");
	    _sensors.push_back(sensor);
	}

	void setSensingWindows(int numWindows)
	{
	    if(_sensors.size() == 0)
	        pinfo("WARNING: no sensors attached \"%s\"\n",__PRETTY_FUNCTION__);
	    for(auto s : _sensors)
	        s->setSensingWindows(numWindows);
	    _sensingWindowSet = true;
	}

	void windowReady(int wid)
	{
	    for(auto s : _sensors)
	        s->windowReady(wid);
	}

	void startSensing()
	{
	    if(_isRunning)
	        arm_throw(SensingException,"Sensing already running");

	    int rc = pthread_create(&_thread, NULL, _thread_func, this);
	    if (rc)
	        arm_throw(SensingException,"Error code %d returned from pthread_create()",rc);

	    _isRunning = true;
	}

	void stopSensing()
	{
	    if(!_isRunning)
	        arm_throw(SensingException,"Sensing was not running");

	    _threadStop = true;
	    pthread_join(_thread,nullptr);

	    _isRunning = false;
	}

};



template<SensingType TYPE, typename Derived, typename SensingModule>
class SensorBase : public PeriodicSensor<SensingModule> {

	friend class PeriodicSensing;

  private:
	pthread_mutex_t _windowMutex;

	std::vector<typename SensingTypeInfo<TYPE>::ValType> _currWindowAcc;
	std::vector<typename SensingTypeInfo<TYPE>::ValType> _currWindowSamples;

	std::vector<typename SensingTypeInfo<TYPE>::ValType> _lastWindowAcc;
	std::vector<typename SensingTypeInfo<TYPE>::ValType> _lastWindowSamples;

	std::vector<typename SensingTypeInfo<TYPE>::ValType> _aggWindowAcc;
	std::vector<typename SensingTypeInfo<TYPE>::ValType> _aggWindowSamples;

	std::vector<typename SensingTypeInfo<TYPE>::ValType> _lastAggWindowAcc;
	std::vector<typename SensingTypeInfo<TYPE>::ValType> _lastAggWindowSamples;

	void _windowsOK() const{
		assert_true(_currWindowAcc.size()>0);
		assert_true(_currWindowAcc.size()==_currWindowSamples.size());
		assert_true(_lastWindowAcc.size()==_lastWindowAcc.size());
	}

  protected:

	void setSensingWindows(int numWindows) override
	{
		for(int i = 0; i < numWindows; ++i){
			_currWindowAcc.push_back(0);
			_currWindowSamples.push_back(0);
			_lastWindowAcc.push_back(0);
			_lastWindowSamples.push_back(0);
			_aggWindowAcc.push_back(0);
			_aggWindowSamples.push_back(0);
			_lastAggWindowAcc.push_back(0);
			_lastAggWindowSamples.push_back(0);
		}
	}

	void windowReady(int wid) override
	{
		assert_true((unsigned)wid < _lastWindowAcc.size());
		assert_true((unsigned)wid < _currWindowAcc.size());
		pthread_mutex_lock(&_windowMutex);
		_lastWindowAcc[wid] = _currWindowAcc[wid];
		_lastWindowSamples[wid] = _currWindowSamples[wid];
		_lastAggWindowAcc[wid] = _aggWindowAcc[wid];
		_lastAggWindowSamples[wid] = _aggWindowSamples[wid];
		_currWindowAcc[wid] = 0;
		_currWindowSamples[wid] = 0;
		pthread_mutex_unlock(&_windowMutex);
	}

	void doSampling() override
	{
		_windowsOK();
		typename SensingTypeInfo<TYPE>::ValType sample = static_cast<Derived*>(this)->readSample();
		pthread_mutex_lock(&_windowMutex);
		for(unsigned int i = 0; i < _currWindowAcc.size(); ++i){
			_currWindowAcc[i] += sample;
			_aggWindowAcc[i] += sample;
			_currWindowSamples[i] += 1;
			_aggWindowSamples[i] += 1;
		}
		pthread_mutex_unlock(&_windowMutex);
	}

	SensorBase() :PeriodicSensor<SensingModule>()
	{
		if(pthread_mutex_init(&_windowMutex,nullptr))
			arm_throw(SensingException,"pthread_mutex_init failed with errno %d",errno);
	}

  public:

	// Returns the accumulated sample data for a sensing window
	typename SensingTypeInfo<TYPE>::ValType accData(int wid) { return _lastWindowAcc[wid]; }
	typename SensingTypeInfo<TYPE>::ValType samples(int wid) { return _lastWindowSamples[wid]; }

	typename SensingTypeInfo<TYPE>::ValType accDataAgg(int wid) { return _lastAggWindowAcc[wid]; }
	typename SensingTypeInfo<TYPE>::ValType samplesAgg(int wid) { return _lastAggWindowSamples[wid]; }

	// The type of information we are sensing
	SensingType type() const { return TYPE;}

	virtual ~SensorBase(){}
};

#endif



