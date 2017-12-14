#include "heartbeat.h"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

static int ONE_BILLION = 1000000000;
static int ONE_MILLION = 1000000;

int heartbeat_init(heartbeat_t* hb, 
        double min_target, 
        double max_target, 
        int64_t window_size,
        int64_t buffer_depth,
        char* log_name) {
    // FILE* file;
//    char temp_buffer[1024];
    int pid = getpid();
    char *mmap_address = NULL;
    //  char hb_filename[256];
    hb->binary_file = open("/sys/kernel/debug/heartbeat", O_RDWR);
    if(hb->binary_file < 0){
        perror("Heartbeat module not inserted\n");
        return -1;
    }

    if(buffer_depth > 64){
        printf("Buffer_depth should be less than 65.\n");
        buffer_depth = 64;
    }

    mmap_address = mmap(NULL, sizeof(HB_global_state_t) + buffer_depth*sizeof(heartbeat_record_t), PROT_READ | PROT_WRITE, MAP_SHARED, hb->binary_file, 0);

    if(mmap_address == MAP_FAILED){
        perror("mmap error\n");
        return 1;
    }else{
        hb->state = (HB_global_state_t *)mmap_address;
        hb->log = (heartbeat_record_t *)(mmap_address + sizeof(HB_global_state_t));
    }

    hb->state->pid = pid;
    hb->first_timestamp = hb->last_timestamp = -1;
    hb->state->window_size = window_size;
    hb->window = (int64_t*) malloc(window_size*sizeof(int64_t));
    hb->current_index = 0;
    hb->state->min_heartrate = (int64_t)(min_target * (double)ONE_MILLION); //sclaed 1000000
    hb->state->max_heartrate = (int64_t)(max_target * (double)ONE_MILLION); //sclaed 1000000
    hb->state->counter = 0;
    hb->state->buffer_index = 0;
    hb->state->read_index = 0;
    hb->state->buffer_depth = buffer_depth;
    //pthread_mutex_init(&hb->mutex, NULL);///TODO TODO fix
    hb->steady_state = 0;
    hb->state->valid = 0;

    return 0;
}

/**
 * Cleanup function for process that
 * wants to register heartbeats
 * @param hb pointer to heartbeat_t
 */
void heartbeat_finish(heartbeat_t* hb) {
    free(hb->window);
    if(hb->text_file != NULL)
        fclose(hb->text_file);
    remove(hb->filename);
    ioctl(hb->binary_file, 0);
    close(hb->binary_file);
    /*TODO : need to deallocate log */
}

/**
 * Returns the record for the current heartbeat
 * currently may read old data
 * @param hb pointer to heartbeat_t
 * @see
 * @return 
 */
void hb_get_current(heartbeat_t volatile * hb, 
        heartbeat_record_t volatile * record) {
    // uint64_t read_index =  (hb->state->buffer_index-1) % hb->state->buffer_depth;
    //memcpy(record, &hb->log[hb->state->read_index], sizeof(heartbeat_record_t));
    record->beat         = hb->log[hb->state->read_index].beat;
    record->tag          = hb->log[hb->state->read_index].tag;
    record->timestamp    = hb->log[hb->state->read_index].timestamp;
    record->global_rate  = hb->log[hb->state->read_index].global_rate;
    record->window_rate  = hb->log[hb->state->read_index].window_rate;
    record->instant_rate = hb->log[hb->state->read_index].instant_rate;
}

/**
 * Returns all heartbeat information for the last n heartbeats
 * @param hb pointer to heartbeat_t
 * @param record pointer to heartbeat_record_t
 * @param n integer
 */
int hb_get_history(heartbeat_t volatile * hb,
        heartbeat_record_t volatile * record,
        int n) {
    if(hb->state->counter > hb->state->buffer_index) {
        memcpy(record, 
                &hb->log[hb->state->buffer_index], 
                (hb->state->buffer_index*hb->state->buffer_depth)*sizeof(heartbeat_record_t));
        memcpy(record + (hb->state->buffer_index*hb->state->buffer_depth), 
                &hb->log[0], 
                (hb->state->buffer_index)*sizeof(heartbeat_record_t));
        return hb->state->buffer_depth;
    }
    else {
        memcpy(record, 
                &hb->log[0], 
                hb->state->buffer_index*sizeof(heartbeat_record_t));
        return hb->state->buffer_index;
    }
}

/**
 * Returns the heart rate over the life 
 * of the entire application
 * @param hb pointer to heartbeat_t
 * @return the heart rate (double) over the entire life of the application
 */

//return double => int64_t
double hb_get_global_rate(heartbeat_t volatile * hb) {
    //uint64_t read_index =  (hb->state->buffer_index-1) % hb->state->buffer_depth;
    //printf("Reading from %lld\n", (long long int) hb->state->read_index);
//    printf("hb->state->read_index :  %lld\n", (long long int) hb->state->read_index);
    return (double)(hb->log[hb->state->read_index].global_rate) / (double)ONE_MILLION;
}

/**
 * Returns the heart rate over the last 
 * window (as specified to init) heartbeats
 * @param hb pointer to heartbeat_t
 * @return the heart rate (double) over the last window 
 */
//return double => int64_t
double hb_get_windowed_rate(heartbeat_t volatile * hb) {
    //uint64_t read_index =  (hb->state->buffer_index-1) % hb->state->buffer_depth;
    //printf("Reading from %lld\n", (long long int) read_index);
    return (double)(hb->log[hb->state->read_index].window_rate) / (double)ONE_MILLION;
}

