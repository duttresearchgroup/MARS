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

#ifndef __arm_rt_controllers_new_h
#define __arm_rt_controllers_new_h

#include <base/base.h>

#include <cmath>
#include <limits>

namespace Filters {

template<typename T, int WIDTH, typename Derived>
class FilterBase{
protected:
	T _filteredVal[WIDTH];
public:

	FilterBase(){
		for(int i = 0; i < WIDTH; ++i){
			_filteredVal[i] = 0;
		}
	}

	void sampleInput(std::initializer_list<T> inputs){
		assert_true(WIDTH==inputs.size());
		int i = 0;
		for(auto in : inputs){
			_filteredVal[i] = static_cast<Derived*>(this)->_filterFunc(i,in);
			++i;
		}
	}
	void sampleInput(T *inputs){
		for(int i = 0; i < WIDTH; ++i)
			_filteredVal[i] = static_cast<Derived*>(this)->_filterFunc(i,inputs[i]);
	}
	void sampleInput(T input){
		assert_true(WIDTH==1);
		_filteredVal[0] = static_cast<Derived*>(this)->_filterFunc(0,input);
	}

	T output(int idx) {
		return _filteredVal[idx];
	}
	T output() {
		assert_true(WIDTH==1);
		return _filteredVal[0];
	}
	T* outputs() {
		return _filteredVal;
	}
};

template<typename T, typename FirstFilter, typename SecondFilter>
class CompositeFilter{
public:

	FirstFilter first;
	SecondFilter second;

	template<typename InputT>
	void sampleInput(InputT inputs){
		first.sampleInput(inputs);
		second.sampleInput(first.outputs());
	}

	T output(int idx) {
		return second.output(idx);
	}
	T output() {
		return second.output();
	}
	T* outputs() {
		return second.outputs();
	}
};

template<typename T, int WIDTH>
class NoFilter: public FilterBase<T,WIDTH,NoFilter<T,WIDTH>>{
public:
	/*
	 * filter processing func
	 */
	T _filterFunc(int idx, T inputVal){
		return inputVal;
	}
};

template<typename T, int WIDTH>
class Average: public FilterBase<T,WIDTH,Average<T,WIDTH>>{
	T _weight[WIDTH];
	using FilterBase<T,WIDTH,Average<T,WIDTH>>::_filteredVal;

public:

	Average(){
		for(int i = 0; i < WIDTH; ++i){
			_weight[i] = 0;
		}
	}

	/*
	 * Filter params
	 */
	//set weight for a sampling window to always store the average in the last sec.
	void weight(double windowLengthSec) {
		for(int i = 0; i < WIDTH; ++i)
			_weight[i] = (windowLengthSec) > 1 ? 0 : 1-windowLengthSec;
	}
	void weight(std::initializer_list<T> weights) {
		assert_true(WIDTH==weights.size());
		for(int i = 0; i < WIDTH; ++i) 	_weight[i] = weights[i];
	}
	void weight(T* weights) {
		for(int i = 0; i < WIDTH; ++i) 	_weight[i] = weights[i];
	}
	void weight(T weightVal) {
		assert_true(WIDTH==1);
		_weight[0] = weightVal;
	}

	/*
	 * filter processing func
	 */
	T _filterFunc(int idx, T inputVal){
		return (_weight[idx]*_filteredVal[idx]) + ((1-_weight[idx])*inputVal);
	}
};

template<typename T, int WIDTH>
class Error: public FilterBase<T,WIDTH,Error<T,WIDTH>>{
	T _ref[WIDTH];

public:
	Error(){
		for(int i = 0; i < WIDTH; ++i){
			_ref[i] = 0;
		}
	}

	/*
	 * Filter params
	 */
	void ref(std::initializer_list<T> refs) {
		assert_true(WIDTH==refs.size());
		int i = 0;
		for(auto ref : refs){
			_ref[i] = ref;
			++i;
		}
	}
	void ref(T* refs) {
		for(int i = 0; i < WIDTH; ++i) 	_ref[i] = refs[i];
	}
	void ref(T refVal) {
		assert_true(WIDTH==1);
		_ref[0] = refVal;
	}

	/*
	 * filter processing func
	 */
	T _filterFunc(int idx, T inputVal){
		return _ref[idx] - inputVal;
	}
};


template<typename T, int WIDTH>
class NormalizeBase{
protected:
	T _gainS[WIDTH];
	T _gainO[WIDTH];

public:
	NormalizeBase(){
		for(int i = 0; i < WIDTH; ++i){
			_gainS[i] = 0;
			_gainO[i] = 0;
		}
	}

