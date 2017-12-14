/** \file 
 *  \brief 
 *  \author Hank Hoffmann 
 *  \version 1.0
 */
#include "heart_rate_monitor.h"
#include <stdlib.h>
#include <string.h>

int heart_rate_monitor_init(heart_rate_monitor_t* hrm, 
			    int pid) {
  key_t key;
  int rc = 0;

  key = pid;


  hrm->state = (HB_global_state_t*) malloc(sizeof(HB_global_state_t));


  hrm->log = (heartbeat_record_t*)  malloc(sizeof(heartbeat_record_t));

  sprintf(hrm->filename, "/data/oldcag/home/bits7/hank/heartbeats/%d", pid);  
  //hrm->file = fopen(hrm->filename, "rb");

  return rc;
}

void heart_rate_monitor_finish(heart_rate_monitor_t* heart) {

}

int hrm_get_current(heart_rate_monitor_t volatile * hb, 
		     heartbeat_record_t volatile * record) {
  
  int rc = -1;
  hb->file = fopen(hb->filename, "rb");

  rc = fseek(hb->file, -1*sizeof(heartbeat_record_t), SEEK_END);
  
  //printf("seek rc = %d\n", rc);

  if (rc == 0)
    rc = fread(record, sizeof(heartbeat_record_t), 1, hb->file);
  
  //printf("fread rc = %d\n", rc);
  fclose(hb->file);

  return !(rc == 1);
}

int hrm_get_history(heart_rate_monitor_t volatile * hb,
		     heartbeat_record_t volatile * record,
		     int n) {
  
  int rc = -1;

  rc = fseek(hb->file, -n*sizeof(heartbeat_record_t), SEEK_END);
  
  //printf("seek rc = %d\n", rc);

  if (rc == 0)
    rc = fread(record, sizeof(heartbeat_record_t), n, hb->file);
  
  //printf("fread rc = %d\n", rc);
  //fclose(file);

  return !(rc == 1);
}

double hrm_get_global_rate(heart_rate_monitor_t volatile * hb) {
  return hb->log[hb->state->counter].global_rate;
}

double hrm_get_windowed_rate(heart_rate_monitor_t volatile * hb) {
  return hb->log[hb->state->counter].window_rate;
}

double hrm_get_min_rate(heart_rate_monitor_t volatile * hb) {
  return hb->state->min_heartrate;
}

double hrm_get_max_rate(heart_rate_monitor_t volatile * hb) {
  return hb->state->max_heartrate;
}

int64_t hrm_get_window_size(heart_rate_monitor_t volatile * hb) {
  return hb->state->window_size;
}

