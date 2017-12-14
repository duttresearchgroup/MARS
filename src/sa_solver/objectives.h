#ifndef __objectives_h
#define __objectives_h

#include "schedule.h"

namespace SASolverImpl {

namespace Objectives {

template<typename Objective>
class CFS_Estimator{
	double equal_share;
	double residual_share;
	double demand_sum;
	double demand_sum_residual;
	int task_cnt;
	const int cpu;
	const Schedule<Objective> &schedule;
	const Global::Input_Data &input_data;

public:
	CFS_Estimator(const int _cpu,const Schedule<Objective> &_schedule)
		:equal_share(0), residual_share(0), demand_sum(0), demand_sum_residual(0),
		 task_cnt(0), cpu(_cpu), schedule(_schedule),
		 input_data(Global::input_data())
	{
		for (int j = 0; j < Global::Input_Params::N_TASKS; ++j) {
			int task = schedule(cpu,j);
			if(task != 0){
				demand_sum += input_data.task_per_cpu_data[task-1][cpu].demand;
				++task_cnt;
			}
		}
		vassert(demand_sum >= 0);

		if(demand_sum >= 1){
			equal_share = 1.0/(double)task_cnt;
			vassert(equal_share >= 0);
			for (int j = 0; j < Global::Input_Params::N_TASKS; ++j) {
				int task = schedule(cpu,j);
				if(task != 0){
					if(input_data.task_per_cpu_data[task-1][cpu].demand <= equal_share){
						residual_share += equal_share - input_data.task_per_cpu_data[task-1][cpu].demand;
					}
					else{
						demand_sum_residual += input_data.task_per_cpu_data[task-1][cpu].demand;
					}
				}
			}
		}
	}

	double estimate_contribution(int task){
		double task_contrib;
		if((demand_sum < 1) || (input_data.task_per_cpu_data[task-1][cpu].demand <= equal_share)){
			task_contrib = input_data.task_per_cpu_data[task-1][cpu].demand;
		}
		else{
			task_contrib = equal_share + ((input_data.task_per_cpu_data[task-1][cpu].demand/demand_sum_residual)*residual_share);
		}
		vassert(task_contrib <= 1);
		vassert(task_contrib >= 0);
		//std::cout << "contrib(task="<<task<<")="<<task_contrib<<"\n";
		return task_contrib;
	}

	double total_load(){
		//std::cout << "total_load(cpu="<<cpu<<")="<<(demand_sum > 1 ? 1 : demand_sum)<<"\n";
		return demand_sum > 1 ? 1 : demand_sum;
	}

	int num_tasks(){
		return task_cnt;
	}

};

class Max_IPC_per_Watt{

private:
	double _objective;
	double _objective_ips;
	double _objective_power;
	double *_cpus_ips;//[Global::Input_Params::MAX_N_CPUS];
	double *_cpus_power;//[Global::Input_Params::MAX_N_CPUS];
	const Global::Input_Data &input_data;
	Global::Output_Data &output_data;

private:

	void calc_cpu_ipc_and_power(const int cpu, const Schedule<Max_IPC_per_Watt> &sched){

		_cpus_ips[cpu] = 0;
		_cpus_power[cpu] = 0;

		CFS_Estimator<Max_IPC_per_Watt> cfs(cpu,sched);

		double share_sum = 0;

		//calc ips
		for (int j = 0; j < Global::Input_Params::N_TASKS; ++j) {
			int task = sched(cpu,j);
			if(task != 0) {
				double task_share = cfs.estimate_contribution(task);
				share_sum+=task_share;
				output_data.task_pred_load[task-1] = task_share;
				_cpus_ips[cpu] += task_share * input_data.task_per_cpu_data[task-1][cpu].active_ips;
				_cpus_power[cpu] += task_share * input_data.task_per_cpu_data[task-1][cpu].active_power;
			}
		}
		if(cfs.num_tasks() > 0){
		    vassert(_cpus_ips[cpu] >= 0);
		    vassert(_cpus_power[cpu] >= 0);//so far accounts for only dyn power

		    double total_load = cfs.total_load();
		    _cpus_power[cpu] += input_data.cpu_static_data[cpu].idle_power * (1-total_load);//time idle

		    vassertSTR(total_load <= 1);
		    vassertSTR(total_load < (share_sum+0.01));
		    vassertSTR(total_load > (share_sum-0.01));
		}
		else {
		    //assumes cpu freq/voltage scales down when no tasks are mapped to the core
		    //so idle_power != idle_power_when_empty
		    _cpus_power[cpu] += input_data.cpu_static_data[cpu].idle_power_when_empty * (1-input_data.cpu_static_data[cpu].kernel_idle_load);
			_cpus_power[cpu] += input_data.cpu_static_data[cpu].kernel_active_power * input_data.cpu_static_data[cpu].kernel_idle_load;
		}

		//always non zero static power
		vassert(_cpus_power[cpu] > 0);
	}