/**
 * Returns the minimum desired heart rate
 * @param hb pointer to heartbeat_t
 * @return the minimum desired heart rate (double)
 */
//return double => int64_t
double hb_get_min_rate(heartbeat_t volatile * hb) {
    return (double)(hb->state->min_heartrate) / (double)ONE_MILLION;
}

/**
 * Returns the maximum desired heart rate
 * @param hb pointer to heartbeat_t
 * @return the maximum desired heart rate (double)
 */
//return double => int64_t
double hb_get_max_rate(heartbeat_t volatile * hb) {
    return (double)(hb->state->max_heartrate) / (double)ONE_MILLION;
}

/**
 * Returns the size of the sliding window 
 * used to compute the current heart rate
 * @param hb pointer to heartbeat_t 
 * @return the size of the sliding window (int64_t)
 */
int64_t hb_get_window_size(heartbeat_t volatile * hb) {
    return hb->state->window_size;
}

/**
 * Helper function to compute windowed heart rate
 * @param hb pointer to heartbeat_t
 * @param time int64_t
 */
static inline float hb_window_average(heartbeat_t volatile * hb, int64_t time) {
    int i;
    double average_time = 0;
    double fps;

    if(!hb->steady_state) {
        hb->window[hb->current_index] = time;

        for(i = 0; i < hb->current_index+1; i++) {
            average_time += (double) hb->window[i];
        }
        average_time = average_time / ((double) hb->current_index + 1 );
        hb->last_average_time = average_time;
        hb->current_index++;
        if( hb->current_index == hb->state->window_size) {
            hb->current_index = 0;
            hb->steady_state = 1;
        }
    }
    else {
        average_time = 
            hb->last_average_time - 
            ((double) hb->window[hb->current_index] / (double) hb->state->window_size);
        average_time += (double) time / (double) hb->state->window_size;

        hb->last_average_time = average_time;

        hb->window[hb->current_index] = time;
        hb->current_index++;

        if( hb->current_index == hb->state->window_size)
            hb->current_index = 0;
    }
    fps = ( ONE_BILLION / (float) average_time );

    return fps;
}

/**
 * 
 * @param hb pointer to heartbeat_t
 */
static void hb_flush_buffer(heartbeat_t volatile * hb) {
    int64_t i;
    int64_t nrecords = hb->state->buffer_depth;

    //printf("Flushing buffer - %lld records\n", 
    //	 (long long int) nrecords);

    if(hb->text_file != NULL) {
        for(i = 0; i < nrecords; i++) {
            fprintf(hb->text_file, 
                    "%lld    %d    %lld    %lld    %lld    %lld\n", 
                    hb->log[i].beat,
                    hb->log[i].tag,
                    hb->log[i].timestamp,
                    hb->log[i].global_rate,
                    hb->log[i].window_rate,
                    hb->log[i].instant_rate);
        }

        fflush(hb->text_file);
    }
}
/**
       * Registers a heartbeat
       * @param hb pointer to heartbeat_t
       * @param tag integer
       */
int64_t heartbeat(heartbeat_t* hb, int tag )
{
    struct timespec time_info;
    int64_t time;
    int64_t old_last_time = hb->last_timestamp;

    //printf("Registering Heartbeat\n");
    clock_gettime( CLOCK_REALTIME, &time_info );

    time = ( (int64_t) time_info.tv_sec * ONE_BILLION + (int64_t) time_info.tv_nsec );
    //pthread_mutex_lock(&hb->mutex);///TODO TODO fix
    hb->last_timestamp = time;


    if(hb->first_timestamp == -1) {
        //printf("In heartbeat - first time stamp\n");
        hb->first_timestamp = time;
        hb->last_timestamp  = time;
        hb->window[0] = 0;

        //printf("             - accessing state and log\n");
        
        hb->log[0].beat = hb->state->counter;
        hb->log[0].tag = tag;
        hb->log[0].timestamp = time;
        hb->log[0].window_rate = 0;
        hb->log[0].instant_rate = 0;
        hb->log[0].global_rate = 0;

        hb->state->counter++;                 //wierd for count compare to read_index 
        hb->state->buffer_index++;
        hb->state->valid = 1;
    }
    else {
        int index =  hb->state->buffer_index;
        hb->last_timestamp = time;
        double window_heartrate = hb_window_average(hb, time-old_last_time);
        double global_heartrate = (double)(hb->state->counter+1)/(double)(time - hb->first_timestamp)*(double)ONE_BILLION;

        double instant_heartrate = (double)ONE_BILLION / (double)(time - old_last_time); 

        hb->log[index].beat = hb->state->counter;
        hb->log[index].tag = tag;
        hb->log[index].timestamp = time;
        hb->log[index].window_rate =  (int64_t)(window_heartrate  * (double)ONE_MILLION); // scaled 1000000
        hb->log[index].instant_rate = (int64_t)(instant_heartrate * (double)ONE_MILLION); // scaled 1000000
        hb->log[index].global_rate =  (int64_t)(global_heartrate  * (double)ONE_MILLION); // scaled 1000000
        hb->state->buffer_index++;
        hb->state->counter++;
        hb->state->read_index++;

        if(hb->state->buffer_index%hb->state->buffer_depth == 0) {
            if(hb->text_file != NULL)
                hb_flush_buffer(hb);
            hb->state->buffer_index = 0;
        }
        if(hb->state->read_index%hb->state->buffer_depth == 0) {
            hb->state->read_index = 0;
        }
    }
    //pthread_mutex_unlock(&hb->mutex);///TODO TODO fix
    return time;
}
