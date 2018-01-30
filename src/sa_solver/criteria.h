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