	void update_objective(){
		_objective_ips = 0;
		_objective_power = 0;
		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			_objective_ips += _cpus_ips[i];
			_objective_power += _cpus_power[i];
		}
		vassert(_objective_power > 0);//static power is never 0
		_objective = _objective_ips/_objective_power;
	}

public:
	Max_IPC_per_Watt()
		:_objective(0), _objective_ips(0), _objective_power(0), _cpus_ips(0), _cpus_power(0),
		 input_data(Global::input_data()),
		 output_data(Global::output_data()){

		_cpus_ips = Global::allocate_array<double>(Global::Input_Params::MAX_N_CPUS);
		_cpus_power = Global::allocate_array<double>(Global::Input_Params::MAX_N_CPUS);
	};

	Max_IPC_per_Watt(const Max_IPC_per_Watt &other)
		:_objective(other._objective),
		 _objective_ips(other._objective_ips),
		 _objective_power(other._objective_power),
		 _cpus_ips(0), _cpus_power(0),
		 input_data(Global::input_data()),
		 output_data(Global::output_data()){

		_cpus_ips = Global::allocate_array<double>(Global::Input_Params::MAX_N_CPUS);
		_cpus_power = Global::allocate_array<double>(Global::Input_Params::MAX_N_CPUS);
		vassert(is_ok());

		for (int cpu = 0; cpu < Global::Input_Params::MAX_N_CPUS; ++cpu) _cpus_ips[cpu] = other._cpus_ips[cpu];
		for (int cpu = 0; cpu < Global::Input_Params::MAX_N_CPUS; ++cpu) _cpus_power[cpu] = other._cpus_power[cpu];
	};

	Max_IPC_per_Watt&  operator=( const Max_IPC_per_Watt& other ) {
		_objective = other._objective;
		_objective_ips = other._objective_ips;
		_objective_power = other._objective_power;

		//the matrices should have been allocates and different from other
		vassert(is_ok());

		vassert(_cpus_ips!=other._cpus_ips);
		vassert(_cpus_power!=other._cpus_power);

		for (int cpu = 0; cpu < Global::Input_Params::MAX_N_CPUS; ++cpu) _cpus_ips[cpu] = other._cpus_ips[cpu];
		for (int cpu = 0; cpu < Global::Input_Params::MAX_N_CPUS; ++cpu) _cpus_power[cpu] = other._cpus_power[cpu];

		return *this;
	}

	~Max_IPC_per_Watt(){
		if(_cpus_ips != 0) Global::deallocate(_cpus_ips);
		if(_cpus_power != 0) Global::deallocate(_cpus_power);
	}

	bool is_ok(){
		return (_cpus_ips!=0) && (_cpus_power!=0);
	}

	void update_new(const Schedule<Max_IPC_per_Watt> &sched){
		_objective = 0; _objective_ips= 0; _objective_power = 0;
		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			calc_cpu_ipc_and_power(i, sched);
		}

		update_objective();
	}

	//must be called AFTER this switch:
	//sched[pos1] = task2 //prev was sched[pos1] = task1
	//sched[pos2] = task1 //prev was sched[pos2] = task2
	void update_on_switch(int task1, int cpu1, int task2, int cpu2,
			const Schedule<Max_IPC_per_Watt> &sched){

		vassert((cpu1 >= 0)&&(cpu1 < Global::Input_Params::N_CPUS));
		vassert((cpu2 >= 0)&&(cpu2 < Global::Input_Params::N_CPUS));

		if(cpu1 == cpu2) return;

		if((task1 != 0) || (task2 != 0)){
			calc_cpu_ipc_and_power(cpu1, sched);
			calc_cpu_ipc_and_power(cpu2, sched);
		}

		update_objective();
	}

#ifndef __KERNEL__
	std::string to_string() const
	{
		std::stringstream output;
		output << "max_ipc_watt=" << _objective << "(max_ipc= " << _objective_ips << " max_power= " << _objective_power <<")";
		return output.str();
	}

