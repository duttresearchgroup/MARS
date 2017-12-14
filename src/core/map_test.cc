#define HASDEBUG
#include "core.h"

static int map_count = 0;

void
vitamins_odroid_test_map(model_sys_t *sys)
{
    int i;
    //vitamins_task_t *iter;
    int task;
    int corei;
    bool empty;
    model_core_t *core;
    uint32_t ips = 0;
    uint32_t power = 0;
    uint32_t core_ips,core_power;
    uint32_t core_idle;
	int ctr;

	vitamins_load_tracker_set(LT_CFS);

	for (ctr = 0; ctr < sys->task_list_size; ctr++) {
		clear_object_default(sys->task_list[ctr]);
	}
	clear_next_map(sys);


	for (i = 0; i < sys->task_list_size; i++) {
		task_next_core_map(sys->task_list[i],sys->info->core_list[map_count % sys->info->core_list_size].this_core);
	}
	map_count++;

	pdebug("VITAMINS map result:\n");
    for(corei = 0; corei < sys->info->core_list_size; ++corei){
        core = sys->info->core_list[corei].this_core;
        core_ips = 0;
        core_power = 0;
        empty = true;
        for(task = 0; task < sys->task_list_size; ++task)
            if(task_next_core_idx(sys->task_list[task]) == corei){
                core_ips += task_total_ips(sys->task_list[task]);
                empty = false;
            }
        core_power = core_total_power(core);
        //checks
        core_idle = CONV_INTany_scaledINTany(1) - core->load_tracking.common.load;
        BUG_ON(empty && (core_idle != CONV_INTany_scaledINTany(1)));
        BUG_ON(!empty && (core_idle == CONV_INTany_scaledINTany(1)));

        ips += core_ips;
        power += core_power;
        pdebug("VITAMINS\t core %d ips=%u load=%u power=%u freq=%u\n",corei,core_ips,core->load_tracking.common.load,core_power,freqToValMHz_i(core->info->freq->this_domain->last_pred_freq));
    }
    BUG_ON(power==0);

    pdebug("VITAMINS\t total_ips=%u total_power=%u\n",ips,power);
}