	/*
	 * Filter params
	 */
	void range(std::initializer_list<T> minVals,std::initializer_list<T> maxVals) {
		assert_true(WIDTH==minVals.size());
		assert_true(WIDTH==maxVals.size());
		for(int i = 0; i < WIDTH; ++i) {
			_gainS[i] = (maxVals[i]-minVals[i])/2;
			_gainO[i] = -1*((maxVals[i]+minVals[i])/(maxVals[i]-minVals[i]));
		}
	}
	void range(const T* minVals,const T* maxVals) {
		for(int i = 0; i < WIDTH; ++i) {
			_gainS[i] = (maxVals[i]-minVals[i])/2;
			_gainO[i] = -1*((maxVals[i]+minVals[i])/(maxVals[i]-minVals[i]));
		}
	}
	void range(T minVal,T maxVal) {
		assert_true(WIDTH==1);
		_gainS[0] = (maxVal-minVal)/2;
		_gainO[0] = -1*((maxVal+minVal)/(maxVal-minVal));
	}
};

template<typename T, int WIDTH>
class Normalize: public NormalizeBase<T,WIDTH>, public FilterBase<T,WIDTH,Normalize<T,WIDTH>> {
	using NormalizeBase<T,WIDTH>::_gainS;
	using NormalizeBase<T,WIDTH>::_gainO;
public:
	/*
	 * filter processing func
	 */
	T _filterFunc(int idx, T inputVal){
		return (inputVal / _gainS[idx]) + _gainO[idx];
	}
};

template<typename T, int WIDTH>
class DeNormalize: public NormalizeBase<T,WIDTH>, public FilterBase<T,WIDTH,DeNormalize<T,WIDTH>> {
	using NormalizeBase<T,WIDTH>::_gainS;
	using NormalizeBase<T,WIDTH>::_gainO;
public:
	/*
	 * filter processing func
	 */
	T _filterFunc(int idx, T inputVal){
		return (inputVal - _gainO[idx]) * _gainS[idx];
	}
};

template<typename T, int WIDTH>
class Offset: public FilterBase<T,WIDTH,Offset<T,WIDTH>>{
	T _offset[WIDTH];
	using FilterBase<T,WIDTH,Offset<T,WIDTH>>::_filteredVal;

public:

	Offset(){
		for(int i = 0; i < WIDTH; ++i){
			_offset[i] = 0;
		}
	}

	/*
	 * Filter params
	 */
	void offset(std::initializer_list<T> offsets) {
		assert_true(WIDTH==offsets.size());
		for(int i = 0; i < WIDTH; ++i) 	_offset[i] = offsets[i];
	}
	void offset(T* offsets) {
		for(int i = 0; i < WIDTH; ++i) 	_offset[i] = offsets[i];
	}
	void offset(T offsetVal) {
		assert_true(WIDTH==1);
		_offset[0] = offsetVal;
	}

	/*
	 * filter processing func
	 */
	T _filterFunc(int idx, T inputVal){
		return inputVal + _offset[idx];
	}
};


template<typename T, int WIDTH>
class LinearInterp: public FilterBase<T,WIDTH,LinearInterp<T,WIDTH>>{
	T _xmin[WIDTH];
	T _xmax[WIDTH];
	T _ymin[WIDTH];
	T _ymax[WIDTH];

	using FilterBase<T,WIDTH,LinearInterp<T,WIDTH>>::_filteredVal;

public:

	LinearInterp(){
		for(int i = 0; i < WIDTH; ++i){
			_xmin[i] = 0;
			_xmax[i] = 0;
			_ymin[i] = 0;
			_ymax[i] = 0;
		}
	}

	/*
	 * Filter params
	 */
	void range(std::initializer_list<T> minValsX,std::initializer_list<T> maxValsX,
			   std::initializer_list<T> minValsY,std::initializer_list<T> maxValsY) {
		assert_true(WIDTH==minValsX.size());
		assert_true(WIDTH==maxValsX.size());
		assert_true(WIDTH==minValsY.size());
		assert_true(WIDTH==maxValsY.size());
		assert_false("Not implemented");
	}
	void range(const T* minValsX,const T* maxValsX,const T* minValsY,const T* maxValsY) {
		assert_false("Not implemented");
	}
	void range(T minValX,T maxValX,T minValY,T maxValY) {
		assert_true(WIDTH==1);
		_xmin[0] = minValX;
		_xmax[0] = maxValX;
		_ymin[0] = minValY;
		_ymax[0] = maxValY;
	}

	/*
	 * filter processing func
	 */
	T _filterFunc(int idx, T inputVal){
		return _ymin[idx] + ((_ymax[idx]-_ymin[idx])*(inputVal-_xmin[idx]))/(_xmax[idx]-_xmin[idx]);
	}
};

//only implementation for WIDTH=1 is valid for PID
template<typename T, int WIDTH=1>
class PID: public FilterBase<T,1,PID<T,1>>{
	T kP;
	T kI;
	T kD;
    T integral;
    T derivative;
    T dt;

	using FilterBase<T,1,PID<T,1>>::_filteredVal;

public:

	PID(T KP, T KI, T KD)
		:kP(KP),kI(KI),kD(KD),
		 integral(0),derivative(0),dt(0)
	{
	}
	PID():PID(0,0,0){};

	/*
	 * Filter params
	 */
	void gains(T KP, T KI, T KD) {
		kP=KP;kI=KI;kD=KD;
	}
	void period(T _dt) {dt = _dt;}

