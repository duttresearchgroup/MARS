#ifndef __criteria_h
#define __criteria_h

namespace SASolverImpl {

namespace Stop_Criteria{

//met when the diff. between the curr. solution and target is <= than the given precision
class Min_Precision {
	double value;
public:
	Min_Precision(const double &other) :value(other) {};

	bool met(double &curr_solution_dist_to_target, double &prev_solution_dist_to_target) {
		return curr_solution_dist_to_target <= value;
	}
};

//met when the diff. between the curr. solution and the new one is is <= than
//the given precision during the specified number of iterations
class Converged {
	const double diff;
	const int iter_target;
	int iteration;
public:
	Converged(const double &_diff, const int &_iterations) :diff(_diff), iter_target(_iterations), iteration(0) {};

	bool met(double &curr_solution_dist_to_target, double &prev_solution_dist_to_target) {
		double calc_diff = curr_solution_dist_to_target-prev_solution_dist_to_target;
		if(calc_diff <= diff) ++iteration;
		else iteration = 0;

		if(iteration == iter_target) return true;
		else return false;
	}
};

//always run all iterations
class Never {
public:
	bool met(double &curr_solution_dist_to_target, double &prev_solution_dist_to_target) {
		return false;
	}
};

};

};

#endif
