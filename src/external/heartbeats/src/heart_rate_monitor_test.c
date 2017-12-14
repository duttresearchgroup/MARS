#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "heartbeat.h"
#include <time.h>
#include <signal.h>
#include <stdlib.h>

#define PAGE_SIZE 4096
heartbeat_t heart;

void call_heartbeat_finish(int signum){

    heartbeat_finish(&heart);

    exit(signum);
}

int main ( int argc, char **argv )
{
//    char temp_buffer[1024];
//    int r = 0;
    int64_t i = 0; 

    signal(SIGINT, call_heartbeat_finish);

//    srand(time(NULL));
//    r = (rand()%65);

    if(heartbeat_init(&heart, 0, 1000000, 100, 80, NULL))
        return 0;

    while(1){
        heartbeat(&heart, i);
        if(i % 100000 == 0)
            printf("In application ->  global_rate : %lf, tag : %d \n", hb_get_global_rate(&heart), heart.log[heart.state->buffer_index].tag);
        i++;
    }
//    read(heart.binary_file, temp_buffer , 1024);
//    printf("rand : %d, pid : %d, read :\n%s" ,r ,getpid(), temp_buffer);
//    printf("In application -> global_rate : %lld\n", hb_get_global_rate(&heart));

//    heartbeat_finish(&heart);

    return 0;
}

