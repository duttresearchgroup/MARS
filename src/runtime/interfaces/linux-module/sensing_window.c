#include "../linux-module/sensing_window.h"


#include <linux/module.h>
#include <linux/kernel.h>

#include "../linux-module/core.h"
#include "../linux-module/sense.h"
#include "../linux-module/sense_data.h"
#include "../linux-module/setup.h"
#include "../linux-module/user_if.h"


struct sensing_window_ctrl_struct {
	int wid;
	int period;//In terms of MINIMUM_WINDOW_LENGHT
	int period_ms;
	int period_jiffies;
	int time_to_ready;//In terms of MINIMUM_WINDOW_LENGHT
	//Link for adding this to the list of sensing windows
	define_list_addable_default(struct sensing_window_ctrl_struct);
};
typedef struct sensing_window_ctrl_struct sensing_window_ctrl_t;

static sensing_window_ctrl_t sensing_windows[MAX_WINDOW_CNT];
int sensing_window_cnt = 0;

//list of sensing windows to keep track of the ones ready to execute
define_vitamins_list(static sensing_window_ctrl_t,next_window);
//list is ordered by time_to_ready
inline static bool sw_order_crit(sensing_window_ctrl_t *a, sensing_window_ctrl_t *b) {
    return (a->time_to_ready == b->time_to_ready) ? (a->period < b->period) : (a->time_to_ready < b->time_to_ready);
}


int create_sensing_window(int period_ms)
{
	int i;
	sensing_window_ctrl_t *iter;
	int period = period_ms / MINIMUM_WINDOW_LENGHT_MS;

	if((period <= 0) || ((period_ms % MINIMUM_WINDOW_LENGHT_MS)!=0))
		return WINDOW_INVALID_PERIOD;

	if(sensing_window_cnt >= MAX_WINDOW_CNT)
		return WINDOW_MAX_NWINDOW;

	for(i = 0; i < sensing_window_cnt; ++i)
		if(sensing_windows[i].period == period)
			return WINDOW_EXISTS;

	sensing_windows[sensing_window_cnt].wid = sensing_window_cnt;
	sensing_windows[sensing_window_cnt].period = period;
	sensing_windows[sensing_window_cnt].period_ms = period_ms;
	sensing_windows[sensing_window_cnt].period_jiffies = (period_ms*CONFIG_HZ)/1000;
	sensing_windows[sensing_window_cnt].time_to_ready = period;
	clear_object_default(&(sensing_windows[sensing_window_cnt]));
	add_to_priority_list_default(next_window,&(sensing_windows[sensing_window_cnt]),sw_order_crit,iter);
	++sensing_window_cnt;

	return sensing_windows[sensing_window_cnt-1].wid;
}



//Epoch is a delayed work in a work queue;
static void sensing_epoch(struct work_struct *work);
static struct workqueue_struct *sensing_wq;
static DECLARE_DELAYED_WORK(sensing_delayed_work,sensing_epoch);

//set to true when the module is running so we know we can schedule new epochs
static volatile bool _sensing_running = false;

bool sensing_running(void) { return _sensing_running;}

static const int sensing_epoch_prefered_cpu_to_run = 0;

static inline void __sensing_epoch(int cpu)
{
	int i;
	sensing_window_ctrl_t *iter;

	//no windows, no sensing
	if(sensing_window_cnt <= 0) return;

	minimum_sensing_window(system_info());

	//decrease the time for all windows
	for(i = 0; i < sensing_window_cnt; ++i)
		sensing_windows[i].time_to_ready -= 1;

	//triggers all ready ones
	while(vitamins_list_head(next_window)->time_to_ready == 0){
		sensing_window_ctrl_t *w = vitamins_list_head(next_window);
		//reset period and update list
		w->time_to_ready = w->period;
		remove_from_list_default(next_window, w);
		add_to_priority_list_default(next_window,w,sw_order_crit,iter);

		sense_window(system_info(), w->wid);
		sensing_window_ready(w->wid);
	}
}

//starts a epoch EPOCH_LENGHT from the current time
static void _start_sensing_epoch(void);

static void sensing_epoch(struct work_struct *work){
    int cpu = smp_processor_id();
    smp_mb();
    if(_sensing_running) __sensing_epoch(cpu);
    smp_mb();
    if(_sensing_running) _start_sensing_epoch();
}

//starts a epoch EPOCH_LENGHT from the current time
static inline void _start_sensing_epoch(void){
    queue_delayed_work_on(sensing_epoch_prefered_cpu_to_run, sensing_wq, &sensing_delayed_work, MINIMUM_WINDOW_LENGHT_JIFFIES);
}

//stops the currently scheduled epoch
static inline void _vitamins_stop_epoch(void){
	cancel_delayed_work(&sensing_delayed_work);
}

void start_sensing_windows(){
	_sensing_running = true;
	sense_begin(system_info());
	smp_mb();
	_start_sensing_epoch();
}

void stop_sensing_windows(){
	_sensing_running = false;
	smp_mb();
	_vitamins_stop_epoch();
	flush_workqueue(sensing_wq);
	sense_stop(system_info());
}

bool create_queues(){
	clear_list(next_window);
	sensing_window_cnt = 0;
	sensing_wq = create_workqueue("sensing_epoch_workqueue");
	return sensing_wq != nullptr;
}

void destroy_queues(){
	destroy_workqueue(sensing_wq);
	clear_list(next_window);
	sensing_window_cnt = 0;
}