	friend std::ostream& operator<<( std::ostream &output,
			const Max_IPC_per_Watt &s)
	{
		output << s.to_string();
		return output;
	}

	static std::string class_name()
	{
		std::stringstream output;
		output << "MAX_IPC_per_Watt";
		return output.str();
	}
#endif

public:
	const double& value() const{
		return _objective;
	}

	static const int SECUNDARY_OBJ_CNT = 2;
	const double& value(int sec) const{
		switch (sec) {
			case 0: return _objective_ips;
			case 1: return _objective_power;
			default: vassert(false);
		}
		return _objective_ips;
	}
};

/*

class Max_IPC{

private:

	void calc_cpu_ipc_and_power(const int cpu, const Schedule<Max_IPC> &sched){
		_cpus_ips[cpu] = 0;

		CFS_Estimator<Max_IPC> cfs(cpu,sched);

		//calc ips
		for (int j = 0; j < Global::Input_Params::N_TASKS; ++j) {
			int task = sched(cpu,j);
			if(task != 0) {
				double task_share = cfs.estimate_contribution(task);
				_cpus_ips[cpu] += task_share * input_data.task_per_cpu_data[task-1][cpu].active_ips;
			}
		}
		vassert(_cpus_ips[cpu] >= 0);
	}

	void update_objective(){
		_objective_ips = 0;
		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			_objective_ips += _cpus_ips[i];
		}
	}

public:
	explicit Max_IPC(const double &tgt) :_objective_ips(0) {};

	//couputes based on schedule
	Max_IPC(const Schedule<Max_IPC> &sched) :_objective_ips(0){

		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			calc_cpu_ipc_and_power(i, sched);
		}

		update_objective();
	};

	//must be called AFTER this switch:
	//sched[pos1] = task2 //prev was sched[pos1] = task1
	//sched[pos2] = task1 //prev was sched[pos2] = task2
	void update_on_switch(int task1, int cpu1, int task2, int cpu2,
			const Schedule<Max_IPC> &sched){

		vassert((cpu1 >= 0)&&(cpu1 < Global::Input_Params::N_CPUS));
		vassert((cpu2 >= 0)&&(cpu2 < Global::Input_Params::N_CPUS));

		if(cpu1 == cpu2) return;

		if((task1 != 0) || (task2 != 0)){
			calc_cpu_ipc_and_power(cpu1, sched);
			calc_cpu_ipc_and_power(cpu2, sched);
		}
		update_objective();
	}

#ifndef __KERNEL__
	std::string to_string() const
	{
		std::stringstream output;
		output << "max_ips=" << _objective_ips << "(max_ips= " << _objective_ips << ")";
		return output.str();
	}

	friend std::ostream& operator<<( std::ostream &output,
			const Max_IPC &s)
	{
		output << s.to_string();
		return output;
	}

	static std::string class_name()
	{
		std::stringstream output;
		output << "MAX_IPC";
		return output.str();
	}
#endif

public:
	const double& value() const{
		return _objective_ips;
	}


private:
	double _objective_ips;
	double _cpus_ips[Global::Input_Params::MAX_N_CPUS];
};



class Min_Power{

private:

	void calc_cpu_ipc_and_power(const int cpu, const Schedule<Min_Power> &sched){

		_cpus_power[cpu] = 0;

		CFS_Estimator<Min_Power> cfs(cpu,sched);

		//calc ips
		for (int j = 0; j < Global::Input_Params::N_TASKS; ++j) {
			int task = sched(cpu,j);
			if(task != 0) {
				double task_share = cfs.estimate_contribution(task);
				_cpus_power[cpu] += task_share * input_data.task_per_cpu_data[task-1][cpu].dyn_power;
			}
		}
		vassert(_cpus_power[cpu] >= 0);//so far accounts for only dyn power

		double total_load = cfs.total_load();
		_cpus_power[cpu] += input_data.cpu_static_data[cpu].static_power_active * total_load;//time active
		_cpus_power[cpu] += input_data.cpu_static_data[cpu].static_power_idle * (1-total_load);//time idle

		//always non zero static power
		vassert(_cpus_power[cpu] > 0);
	}

	void update_objective(){
		_objective_power = 0;
		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			_objective_power += _cpus_power[i];
		}
		vassert(_objective_power > 0);//static power is never 0
	}

public:
	explicit Min_Power(const double &tgt) :_objective_power(0) {};

	//couputes based on schedule
	Min_Power(const Schedule<Min_Power> &sched) :_objective_power(0) {

		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			calc_cpu_ipc_and_power(i, sched);
		}

		update_objective();
	};

	//must be called AFTER this switch:
	//sched[pos1] = task2 //prev was sched[pos1] = task1
	//sched[pos2] = task1 //prev was sched[pos2] = task2
	void update_on_switch(int task1, int cpu1, int task2, int cpu2,
			const Schedule<Min_Power> &sched){

		vassert((cpu1 >= 0)&&(cpu1 < Global::Input_Params::N_CPUS));
		vassert((cpu2 >= 0)&&(cpu2 < Global::Input_Params::N_CPUS));

		if(cpu1 == cpu2) return;

		if((task1 != 0) || (task2 != 0)){
			calc_cpu_ipc_and_power(cpu1, sched);
			calc_cpu_ipc_and_power(cpu2, sched);
		}
		update_objective();
	}

#ifndef __KERNEL__
	std::string to_string() const
	{
		std::stringstream output;
		output << "min_watt=" << _objective_power << "(power= " << _objective_power <<")";
		return output.str();
	}

	friend std::ostream& operator<<( std::ostream &output,
			const Min_Power &s)
	{
		output << s.to_string();
		return output;
	}

	static std::string class_name()
	{
		std::stringstream output;
		output << "Min_Power";
		return output.str();
	}
#endif

public:
	const double& value() const{
		return _objective_power;
	}


private:
	double _objective_power;
	double _cpus_power[Global::Input_Params::MAX_N_CPUS];
};

class Max_Perf_given_Power{

private:

	void calc_cpu_ipc_and_power(const int cpu, const Schedule<Max_Perf_given_Power> &sched){
		_cpus_ips[cpu] = 0;
		_cpus_power[cpu] = 0;

		CFS_Estimator<Max_Perf_given_Power> cfs(cpu,sched);

		//calc ips
		for (int j = 0; j < Global::Input_Params::N_TASKS; ++j) {
			int task = sched(cpu,j);
			if(task != 0) {
				double task_share = cfs.estimate_contribution(task);
				_cpus_ips[cpu] += task_share * input_data.task_per_cpu_data[task-1][cpu].active_ips;
				_cpus_power[cpu] += task_share * input_data.task_per_cpu_data[task-1][cpu].dyn_power;
			}
		}
		vassert(_cpus_ips[cpu] >= 0);
		vassert(_cpus_power[cpu] >= 0);//so far accounts for only dyn power

		double total_load = cfs.total_load();
		_cpus_power[cpu] += input_data.cpu_static_data[cpu].static_power_active * total_load;//time active
		_cpus_power[cpu] += input_data.cpu_static_data[cpu].static_power_idle * (1-total_load);//time idle

		//always non zero static power
		vassert(_cpus_power[cpu] > 0);
	}

	void update_objective(){
		_objective_ips = 0;
		_objective_power = 0;
		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			_objective_ips += _cpus_ips[i];
			_objective_power += _cpus_power[i];
		}
		vassert(_objective_power > 0);//static power is never 0

		if(_objective_power > Constraints::total_power){
			double dist = _objective_power/Constraints::total_power;
			double dist_4 = dist*dist*dist*dist;
			_objective = _objective_ips/(dist_4*dist_4*dist_4);
		}
		else{
			_objective = _objective_ips;
		}
	}

public:
	explicit Max_Perf_given_Power(const double &tgt) :_objective(tgt), _objective_ips(0), _objective_power(0) {};

	//couputes based on schedule
	Max_Perf_given_Power(const Schedule<Max_Perf_given_Power> &sched) :_objective(0), _objective_ips(0), _objective_power(0) {

		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			calc_cpu_ipc_and_power(i, sched);
		}

		update_objective();
	};

	//must be called AFTER this switch:
	//sched[pos1] = task2 //prev was sched[pos1] = task1
	//sched[pos2] = task1 //prev was sched[pos2] = task2
	void update_on_switch(int task1, int cpu1, int task2, int cpu2,
			const Schedule<Max_Perf_given_Power> &sched){

		vassert((cpu1 >= 0)&&(cpu1 < Global::Input_Params::N_CPUS));
		vassert((cpu2 >= 0)&&(cpu2 < Global::Input_Params::N_CPUS));

		if(cpu1 == cpu2) return;

		if((task1 != 0) || (task2 != 0)){
			calc_cpu_ipc_and_power(cpu1, sched);
			calc_cpu_ipc_and_power(cpu2, sched);
		}

		update_objective();
	}

#ifndef __KERNEL__
	std::string to_string() const
	{
		std::stringstream output;
		output << "max_ipc_watt=" << _objective << "(max_ipc= " << _objective_ips << " max_power= " << _objective_power <<")";
		return output.str();
	}

	friend std::ostream& operator<<( std::ostream &output,
			const Max_Perf_given_Power &s)
	{
		output << s.to_string();
		return output;
	}

	static std::string class_name()
	{
		std::stringstream output;
		output << "MAX_IPC_per_Watt";
		return output.str();
	}
#endif

public:
	const double& value() const{
		return _objective;
	}


private:
	double _objective;
	double _objective_ips;
	double _objective_power;
	double _cpus_ips[Global::Input_Params::MAX_N_CPUS];
	double _cpus_power[Global::Input_Params::MAX_N_CPUS];
};



class Max_IPC_per_Watt_AVG_ONLY{

private:

	void calc_cpu_ipc_and_power(const int cpu, const Schedule<Max_IPC_per_Watt_AVG_ONLY> &sched){

		_cpus_ips[cpu] = 0;
		_cpus_power[cpu] = 0;

		int tasknct = 0;

		//calc ips
		for (int j = 0; j < Global::Input_Params::N_TASKS; ++j) {
			int task = sched(cpu,j);
			if(task != 0) {
				_cpus_ips[cpu] +=  input_data.task_per_cpu_data[task-1][cpu].active_ips*input_data.task_per_cpu_data[task-1][cpu].demand;
				_cpus_power[cpu] +=
					(input_data.task_per_cpu_data[task-1][cpu].dyn_power+input_data.cpu_static_data[cpu].static_power_active)
					*input_data.task_per_cpu_data[task-1][cpu].demand;
				tasknct += 1;
			}
		}

		if(tasknct>0){
			_cpus_ips[cpu] /= tasknct;
			_cpus_power[cpu] /= tasknct;
		}
		else{
			_cpus_ips[cpu] = 0;
			_cpus_power[cpu] = input_data.cpu_static_data[cpu].static_power_idle;
		}

		vassertX(_cpus_ips[cpu] >= 0, tasknct);
		vassertX(_cpus_power[cpu] >= 0, _cpus_power[cpu]);//so far accounts for only dyn power
	}

	void update_objective(){
		_objective_ips = 0;
		_objective_power = 0;
		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			_objective_ips += _cpus_ips[i];
			_objective_power += _cpus_power[i];
		}
		vassert(_objective_power > 0);//static power is never 0
		_objective = _objective_ips/_objective_power;
	}

public:
	explicit Max_IPC_per_Watt_AVG_ONLY(const double &tgt) :_objective(tgt), _objective_ips(0), _objective_power(0) {};

	//couputes based on schedule
	Max_IPC_per_Watt_AVG_ONLY(const Schedule<Max_IPC_per_Watt_AVG_ONLY> &sched) :_objective(0), _objective_ips(0), _objective_power(0) {

		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			calc_cpu_ipc_and_power(i, sched);
		}

		update_objective();
	};

	//must be called AFTER this switch:
	//sched[pos1] = task2 //prev was sched[pos1] = task1
	//sched[pos2] = task1 //prev was sched[pos2] = task2
	void update_on_switch(int task1, int cpu1, int task2, int cpu2,
			const Schedule<Max_IPC_per_Watt_AVG_ONLY> &sched){

		vassert((cpu1 >= 0)&&(cpu1 < Global::Input_Params::N_CPUS));
		vassert((cpu2 >= 0)&&(cpu2 < Global::Input_Params::N_CPUS));

		if(cpu1 == cpu2) return;

		if((task1 != 0) || (task2 != 0)){
			calc_cpu_ipc_and_power(cpu1, sched);
			calc_cpu_ipc_and_power(cpu2, sched);
		}

		update_objective();
	}

#ifndef __KERNEL__
	std::string to_string() const
	{
		std::stringstream output;
		output << "max_ipc_watt=" << _objective << "(max_ipc= " << _objective_ips << " max_power= " << _objective_power <<")";
		return output.str();
	}

	friend std::ostream& operator<<( std::ostream &output,
			const Max_IPC_per_Watt_AVG_ONLY &s)
	{
		output << s.to_string();
		return output;
	}

	static std::string class_name()
	{
		std::stringstream output;
		output << "MAX_IPC_per_Watt";
		return output.str();
	}
#endif

public:
	const double& value() const{
		return _objective;
	}

	static const int SECUNDARY_OBJ_CNT = 2;
	const double& value(int sec) const{
		switch (sec) {
			case 0: return _objective_ips;
			case 1: return _objective_power;
			default: vassert(false);
		}
		return _objective_ips;
	}


private:
	double _objective;
	double _objective_ips;
	double _objective_power;
	double _cpus_ips[Global::Input_Params::MAX_N_CPUS];
	double _cpus_power[Global::Input_Params::MAX_N_CPUS];
};

*/

