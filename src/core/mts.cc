//#define HASDEBUG
#include "core.h"

define_vitamins_list(static model_core_t,free_cores[SIZE_COREARCH]);

define_vitamins_list(static model_task_t,no_qos_list);

static
void
map_tasks(model_task_t *list_head)
{
    int j;
    core_arch_t mapped_core_arch;
    model_core_t *tgtCore;
    model_task_t *currTask;

    for_each_in_list_head_default(list_head,currTask) {
        int64_t min_ips, max_ips;
        core_arch_t max_ips_core_type;

        min_ips = LONG_MAX;
        max_ips = 0;
        max_ips_core_type = SIZE_COREARCH;

        mapped_core_arch = SIZE_COREARCH;
        //mapped_core_freq = SIZE_COREFREQ;

        for (j = 0; j < SIZE_COREARCH; j++) {
            //			    pdebug("###core type %d has %d free cores\n", j, free_cores_type[j]);
            if (vitamins_list_head(free_cores[j]) != nullptr) {
                task_next_core_map(currTask,vitamins_list_head(free_cores[j]));
                if (task_max_total_ips(currTask) <= min_ips) {
                    min_ips = task_max_total_ips(currTask);
                }
                if (task_max_total_ips(currTask) >= max_ips) {
                    max_ips = task_max_total_ips(currTask);
                    max_ips_core_type = (core_arch_t)j;
                }
                task_next_core_unmap(currTask);
            }
        }

        BUG_ON(max_ips_core_type == SIZE_COREARCH);

        mapped_core_arch = max_ips_core_type;

        BUG_ON(mapped_core_arch == SIZE_COREARCH);

        // we found the type of core we want to map, so do the actual mapping below
        pdebug("###mapping task %d to core arch %d\n", currTask->id, mapped_core_arch);

        tgtCore = vitamins_list_head(free_cores[mapped_core_arch]);

        BUG_ON(tgtCore == nullptr);

        task_next_core_map(currTask,tgtCore);

        remove_from_list_default(free_cores[mapped_core_arch], tgtCore);


        if (vitamins_list_head(free_cores[mapped_core_arch]) == nullptr)
        	pdebug("###all cores of this type now occupied\n");

        // if we get here, there were no optimal mappings - what do we do? map suboptimally? don't map?
    }

}


static void calc_thoughput_boundness(model_task_t *task) {
    int core_type;
    task->max_ips_scaled = 0;
    for (core_type = 0; core_type < SIZE_COREARCH; ++core_type) {
        if(vitamins_list_head(free_cores[core_type]) == nullptr) continue;

        task_next_core_map(task,vitamins_list_head(free_cores[core_type]));

        if (task_max_total_ips(task) > task->max_ips_scaled)
            task->max_ips_scaled = task_max_total_ips(task);

        task_next_core_unmap(task);
    }
}

static void init_vitamins(model_sys_t *sys) {
	int ctr;

	for (ctr = 0; ctr < sys->task_list_size; ctr++) {
		clear_object_default(sys->task_list[ctr]);
	}

	for (ctr = 0; ctr < SIZE_COREARCH; ++ctr) clear_list(free_cores[ctr]);

	for (ctr = 0; ctr < sys->info->core_list_size; ctr++) {
		pdebug("###CORE %d ARCH %d\n", ctr, core_list[ctr].arch);

		add_to_list_default(free_cores[sys->info->core_list[ctr].arch], (sys->info->core_list[ctr].this_core));
	}

	clear_next_map(sys);

	clear_list(no_qos_list);
}

inline static bool crit_ips(model_task_t *a, model_task_t *b) {
    return (a->max_ips_scaled) >= (b->max_ips_scaled);
}


void vitamins_map_mts(model_sys_t *sys)
{

    int i;
    model_task_t *iter;

    vitamins_load_tracker_set(LT_CFS);
    init_vitamins(sys);

	for (i = 0; i < sys->task_list_size; i++) {
		calc_thoughput_boundness(sys->task_list[i]);
		add_to_priority_list_default(no_qos_list, sys->task_list[i],crit_ips,iter);
	}


	// basically free_core_list = core_list, and we are manipulating core_list in map_tasks()
	// assume core_list is a linked list
	BUG_ON(vitamins_list_head(no_qos_list) == nullptr);

    map_tasks(vitamins_list_head(no_qos_list));

}

