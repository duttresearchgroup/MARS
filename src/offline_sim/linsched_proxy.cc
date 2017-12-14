
#include "linsched_proxy.h"
#include <linsched_interface.h>
#include <mutex>


static std::mutex glb_mutex;
static volatile bool do_init = true;

Linsched::Linsched(int numcpus)
{
    glb_mutex.lock();

    if(do_init) {
        linsched_setup(numcpus);
        do_init = false;
    }
    linsched_reset(numcpus);
}

Linsched::~Linsched(){
    glb_mutex.unlock();
}

int
Linsched::create_task(double run, double sleep, int core, int niceval)
{
    return linsched_create_task(run,sleep,core,niceval);
}

void
Linsched::sim(double time)
{
    linsched_sim(time);
}

void
Linsched::task_info(int task, struct task_sched_info *info)
{
    linsched_task_info(task,info);
}

void
Linsched::print_info()
{
    linsched_print_info();
}




