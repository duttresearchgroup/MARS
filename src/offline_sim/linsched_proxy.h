#ifndef __linsched_proxy_h
#define __linsched_proxy_h

#include <linsched_interface.h>

class Linsched {

public:
    Linsched(int numcpus);
    ~Linsched();

    int create_task(double run, double sleep, int core, int niceval);
    void sim(double time);
    void task_info(int task, struct task_sched_info *info);
    void print_info();

};


#endif
