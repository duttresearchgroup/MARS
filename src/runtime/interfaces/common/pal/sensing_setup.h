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

#ifndef __arm_rt_sensing_setup_h
#define __arm_rt_sensing_setup_h


/*
 * We should stop using the stuff from pal_setup.h in daemon code.
 * So the used-level sensing interfaces setup are defined in this file.
 *
 */


template<typename SensingModule>
void pal_sensing_setup(SensingModule *m);


template<typename SensingModule>
void pal_sensing_teardown(SensingModule *m);

#endif
