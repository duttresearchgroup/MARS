#ifndef __arm_rt_controllers_h
#define __arm_rt_controllers_h

#include <cmath>
#include <limits>

template<typename T>
class SISOController
{
	T kP;
	T kI;
	T kD;
	T yNorm;
	T uNorm;
	T yOffset;
	T uOffset;
	T ref;
	T prevInput;
    T integral;
    T derivative;
    T dt;
	T prevError;
	T prevAvgOutput;
	T avgWeight;
	T et_norm;
	T et_abs;


    /*T compute(T currErr)
	{
		return prevInput + (kP+kI)*currErr - kP*prevError;
	}*/

	/*T compute(T currErr)
	{
		T proportional = kP*currErr;
		integral += currErr*dt;
		derivative = (currErr - prevError)/dt;
		return proportional + integral + derivative;
	}*/

	T compute(T currErr)
	{
		T currDerivative = (dt * currErr - derivative) * kD;//TODO is that right ?
		T nextInput = (kP * currErr + integral) + currDerivative;
		integral += kI * currErr;
		derivative += currDerivative;
		return nextInput;
	}

	void set_error_tolerance()
	{
		et_abs = std::fabs(ref * et_norm);
	}

public:

	SISOController(T KP, T KI, T KD)
		:kP(KP),kI(KI),kD(KD),yNorm(1),uNorm(1),yOffset(0),uOffset(0),ref(0),
		 prevInput(0),integral(0),derivative(0),dt(0),
		 prevError(0),prevAvgOutput(0),avgWeight(0),et_norm(0),et_abs(0)
	{
	}

	SISOController():SISOController(0,0,0){};

	SISOController<T>& operator=(const SISOController<T> &o)
	{
		kP=o.kP;
		kI=o.kI;
		kD=o.kD;
		yNorm=o.yNorm;
		uNorm=o.uNorm;
		yOffset=o.yOffset;
		uOffset=o.uOffset;
		ref=o.ref;
		integral=o.integral;
		derivative=o.derivative;
        prevError = o.prevError;
		prevInput=o.prevInput;
		prevAvgOutput=o.prevAvgOutput;
		avgWeight=o.avgWeight;
		et_norm=o.et_norm;
		et_abs=o.et_abs;
		return *this;
	}

	T referenceOutput() { return (ref*yNorm)+yOffset;}
	void referenceOutput(T _ref)
	{
		ref = (_ref-yOffset)/yNorm;
		set_error_tolerance();
	}

	T nextInputVal(T _currOutput)
	{
		//filter
		T currOutput = (avgWeight*prevAvgOutput) + ((1-avgWeight)*_currOutput);

		//get error
		T currErr = ref - ((currOutput-yOffset)/yNorm);
		if(std::fabs(currErr) <= et_abs) currErr = 0;

		//compute next
		T nextInput = compute(currErr);

		//keep perevious values and return corrected input
		prevInput = nextInput;
		prevError = currErr;
		prevAvgOutput = currOutput;
		return (nextInput*uNorm)+uOffset;
	}

	T lastError() { return prevError;}

	void state(T currInput, T currOutput)
	{
	    prevInput = (currInput-uOffset)/uNorm;
		prevError = ref - ((currOutput-yOffset)/yNorm);
	}

	void offsets(T _yOffset, T _uOffset)
	{
	    yOffset = _yOffset;
	    uOffset = _uOffset;
	}

	void normalize(T _yNorm, T _uNorm)
	{
	    yNorm = _yNorm;
	    uNorm = _uNorm;
	}

	void period(T _dt) {dt = _dt;}

	void filterOutput(T c) { avgWeight = c;}

	void errorTolerance(T error){
		et_norm = error;
		set_error_tolerance();
	}



};


template<int NUM_INPUTS, int NUM_OUTPUTS, int ORDER, typename T=double>
class StateSpaceSystem
{
	T A[ORDER][ORDER];
	T B[ORDER][NUM_INPUTS];
	T C[NUM_OUTPUTS][ORDER];
	T D[NUM_OUTPUTS][NUM_INPUTS];
	T X[2][ORDER][1];
	int x_curr;
	int x_next;

	T yNorm[NUM_OUTPUTS];
	T uNorm[NUM_INPUTS];
	T yOffset[NUM_OUTPUTS];
	T uOffset[NUM_INPUTS];

	T nextOutput[NUM_OUTPUTS][1];
	T nextOutputCorrected[NUM_OUTPUTS];

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

	void compute(T currInput[NUM_INPUTS][1], T newOutput[NUM_OUTPUTS][1])
	{
		//compute next state
		matrix_mult<ORDER,ORDER,1>(X[x_next],A,X[x_curr]);
		matrix_mult_acc<ORDER,NUM_INPUTS,1>(X[x_next],B,currInput);

		//compute inputs
		matrix_mult<NUM_OUTPUTS,ORDER,1>(newOutput,C,X[x_curr]);
		matrix_mult_acc<NUM_OUTPUTS,NUM_INPUTS,1>(newOutput,D,currInput);

		int aux = x_curr;
		x_curr=x_next;
		x_next = aux;
	}


public:

	StateSpaceSystem()
	{
		for(int i = 0; i < NUM_OUTPUTS; ++i){
			yNorm[i] = 1;
			yOffset[i] = 0;
		}
		for(int i = 0; i < NUM_INPUTS; ++i){
			uNorm[i] = 1;
			uOffset[i] = 0;
		}
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


	T* nextOutputs(T _currInputs[NUM_INPUTS])
	{
		//normalize/offset
		T currInput[NUM_INPUTS][1];//matrix format
		for(int i = 0; i < NUM_INPUTS; ++i)	currInput[i][0] = (_currInputs[i]-uOffset[i])/uNorm[i];

		//compute next
		compute(currInput,nextOutput);

		//normalize/offset and return
		for(int i = 0; i < NUM_OUTPUTS; ++i) nextOutputCorrected[i] = (nextOutput[i][0]*yNorm[i])+yOffset[i];
		return nextOutputCorrected;
	}

	void offsetY(const T _yOffset[NUM_OUTPUTS]){ for(int i = 0; i < NUM_OUTPUTS; ++i) yOffset[i] = _yOffset[i];}
	T* offsetY(){ return yOffset;}
	void normalizeY(T _yNorm[NUM_OUTPUTS]){ for(int i = 0; i < NUM_OUTPUTS; ++i) yNorm[i] = _yNorm[i];}
	T* normalizeY(){return yNorm;}
	void offsetU(const T _uOffset[NUM_INPUTS]) { for(int i = 0; i < NUM_INPUTS; ++i) uOffset[i] = _uOffset[i];}
	T* offsetU(){ return uOffset;}
	void normalizeU(const T _uNorm[NUM_INPUTS]) { for(int i = 0; i < NUM_INPUTS; ++i) uNorm[i] = _uNorm[i];}
	T* normalizeU(){return uNorm;}

	T* currState() { return &(X[x_curr][0][0]);}
};

template<int NUM_INPUTS, int NUM_OUTPUTS, int SYS_ORDER, typename T=double>
class MIMOController
{
public:

	typedef T (*errorFuncType)(T);
	static T errorFunc_default(T val) { return val;}

	/*template<T abs_val>
	static T errorFunc_abs(T val)
	{
		if(std::fabs(val) <= abs_val) return 0;
		else return val;
	}
	template<T byVal>
	static T errorFunc_scale(T val) { return val*byVal; }
	template<T base>
	static T errorFunc_exp(T val)
	{
		if(val < 0 ) return std::pow(std::fabs(val),base)*-1;
		else return std::pow(val,base);
	}
	template<T byVal, T base>
	static T errorFunc_scale_exp(T val) { return errorFunc_exp<base>(errorFunc_scale<byVal>(val)); }*/


private:

	static const int ORDER = SYS_ORDER+NUM_OUTPUTS;

	StateSpaceSystem<NUM_OUTPUTS,NUM_INPUTS,ORDER,T> sss;

	T ref[NUM_OUTPUTS];
	T prevAvgOutput[NUM_OUTPUTS];
	T avgWeight[NUM_OUTPUTS];
	errorFuncType errorFunc[NUM_OUTPUTS];
	T prevError[NUM_OUTPUTS];

public:

	MIMOController()
	{
		for(int i = 0; i < NUM_OUTPUTS; ++i){
			ref[i] = 0;
			prevAvgOutput[i] = std::numeric_limits<double>::quiet_NaN();
			avgWeight[i] = 0;
			errorFunc[i] = errorFunc_default;
			prevError[i] = 0;
		}
	}

	T* referenceOutputs() { return ref;	}
	void referenceOutputs(T _ref[NUM_OUTPUTS])
	{
		for(int i = 0; i < NUM_OUTPUTS; ++i) ref[i] = _ref[i];
	}

	T* nextInputVal(T _currOutput[NUM_OUTPUTS])
	{
		//filter output
		T currOutputErr[NUM_OUTPUTS];
		if(std::isnan(prevAvgOutput[0])) for(int i = 0; i < NUM_OUTPUTS; ++i) prevAvgOutput[i] =  _currOutput[i];
		for(int i = 0; i < NUM_OUTPUTS; ++i)
			currOutputErr[i] = (avgWeight[i]*prevAvgOutput[i]) + ((1-avgWeight[i])*_currOutput[i]);
		//save this one to be the next prev output
		for(int i = 0; i < NUM_OUTPUTS; ++i) prevAvgOutput[i] = currOutputErr[i];

		//get error
		for(int i = 0; i < NUM_OUTPUTS; ++i){
			currOutputErr[i] = ref[i] - currOutputErr[i];
			currOutputErr[i] = (errorFunc[i])(currOutputErr[i]);
		}
		//save this one to be the next prev error
		for(int i = 0; i < NUM_OUTPUTS; ++i) prevError[i] = currOutputErr[i];

		//compute next
		return sss.nextOutputs(currOutputErr);
	}

	T* lastError() { return prevError;}

	void offsetInputs(const T inOffset[NUM_INPUTS]) { sss.offsetY(inOffset);}

	void normalizeInputs(const T inNorm[NUM_INPUTS]){ sss.normalizeY(inNorm);}

	void filterOutput(const T c[NUM_OUTPUTS]) { for(int i = 0; i < NUM_OUTPUTS; ++i) avgWeight[i] = c[i];}

	T* lastFilteredOutput() { return prevAvgOutput;}

	void setErrorFunc(const errorFuncType _errorFunc[NUM_OUTPUTS]){
		for(int i = 0; i < NUM_OUTPUTS; ++i) errorFunc[i] = _errorFunc[i];
	}

	void setMatrices(const T _A[ORDER][ORDER], const T _B[ORDER][NUM_OUTPUTS], const T _C[NUM_INPUTS][ORDER], const T _D[NUM_INPUTS][NUM_OUTPUTS])
	{
		sss.setMatrices(_A,_B,_C,_D);
	}

	T* currState() { return sss.currState();}

	void inputSaturation(const T _max[NUM_INPUTS], const T _min[NUM_INPUTS]) { sss.outputSaturation(_max, _min); }

};



#endif

