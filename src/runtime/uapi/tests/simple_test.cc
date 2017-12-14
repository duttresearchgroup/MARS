
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

	std::cout << "DOne with " << task_beat_read_total(beats) << " beats, rate = " << task_beat_read_total_rate(info,beats) << " beats/s, tgt = " << task_beat_read_tgt_rate(info,beats)  <<" beats/s\n";

	return 0;
}
