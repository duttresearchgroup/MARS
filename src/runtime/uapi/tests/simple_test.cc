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

#include <time.h>
#include <iostream>

#include "../beats.h"



int main(int argc, char * argv[]){

	task_beat_info_t *info = task_beat_register_task();
	task_beat_data_t *beats = task_beat_create_domain(info,BEAT_PERF,30000000);

	const int period_ms = 10;
	for(int i = 0; i < 500; ++i){
		auto time_start = _task_beat_current_timestamp_ms();
		for(int j = 0; j < 300000; ++j) task_beat(beats);
		auto time_total = _task_beat_current_timestamp_ms() - time_start;
		if(time_total < period_ms) usleep((period_ms-time_total)*1000);
	}

	std::cout << "Done with " << task_beat_read_total(beats) << " beats, rate = " << task_beat_read_total_rate(info,beats) << " beats/s, tgt = " << task_beat_read_tgt_rate(info,beats)  <<" beats/s\n";

	return 0;
}
