#ifndef _HEARTBEAT_H_
#define _HEARTBEAT_H_

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

typedef struct {
  int64_t beat;
  int tag;
  int64_t timestamp;
  int64_t global_rate;//double => int64_t
  int64_t  window_rate;//double => int64_t
  int64_t instant_rate;//double => int64_t
} heartbeat_record_t;

typedef struct {
  int pid;
  int64_t min_heartrate;//double => int64_t
  int64_t max_heartrate;//double => int64_t
  int64_t window_size;

  int64_t counter;
  int64_t buffer_depth;
  int64_t buffer_index;
  int64_t read_index;
  char    valid;

} HB_global_state_t;

typedef struct {
  int64_t first_timestamp;
  int64_t last_timestamp;

  int64_t* window;
  //int64_t window_size;
  int64_t current_index;
  
  int steady_state;
  int64_t last_average_time;//double => int64_t

  heartbeat_record_t* log;

  int binary_file;
  FILE* text_file;
  char filename[256];
  pthread_mutex_t mutex;

  HB_global_state_t* state;

} heartbeat_t;

int heartbeat_init(heartbeat_t * hb, 
		   double min_target, 
		   double max_target, 
		   int64_t window_size, 
		   int64_t buffer_depth,
		   char* log_name);

void heartbeat_finish(heartbeat_t * hb);

void hb_get_current(heartbeat_t volatile * hb, 
		    heartbeat_record_t volatile * record);

int hb_get_history(heartbeat_t volatile * hb,
		   heartbeat_record_t volatile * record,
		   int n);

double hb_get_global_rate(heartbeat_t volatile * hb);//return type : double => int64_t

double hb_get_windowed_rate(heartbeat_t volatile * hb);//return type : double => int64_t

double hb_get_min_rate(heartbeat_t volatile * hb);//return type : double => int64_t

double hb_get_max_rate(heartbeat_t volatile * hb);//return type : double => int64_t

int64_t hb_get_window_size(heartbeat_t volatile * hb);

int64_t heartbeat( heartbeat_t* hb, 
		   int tag );


#endif 
