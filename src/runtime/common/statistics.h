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

#ifndef __arm_rt_stats_h
#define __arm_rt_stats_h

#include <cassert>
#include <cmath>
#include <vector>
#include <string>

struct Statistics {
    std::string         sampleXName;
    std::string         sampleYName;
    std::vector<double> samplesX;
    std::vector<double> samplesY;

    Statistics(std::string x, std::string y)
        :sampleXName(x), sampleYName(y)
    {

    }

    struct Average{
        double average;
        double stddev;
    };

    struct Line{
        double slope;
        double intercept;
    };

    Average average(std::vector<double> &samples){
        double acc = 0;
        double accSqr = 0;
        for(auto s : samples){
            acc += s;
            accSqr += s*s;
        }
        Average avg = {acc/samples.size(),0};
        avg.stddev = std::sqrt((accSqr/samples.size())- (avg.average*avg.average));
        return avg;
    }

    double min(std::vector<double> &samples){
        double min = std::numeric_limits<double>::max();
        for(auto s : samples){
            if(s <= min) min = s;
        }
        return min;
    }
    double max(std::vector<double> &samples){
        double max = 0;
        for(auto s : samples){
            if(s >= max) max = s;
        }
        return max;
    }

    double correlation(){
        Average avgX = average(samplesX);
        Average avgY = average(samplesY);

        //calculate covariance
        assert(samplesX.size() == samplesY.size());
        double cov = 0;
        for(unsigned i = 0; i < samplesX.size(); ++i){
            cov += (samplesX[i] - avgX.average) * (samplesY[i] - avgY.average);
        }
        cov /= samplesX.size();

        //correlation
        return cov / (avgX.stddev*avgY.stddev);
    }

    void addSample(double x, double y){
        samplesX.push_back(x);
        samplesY.push_back(y);
    }

    Line least_squares(){
        assert(samplesX.size() == samplesY.size());
        unsigned size = samplesX.size();
        double sumX=0,sumY=0;
        for(unsigned i = 0; i < size; ++i){
            sumX += samplesX[i];
            sumY += samplesY[i];
        }
        double avgX = sumX/size;

        std::vector<double> samplesZ;
        double sumZsqr = 0;
        for(unsigned i = 0; i < size; ++i){
            double aux = samplesX[i]-avgX;
            samplesZ.push_back(aux);
            sumZsqr += aux*aux;
        }

        double sumYZ = 0;
        for(unsigned i = 0; i < size; ++i)
            sumYZ += samplesZ[i] * samplesY[i];

        Line l;
        l.slope = sumYZ / sumZsqr;
        l.intercept = (sumY-sumX*l.slope) / size;
        return l;
    }


};

#endif
