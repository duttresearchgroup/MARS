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
    int period;//In terms of MINIMUM_WINDOW_LENGTH
    int period_ms;
    int time_to_ready;//In terms of MINIMUM_WINDOW_LENGTH
    //Link for adding this to the list of sensing windows
    define_list_addable_default(struct sensing_window_ctrl_struct);
};
typedef struct sensing_window_ctrl_struct sensing_window_ctrl_t;

static sensing_window_ctrl_t sensing_windows[MAX_WINDOW_CNT];

//list of sensing windows to keep track of the ones ready to execute
define_vitamins_list(static sensing_window_ctrl_t,next_window);
//list is ordered by time_to_ready
inline static bool sw_order_crit(sensing_window_ctrl_t *a, sensing_window_ctrl_t *b) {
    return (a->time_to_ready == b->time_to_ready) ?
            (a->period < b->period) : (a->time_to_ready < b->time_to_ready);
}


int create_sensing_window(int period_ms)
{
    int i;
    sensing_window_ctrl_t *iter;
    int period = period_ms / MINIMUM_WINDOW_LENGTH_MS;

    if((period <= 0) || ((period_ms % MINIMUM_WINDOW_LENGTH_MS)!=0))
        return WINDOW_INVALID_PERIOD;

    if(vitsdata->sensing_window_cnt >= MAX_WINDOW_CNT)
        return WINDOW_MAX_NWINDOW;

    for(i = 0; i < vitsdata->sensing_window_cnt; ++i)
        if(sensing_windows[i].period == period)
            return WINDOW_EXISTS;

    sensing_windows[vitsdata->sensing_window_cnt].wid = vitsdata->sensing_window_cnt;
    sensing_windows[vitsdata->sensing_window_cnt].period = period;
    sensing_windows[vitsdata->sensing_window_cnt].period_ms = period_ms;
    sensing_windows[vitsdata->sensing_window_cnt].time_to_ready = period;
    clear_object_default(&(sensing_windows[vitsdata->sensing_window_cnt]));
    add_to_priority_list_default(next_window,
            &(sensing_windows[vitsdata->sensing_window_cnt]),sw_order_crit,iter);
    ++(vitsdata->sensing_window_cnt);

    return sensing_windows[vitsdata->sensing_window_cnt - 1].wid;
}



//Epoch is a delayed work in a work queue;
static void sensing_epoch(struct work_struct *work);
static struct workqueue_struct *sensing_wq;
static DECLARE_WORK(sensing_work,sensing_epoch);

//set to true when the module is running so we know we can schedule new epochs
static volatile bool _sensing_running = false;

bool sensing_running(void) { return _sensing_running;}

static inline void __sensing_epoch(int cpu)
{
    int i;
    sensing_window_ctrl_t *iter;

    //no windows, no sensing
    if(vitsdata->sensing_window_cnt <= 0) return;

    minimum_sensing_window(system_info());

    //decrease the time for all windows
    for(i = 0; i < vitsdata->sensing_window_cnt; ++i)
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

static struct hrtimer sensing_epoch_timer;
static ktime_t sensing_epoch_period;
//starts a epoch EPOCH_LENGTH from the current time
static void _start_sensing_epoch(void);

static enum hrtimer_restart sensing_epoch_handler(struct hrtimer *data) {
    queue_work(sensing_wq, &sensing_work);
    return HRTIMER_NORESTART;
}

static void sensing_epoch(struct work_struct *work){
    int cpu = smp_processor_id();
    smp_mb();
    if(_sensing_running) __sensing_epoch(cpu);
    smp_mb();
    if(_sensing_running) {
        hrtimer_start(&sensing_epoch_timer, sensing_epoch_period, HRTIMER_MODE_REL);
    }
}

//starts a epoch EPOCH_LENGTH from the current time
static inline void _start_sensing_epoch(void){
    hrtimer_init(&sensing_epoch_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    sensing_epoch_timer.function = &sensing_epoch_handler;
    sensing_epoch_period = ns_to_ktime(MINIMUM_WINDOW_LENGTH_MS*1e6);
    hrtimer_start(&sensing_epoch_timer, sensing_epoch_period,
            HRTIMER_MODE_REL);
}

//stops the currently scheduled epoch
static inline void _vitamins_stop_epoch(void){
    hrtimer_try_to_cancel(&sensing_epoch_timer);
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
    sensing_wq = create_workqueue("sensing_epoch_workqueue");
    return sensing_wq != nullptr;
}

void destroy_queues(){
    destroy_workqueue(sensing_wq);
    clear_list(next_window);
}
