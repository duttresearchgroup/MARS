
#ifndef __arm_rt_sensor_h
#define __arm_rt_sensor_h

#include <vector>
#include <pthread.h>
#include <semaphore.h>


#include <core/core.h>
#include <runtime/framework/types.h>
#include <runtime/interfaces/common/sensing_window_defs.h>

class PeriodicSensingManager;

class PeriodicSensor {
	friend class PeriodicSensingManager;

  protected:

	virtual void doSampling() = 0;
	virtual void setSensingWindows(int numWindows) = 0;
	virtual void windowReady(int wid) = 0;
  public:
	virtual ~PeriodicSensor(){}
};

class PeriodicSensingManager {
  private:
	pthread_t		_thread;
	volatile bool	_threadStop;
	bool _isRunning;

	bool _sensingWindowSet;

	std::vector<PeriodicSensor*> _sensors;

	void __thread_func();
	static void* _thread_func(void*arg);

  public:

	PeriodicSensingManager();
	~PeriodicSensingManager();

	void attachSensor(PeriodicSensor* sensor);

	void setSensingWindows(int numWindows);

	void windowReady(int wid);

	void startSensing();
	void stopSensing();

};



template<sensing_type TYPE, typename Derived>
class SensorBase : public PeriodicSensor {

	friend class PeriodicSensing;

  private:
	pthread_mutex_t _windowMutex;

	std::vector<typename sensing_type_val<TYPE>::type> _currWindowAcc;
	std::vector<typename sensing_type_val<TYPE>::type> _currWindowSamples;

	std::vector<typename sensing_type_val<TYPE>::type> _lastWindowAcc;
	std::vector<typename sensing_type_val<TYPE>::type> _lastWindowSamples;

	std::vector<typename sensing_type_val<TYPE>::type> _aggWindowAcc;
	std::vector<typename sensing_type_val<TYPE>::type> _aggWindowSamples;

	std::vector<typename sensing_type_val<TYPE>::type> _lastAggWindowAcc;
	std::vector<typename sensing_type_val<TYPE>::type> _lastAggWindowSamples;

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
		typename sensing_type_val<TYPE>::type sample = static_cast<Derived*>(this)->readSample();
		pthread_mutex_lock(&_windowMutex);
		for(unsigned int i = 0; i < _currWindowAcc.size(); ++i){
			_currWindowAcc[i] += sample;
			_aggWindowAcc[i] += sample;
			_currWindowSamples[i] += 1;
			_aggWindowSamples[i] += 1;
		}
		pthread_mutex_unlock(&_windowMutex);
	}

	SensorBase() :PeriodicSensor()
	{
		if(pthread_mutex_init(&_windowMutex,nullptr))
			arm_throw(SensingException,"pthread_mutex_init failed with errno %d",errno);
	}

  public:

	// Returns the accumulated sample data for a sensing window
	typename sensing_type_val<TYPE>::type accData(int wid) { return _lastWindowAcc[wid]; }
	typename sensing_type_val<TYPE>::type samples(int wid) { return _lastWindowSamples[wid]; }

	typename sensing_type_val<TYPE>::type accDataAgg(int wid) { return _lastAggWindowAcc[wid]; }
	typename sensing_type_val<TYPE>::type samplesAgg(int wid) { return _lastAggWindowSamples[wid]; }

	// The type of information we are sensing
	sensing_type type() const { return TYPE;}

	virtual ~SensorBase(){}
};

#endif