	/*
	 * filter processing func
	 */
	T _filterFunc(int idx, T currErr){
		T currDerivative = (dt * currErr - derivative) * kD;//TODO is that right ?
		T nextInput = (kP * currErr + integral) + currDerivative;
		integral += kI * currErr;
		derivative += currDerivative;
		return nextInput;
	}
};


/*
 * State space filter does not use FIlterBase and has it's own interface
 */
template<typename T, int NUM_INPUTS, int NUM_OUTPUTS,int ORDER>
class StateSpaceSystem
{
	T A[ORDER][ORDER];
	T B[ORDER][NUM_INPUTS];
	T C[NUM_OUTPUTS][ORDER];
	T D[NUM_OUTPUTS][NUM_INPUTS];
	T X[2][ORDER][1];
	int x_curr;
	int x_next;

	T nextOutput[NUM_OUTPUTS][1];

	//multiply and also accumulates
	template<int A_ROWS, int A_COLS, int B_COLS>
	inline void matrix_mult_acc(T c[A_ROWS][B_COLS], T a[A_ROWS][A_COLS], T b[A_COLS][B_COLS]){
		for (int i=0; i<A_ROWS; ++i)
			for(int j=0; j<B_COLS; ++j)
				for(int k=0; k<A_COLS; ++k)
					c[i][j] += a[i][k]*b[k][j];
	}

	template<int A_ROWS, int A_COLS, int B_COLS>
	inline void matrix_mult(T c[A_ROWS][B_COLS], T a[A_ROWS][A_COLS], T b[A_COLS][B_COLS]){
		for (int i=0; i<A_ROWS; ++i)
			for(int j=0; j<B_COLS; ++j)
				c[i][j] = 0;
		matrix_mult_acc<A_ROWS,A_COLS,B_COLS>(c,a,b);
	}


public:

	StateSpaceSystem()
	{
		x_curr = 0;
		x_next = 1;
	}

	void setMatrices(const T _A[ORDER][ORDER], const T _B[ORDER][NUM_INPUTS], const T _C[NUM_OUTPUTS][ORDER], const T _D[NUM_OUTPUTS][NUM_INPUTS]){
		for(int i = 0; i < ORDER; ++i) for(int j = 0; j < ORDER; ++j)		A[i][j] = _A[i][j];
		for(int i = 0; i < ORDER; ++i) for(int j = 0; j < NUM_INPUTS; ++j)	B[i][j] = _B[i][j];
		for(int i = 0; i < NUM_OUTPUTS; ++i) for(int j = 0; j < ORDER; ++j)	C[i][j] = _C[i][j];
		for(int i = 0; i < NUM_OUTPUTS; ++i) for(int j = 0; j < NUM_INPUTS; ++j) D[i][j] = _D[i][j];
		for(int j = 0; j < ORDER; ++j) X[x_curr][j][0] = 0;
		for(int j = 0; j < ORDER; ++j) X[x_next][j][0] = 0;
	}


	T* nextOutputs(T currInput[NUM_INPUTS][1])
	{
		//compute next state
		matrix_mult<ORDER,ORDER,1>(X[x_next],A,X[x_curr]);
		matrix_mult_acc<ORDER,NUM_INPUTS,1>(X[x_next],B,currInput);

		//compute inputs
		matrix_mult<NUM_OUTPUTS,ORDER,1>(nextOutput,C,X[x_curr]);
		matrix_mult_acc<NUM_OUTPUTS,NUM_INPUTS,1>(nextOutput,D,currInput);

		int aux = x_curr;
		x_curr=x_next;
		x_next = aux;

		return &(nextOutput[0][0]);
	}

	T* nextOutputs(T *currInput){
		return nextOutputs(*reinterpret_cast<T (*)[NUM_INPUTS][1]>(currInput));
	}

	T* outputs() { return &(nextOutput[0][0]);}
};


};


namespace Controllers {

template<typename T, typename ErrorFilter, typename InputFilter>
class SISO {
public:
	//access fields directly to set params
	Filters::PID<T,1> pid;
	ErrorFilter errorFilter;
	InputFilter inputFilter;

	T nextInput(T sysOutput){
		errorFilter.sampleInput(sysOutput);
		pid.sampleInput(errorFilter.output());
		inputFilter.sampleInput(pid.output());
		return inputFilter.output();
	}
};

template<typename T, int NUM_OUTPUTS, typename ErrorFilter, int NUM_INPUTS, typename InputFilter, int SYS_ORDER>
class MIMO {
public:

	static const int ORDER = SYS_ORDER+NUM_OUTPUTS;

	//access fields directly to set params
	Filters::StateSpaceSystem<T,NUM_OUTPUTS,NUM_INPUTS,ORDER> sss;
	ErrorFilter errorFilter;
	InputFilter inputFilter;

	T* nextInputs(std::initializer_list<T> sysOutputs){
		errorFilter.sampleInput(sysOutputs);
		T *aux = sss.nextOutputs(errorFilter.outputs());
		inputFilter.sampleInput(aux);
		return inputFilter.outputs();
	}

	T* unfilteredInputs() { return sss.outputs();}

};

};



#endif