class Max_IPC_per_Watt_AVG_ONLY{

private:
	double _objective;
	double _objective_ips;
	double _objective_power;
	double *_cpus_ips;//[Global::Input_Params::MAX_N_CPUS];
	double *_cpus_power;//[Global::Input_Params::MAX_N_CPUS];
	const Global::Input_Data &input_data;

private:

	void calc_cpu_ipc_and_power(const int cpu, const Schedule<Max_IPC_per_Watt_AVG_ONLY> &sched){

		_cpus_ips[cpu] = 0;
		_cpus_power[cpu] = 0;

		int tasknct = 0;

		//calc ips
		for (int j = 0; j < Global::Input_Params::N_TASKS; ++j) {
			int task = sched(cpu,j);
			if(task != 0) {
				_cpus_ips[cpu] +=  input_data.task_per_cpu_data[task-1][cpu].active_ips*input_data.task_per_cpu_data[task-1][cpu].demand;
				_cpus_power[cpu] += input_data.cpu_static_data[cpu].idle_power * (1-input_data.task_per_cpu_data[task-1][cpu].demand);
				_cpus_power[cpu] += input_data.task_per_cpu_data[task-1][cpu].active_power * input_data.task_per_cpu_data[task-1][cpu].demand;
				tasknct += 1;
			}
		}

		if(tasknct>0){
			_cpus_ips[cpu] /= tasknct;
			_cpus_power[cpu] /= tasknct;
		}
		else{
			_cpus_ips[cpu] = 0;
			_cpus_power[cpu] = input_data.cpu_static_data[cpu].idle_power;
		}

		vassert(_cpus_ips[cpu] >= 0);
		vassert(_cpus_power[cpu] > 0);
	}

