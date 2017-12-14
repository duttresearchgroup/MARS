

#include "linsched_interface.h"
#include <string.h>
#include <getopt.h>
#include <stdio.h>

/* one minute */
#define TEST_TICKS 60000

void task_info(int t){
    struct task_sched_info info;
    info.task_id = t;
    linsched_task_info(t,&info);
    printf("task %d rt=%f\n",t,info.run_time);
}

void test_1(void){
    linsched_reset(1);

    linsched_create_task(0.1,0.1,0,0);
    linsched_create_task(0.2,0.1,0,0);
    linsched_create_task(0.2,0.0,0,0);

    linsched_sim(60);

    task_info(0);
    task_info(1);
    task_info(2);

    //linsched_print_info();
    printf("\n");
}

void test_2(void){
    linsched_reset(4);

    linsched_create_task(0.1,0.1,0,0);
    linsched_create_task(0.2,0.1,0,0);
    linsched_create_task(0.2,0.0,1,0);
    linsched_create_task(0.2,0.0,2,0);
    linsched_create_task(0.2,0.0,3,0);

    linsched_sim(60);

    task_info(0);
    task_info(1);
    task_info(2);
    task_info(3);
    task_info(4);

    //linsched_print_info();
    printf("\n");
}


void test_3(void){
    linsched_reset(2);

    linsched_create_task(0.1,0.1,0,-20);
    linsched_create_task(0.2,0.1,0,20);
    linsched_create_task(0.2,0.0,1,20);
    linsched_create_task(0.2,0.0,1,-20);
    linsched_create_task(0.2,0.0,1,-20);

    linsched_sim(60);

    task_info(0);
    task_info(1);
    task_info(2);
    task_info(3);
    task_info(4);


    //linsched_print_info();
    printf("\n");
}

void test_4(void){
    linsched_reset(8);

    linsched_create_task(0.1,0.1,4,0);

    linsched_sim(60);

    task_info(0);

    //linsched_print_info();
    printf("\n");
}


int linsched_test_main(int argc, char **argv)
{
    linsched_setup();

    test_1();
    test_2();
    test_3();
    test_4();

    printf("\n");
    printf("\n");

    test_4();
    test_3();
    test_2();
    test_1();

	return 0;
}