	void update_objective(){
		_objective_ips = 0;
		_objective_power = 0;
		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			_objective_ips += _cpus_ips[i];
			_objective_power += _cpus_power[i];
		}
		vassert(_objective_power > 0);//static power is never 0
		_objective = _objective_ips/_objective_power;
	}

public:
	Max_IPC_per_Watt_AVG_ONLY()
		:_objective(0), _objective_ips(0), _objective_power(0), _cpus_ips(0), _cpus_power(0), input_data(Global::input_data()){

		_cpus_ips = Global::allocate_array<double>(Global::Input_Params::MAX_N_CPUS);
		_cpus_power = Global::allocate_array<double>(Global::Input_Params::MAX_N_CPUS);
	};

	Max_IPC_per_Watt_AVG_ONLY(const Max_IPC_per_Watt_AVG_ONLY &other)
		:_objective(other._objective),
		 _objective_ips(other._objective_ips),
		 _objective_power(other._objective_power),
		 _cpus_ips(0), _cpus_power(0),
		 input_data(Global::input_data()){

		_cpus_ips = Global::allocate_array<double>(Global::Input_Params::MAX_N_CPUS);
		_cpus_power = Global::allocate_array<double>(Global::Input_Params::MAX_N_CPUS);
		vassert(is_ok());

		for (int cpu = 0; cpu < Global::Input_Params::MAX_N_CPUS; ++cpu) _cpus_ips[cpu] = other._cpus_ips[cpu];
		for (int cpu = 0; cpu < Global::Input_Params::MAX_N_CPUS; ++cpu) _cpus_power[cpu] = other._cpus_power[cpu];
	};

	Max_IPC_per_Watt_AVG_ONLY&  operator=( const Max_IPC_per_Watt_AVG_ONLY& other ) {
		_objective = other._objective;
		_objective_ips = other._objective_ips;
		_objective_power = other._objective_power;

		//the matrices should have been allocates and different from other
		vassert(is_ok());

		vassert(_cpus_ips!=other._cpus_ips);
		vassert(_cpus_power!=other._cpus_power);

		for (int cpu = 0; cpu < Global::Input_Params::MAX_N_CPUS; ++cpu) _cpus_ips[cpu] = other._cpus_ips[cpu];
		for (int cpu = 0; cpu < Global::Input_Params::MAX_N_CPUS; ++cpu) _cpus_power[cpu] = other._cpus_power[cpu];

		return *this;
	}

	~Max_IPC_per_Watt_AVG_ONLY(){
		if(_cpus_ips != 0) Global::deallocate(_cpus_ips);
		if(_cpus_power != 0) Global::deallocate(_cpus_power);
	}

	bool is_ok(){
		return (_cpus_ips!=0) && (_cpus_power!=0);
	}

	void update_new(const Schedule<Max_IPC_per_Watt_AVG_ONLY> &sched){
		_objective = 0; _objective_ips= 0; _objective_power = 0;
		for (int i = 0; i < Global::Input_Params::N_CPUS; ++i) {
			calc_cpu_ipc_and_power(i, sched);
		}

		update_objective();
	}

	//must be called AFTER this switch:
	//sched[pos1] = task2 //prev was sched[pos1] = task1
	//sched[pos2] = task1 //prev was sched[pos2] = task2
	void update_on_switch(int task1, int cpu1, int task2, int cpu2,
			const Schedule<Max_IPC_per_Watt_AVG_ONLY> &sched){

		vassert((cpu1 >= 0)&&(cpu1 < Global::Input_Params::N_CPUS));
		vassert((cpu2 >= 0)&&(cpu2 < Global::Input_Params::N_CPUS));

		if(cpu1 == cpu2) return;

		if((task1 != 0) || (task2 != 0)){
			calc_cpu_ipc_and_power(cpu1, sched);
			calc_cpu_ipc_and_power(cpu2, sched);
		}

		update_objective();
	}

#ifndef __KERNEL__
	std::string to_string() const
	{
		std::stringstream output;
		output << "max_ipc_watt_avg_only=" << _objective << "(max_ipc= " << _objective_ips << " max_power= " << _objective_power <<")";
		return output.str();
	}

	friend std::ostream& operator<<( std::ostream &output,
			const Max_IPC_per_Watt_AVG_ONLY &s)
	{
		output << s.to_string();
		return output;
	}

	static std::string class_name()
	{
		std::stringstream output;
		output << "Max_IPC_per_Watt_AVG_ONLY";
		return output.str();
	}
#endif

public:
	const double& value() const{
		return _objective;
	}

	static const int SECUNDARY_OBJ_CNT = 2;
	const double& value(int sec) const{
		switch (sec) {
			case 0: return _objective_ips;
			case 1: return _objective_power;
			default: vassert(false);
		}
		return _objective_ips;
	}
};


};

};

#endif



