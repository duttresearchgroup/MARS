//#define HASDEBUG
#include "core.h"

define_vitamins_list(static model_core_t,cores[SIZE_COREARCH]);
static bool core_arch_present[SIZE_COREARCH];
static int COREARCH_COUNT;

define_vitamins_list(static model_task_t,pc_list);//partially constrained
define_vitamins_list(static model_task_t,fc_list);//fully constrained
define_vitamins_list(static model_task_t,nc_list);//no constraint


//correction stuff for aging
//it was generalized before thats why it looks like this
typedef uint32_t (*task_ips_ajust_f)(model_task_t *task, uint32_t uncorrected_ips , uint32_t limit);
typedef uint32_t (*core_power_ajust_f)(model_core_t *core, uint32_t uncorrected_power, uint32_t limit);
typedef uint32_t (*domain_power_ajust_f)(model_power_domain_t *domain, uint32_t uncorrected_power, uint32_t limit);
static uint32_t max_power_penalty = 0;
static uint32_t max_perf_penalty = 0;
static core_power_ajust_f core_power_penalty = nullptr;
static task_ips_ajust_f task_ips_penalty = nullptr;


static const uint32_t max_aging_power_penalty = CONV_DOUBLE_scaledUINT32(0.1);
static const uint32_t max_aging_perf_penalty = CONV_DOUBLE_scaledUINT32(0.1);


static inline uint32_t compute_penalty(model_core_t *core,uint32_t limit)
{
    //uint32_t penalty = core->aging_info.rel_delay_degrad_penalty * core->aging_info.rel_delay_degrad_penalty_weight;
    //penalty = CONV_scaledINTany_INTany(penalty);
    uint32_t penalty = core->aging_info.rel_delay_degrad_penalty;
    penalty = (penalty < limit) ? limit : penalty;
    return penalty;
}

static inline uint32_t task_aging_ips_penalty(model_task_t *task, uint32_t ips, uint32_t limit)
{
    uint32_t penalty = compute_penalty(task_next_core(task),max_aging_power_penalty);
    uint32_t new_ips = CONV_scaledINTany_INTany(ips*penalty);//remove penalty scaling from resulting IPS
    BUG_ON(new_ips > ips);//can't happned, so overflow
    return new_ips;
}

static inline uint32_t core_aging_power_penalty(model_core_t *core, uint32_t power, uint32_t limit)
{
    uint32_t penalty = compute_penalty(core,max_aging_perf_penalty);
    //scale-up power againg for div
    uint32_t new_power = CONV_INTany_scaledINTany(power)/penalty;
    BUG_ON(new_power < power);//can't happen, so overflow
    return new_power;
}

static inline
uint32_t
corrected_core_total_power(model_core_t *core, core_power_ajust_f correction, uint32_t correction_limit)
{
    uint32_t power = core_total_power(core);
    return (correction)(core,power,correction_limit);
}

static inline
uint32_t
calc_core_power(model_core_t *core)
{
    if(core_power_penalty == nullptr)
    	return core_total_power(core);
    else
    	return corrected_core_total_power(core,core_power_penalty,max_power_penalty);
}

static inline
uint32_t
corrected_task_total_ips(model_task_t *task, task_ips_ajust_f correction, uint32_t correction_limit)
{
    uint32_t ips = task_total_ips(task);
    return (correction)(task,ips,correction_limit);
}

static inline
uint32_t
calc_task_ips(model_task_t *task)
{
    if(task_ips_penalty == nullptr)
    	return task_total_ips(task);
    else
    	return corrected_task_total_ips(task,task_ips_penalty,max_perf_penalty);
}

static inline
uint32_t
corrected_system_total_ips(model_sys_t *sys, task_ips_ajust_f per_task_correction, uint32_t correction_limit)
{
    int core;
    model_core_t *currCore;
    model_task_t *currTask;
    uint32_t totalIPS = 0;
    for (core = 0; core < sys->info->core_list_size; ++core){
    	currCore = sys->info->core_list[core].this_core;
    	for_each_in_internal_list(currCore,mapped_tasks,currTask,mapping){
    		totalIPS += corrected_task_total_ips(currTask,per_task_correction,correction_limit);
    	}
    }
    return totalIPS;
}

static inline
uint32_t
corrected_system_total_power(model_sys_t *sys, core_power_ajust_f per_core_correction, uint32_t correction_limit)
{
    int domain;
    core_info_t *currCore;
    uint32_t totalPower = 0;
    for(domain = 0; domain < sys->info->power_domain_list_size; ++domain){
    	for_each_in_internal_list(&(sys->info->power_domain_list[domain]),cores,currCore,power_domain){
    		totalPower += corrected_core_total_power(currCore->this_core,per_core_correction,correction_limit);
    	}
    }
    return totalPower;
}

static uint32_t total_system_ips(model_sys_t *sys)
{
    if(task_ips_penalty==nullptr)
    	return system_total_ips(sys);
    else
    	return corrected_system_total_ips(sys,task_ips_penalty,max_perf_penalty);
}

static uint32_t total_system_power(model_sys_t *sys)
{
	if(core_power_penalty == nullptr)
		return system_total_power(sys);
	else
		return corrected_system_total_power(sys,core_power_penalty,max_power_penalty);
}

static
model_core_t*
search_core_perf(model_task_t *currTask, model_sys_t *sys)
{
    int arch;

    uint32_t max_ips_all = 0;
    model_core_t* max_ips_core_all = nullptr;

    uint32_t ips;

    model_core_t *currCore = nullptr;

    for (arch = 0; arch < SIZE_COREARCH; arch++) {

        if(!core_arch_present[arch]) continue;

        BUG_ON(vitamins_list_head(cores[arch]) == nullptr);
        for_each_in_list_default(cores[arch],currCore){

            //temporaly map
            task_next_core_map(currTask,currCore);
            ips = total_system_ips(sys);
            if(ips == 0) ips = 1;

            if (ips > max_ips_all) {
                max_ips_all = ips;
                max_ips_core_all = currCore;
            }

            //unmap
            task_next_core_unmap(currTask);

        }
    }

    BUG_ON(max_ips_core_all == nullptr);

    //return max_ips_core_unloaded != nullptr ? max_ips_core_unloaded : max_ips_core_all;
    return max_ips_core_all;
}

static
model_core_t*
search_core_qos_energy(model_task_t *currTask, model_sys_t *sys)
{
    int arch;

    uint32_t ips;
    uint32_t power;
#ifdef HASDEBUG
    uint32_t power_before;
    uint32_t load_before;
#endif
    uint32_t ipsPerSysPower;

    //uint32_t max_ipsPerSysPower = 0;
    //vitamins_core_t *max_ipsPerSysPower_core = nullptr;
    uint32_t max_ips = 0;
    model_core_t *max_ips_core = nullptr;

    uint32_t max_ipsPerSysPower_qos = 0;
    model_core_t *max_ipsPerSysPower_qos_core = nullptr;

    model_core_t *currCore = nullptr;

    for (arch = 0; arch < SIZE_COREARCH; arch++){

        if(!core_arch_present[arch]) continue;

        BUG_ON(vitamins_list_head(cores[arch]) == nullptr);
        for_each_in_list_default(cores[arch],currCore){
            //temporaly map
#ifdef HASDEBUG
        	power_before = total_system_power();
        	load_before = currCore->load_tracking.common.load;
#endif
            task_next_core_map(currTask,currCore);
            ips = calc_task_ips(currTask);
            power = total_system_power(sys);
            ipsPerSysPower = CONV_INTany_scaledINTany(ips) / power;
            if(ipsPerSysPower == 0) ipsPerSysPower = 1;
            if(ips == 0) ips = 1;

            //if (ipsPerSysPower > max_ipsPerSysPower) {
            //    max_ipsPerSysPower = ipsPerSysPower;
            //    max_ipsPerSysPower_core = currCore;
            //}
            if (ips > max_ips) {
            	max_ips = ips;
            	max_ips_core = currCore;
            }
            if ((ipsPerSysPower > max_ipsPerSysPower_qos) && (currCore->load_tracking.common.load < CONV_DOUBLE_scaledUINT32(1.0))) {
                max_ipsPerSysPower_qos = ipsPerSysPower;
                max_ipsPerSysPower_qos_core = currCore;
            }

            pdebug("###\t\ttest task %d on core %d arch %d load=%u freq=%u ips=%u power=%u ipsPerSysPower=%u load_before=%u power_before=%u\n", currTask->id, currCore->position, currCore->arch,currCore->load_tracking.common.load,freqToValMHz_i(currCore->freq->last_pred_freq),ips,power,ipsPerSysPower,load_before,power_before);

            //unmap
            task_next_core_unmap(currTask);

        }
    }

    //BUG_ON(max_ipsPerSysPower_core == nullptr);
    BUG_ON(max_ips_core == nullptr);

    //return (max_ipsPerSysPower_qos_core != nullptr) ? max_ipsPerSysPower_qos_core : max_ipsPerSysPower_core;
    return (max_ipsPerSysPower_qos_core != nullptr) ? max_ipsPerSysPower_qos_core : max_ips_core;

}

typedef enum {
    MAP_PERF,
    MAP_ENERGY
} mapping_t;

inline static bool crit_core_load(model_core_t *a, model_core_t *b) {
    return (a->load_tracking.common.load) <= (b->load_tracking.common.load);
}

static void map_one_task(model_task_t *currTask, mapping_t map, model_sys_t *sys) {
    model_core_t *tgtCore,*iter;

    BUG_ON(task_next_core(currTask) != nullptr);

    if (map == MAP_PERF) {
        tgtCore = search_core_perf(currTask,sys);
    }
    else if(map == MAP_ENERGY) {
        tgtCore = search_core_qos_energy(currTask,sys);
    }
    else tgtCore = nullptr;

    BUG_ON(tgtCore == nullptr);

    // we found the type of core we want to map, so do the actual mapping below
    pdebug("###\tmapping task %d to core %d arch %d\n", currTask->id, tgtCore->position, tgtCore->arch);

    task_next_core_map(currTask,tgtCore);

    //pdebug("###successfully mapped task %d to core %d\n", currTask->id, currTask->_next_mapping__->position);

    //remove and re-add in order
    remove_from_list_default(cores[tgtCore->info->arch], tgtCore);
    add_to_priority_list_default(cores[tgtCore->info->arch],tgtCore,crit_core_load,iter);

}


static void init_sparta(model_sys_t *sys) {
	int ctr;
	model_core_t *iter;

	vitamins_load_tracker_set(LT_CFS);

	for (ctr = 0; ctr < SIZE_COREARCH; ++ctr) {
	    clear_list(cores[ctr]);
	    core_arch_present[ctr] = false;
	}
	COREARCH_COUNT = 0;

	clear_next_map(sys);

	for (ctr = 0; ctr < sys->info->core_list_size; ctr++) {
		//pdebug("###CORE %d ARCH %d\n", ctr, core_list[ctr].arch);

		clear_object_default(sys->info->core_list[ctr].this_core);

		add_to_priority_list_default(cores[sys->info->core_list[ctr].arch], (sys->info->core_list[ctr].this_core),crit_core_load,iter);

		core_arch_present[sys->info->core_list[ctr].arch] = true;
	}

    for (ctr = 0; ctr < SIZE_COREARCH; ++ctr) {
        if(core_arch_present[ctr] == true) ++COREARCH_COUNT;
    }

	clear_list(pc_list);
	clear_list(nc_list);
	clear_list(fc_list);
}

inline static bool crit_ips(model_task_t *a, model_task_t *b) {
    return (a->max_ips_scaled) >= (b->max_ips_scaled);
}

inline static bool crit_ipswatt(model_task_t *a, model_task_t *b) {
    return (a->max_ips_watt_scaled) >= (b->max_ips_watt_scaled);
}

//more contrained goes first
inline static bool crit_ips_constr(model_task_t *a, model_task_t *b) {
    return (a->max_ips_scaled / a->constraining) >= (b->max_ips_scaled / b->constraining);
}
inline static bool crit_ipswatt_constr(model_task_t *a, model_task_t *b) {
    return (a->max_ips_watt_scaled / a->constraining) >= (b->max_ips_watt_scaled / b->constraining);
}


static const uint32_t MAX_IPS_THR = CONV_DOUBLE_scaledUINT32(0.95);

static
void
sparta_qos_finder(model_task_t *task)
{
    int j;
    uint32_t ips,power;
    uint32_t ips_watt;
    task->max_ips_scaled = 0;
    task->max_ips_watt_scaled = 0;
    task->max_ips_core_type = SIZE_COREARCH;
    task->max_ips_watt_core_type = SIZE_COREARCH;
    task->constraining = 0;

    for (j = 0; j < SIZE_COREARCH; j++) {
        if(!core_arch_present[j]) continue;

        task_next_core_map(task,vitamins_list_head(cores[j]));
        ips = calc_task_ips(task);
        power = calc_core_power(task_next_core(task));
        //pdebug("task %d ips=%u load=%u at core %d type %d\n",task->id, ips, vitamins_load_tracker_task_load(LT_CFS,task), vitamins_list_head(cores[j])->position,j);

        if (ips >= task->max_ips_scaled) {
            task->max_ips_scaled = ips;
            task->max_ips_core_type = (core_arch_t)j;
        }
        ips_watt = CONV_INTany_scaledINTany(ips) / power;
        if (ips_watt >= task->max_ips_watt_scaled) {
            task->max_ips_watt_scaled = ips_watt;
            task->max_ips_watt_core_type = (core_arch_t)j;
        }

        task_next_core_unmap(task);
    }
    BUG_ON(task->max_ips_core_type == SIZE_COREARCH);
    BUG_ON(task->max_ips_watt_core_type == SIZE_COREARCH);

    task->max_ips_sat_scaled = task->max_ips_scaled * MAX_IPS_THR;
    //scale-up the max_ips
    task->max_ips_scaled = CONV_INTany_scaledINTany(task->max_ips_scaled);

    if(task->max_ips_scaled == 0){//may happen for very low util
        task->max_ips_scaled = 2;
        task->max_ips_sat_scaled = 1;
        task->constraining = 1;
    }
    else{
        BUG_ON(task->max_ips_sat_scaled >= task->max_ips_scaled);

        for (j = 0; j < SIZE_COREARCH; j++) {
            if(!core_arch_present[j]) continue;

            task_next_core_map(task,vitamins_list_head(cores[j]));
            //ips = calc_task_ips(task);
            //ips = CONV_INTany_scaledINTany(ips);
            if(vitamins_list_head(cores[j])->load_tracking.common.load < CONV_DOUBLE_scaledUINT32(1.0))
            	task->constraining += 1;
            task_next_core_unmap(task);

            //if (ips >= task->max_ips_sat_scaled) {
            //    task->constraining += 1;
            //}
        }
        //TODO debug this later !
        //BUG_ON(task->constraining == 0);
        //if(task->constraining == 0){
        //	task->constraining = 1;
        //}
    }

    //pdebug("task %d sat_ips=%u const=%d\n",task->id, task->max_ips_sat_scaled,task->constraining);
}

static
void
gen_qos_lists(model_task_t **task_list, int task_list_size)
{
    int i;
    model_task_t *iter;

    clear_list(pc_list);
    clear_list(nc_list);
    clear_list(fc_list);

    for (i = 0; i < task_list_size; i++) {
        if(task_next_core(task_list[i]) != nullptr) continue;

        clear_object_default(task_list[i]);

        sparta_qos_finder(task_list[i]);

        if(task_list[i]->constraining == COREARCH_COUNT)//no constraint
            add_to_priority_list_default(nc_list, task_list[i], crit_ipswatt, iter);
        else if(task_list[i]->constraining == 0)//fully constrained
            add_to_priority_list_default(fc_list, task_list[i], crit_ips, iter);
        else//partially constrained
            add_to_priority_list_default(pc_list, task_list[i], crit_ipswatt_constr, iter);
    }

    /*pdebug("###post sort nc_list \n");
    print_list(vitamins_list_head(nc_list));

    pdebug("###post sort fc_list \n");
    print_list(vitamins_list_head(fc_list));

    pdebug("###post sort pc_list \n");
    print_list(vitamins_list_head(pc_list));*/
}

static void
_vitamins_sparta_map(model_sys_t *sys)
{
    int i;
    int core;

    init_sparta(sys);

    //qos
    gen_qos_lists(sys->task_list,sys->task_list_size);

    //map
    while(true){
        if (vitamins_list_head(fc_list) != nullptr) {
            map_one_task(vitamins_list_head(fc_list),MAP_PERF,sys);
            gen_qos_lists(sys->task_list,sys->task_list_size);
        }
        else if (vitamins_list_head(pc_list) != nullptr) {
            map_one_task(vitamins_list_head(pc_list),MAP_ENERGY,sys);
            gen_qos_lists(sys->task_list,sys->task_list_size);
        }
        else if (vitamins_list_head(nc_list) != nullptr) {
            map_one_task(vitamins_list_head(nc_list),MAP_ENERGY,sys);
            gen_qos_lists(sys->task_list,sys->task_list_size);
        }
        else
            break;
    }


    //checks
    for (i = 0; i < sys->task_list_size; i++)
        BUG_ON(task_next_core(sys->task_list[i]) == nullptr);

    for (core = 0; core < sys->info->core_list_size; core++){
        bool empty = true;
        for (i = 0; i < sys->task_list_size; i++){
            BUG_ON(task_next_core(sys->task_list[i]) == nullptr);
            if(task_next_core_idx(sys->task_list[i]) == core){
                BUG_ON(sys->info->core_list[core].this_core->load_tracking.common.load <= 0);
                BUG_ON(sys->info->core_list[core].this_core->load_tracking.common.load > CONV_INTany_scaledINTany(1));
                empty = false;
            }
        }
        BUG_ON(!empty && (sys->info->core_list[core].this_core->load_tracking.common.load == 0));
    }
}

void
vitamins_sparta_map(model_sys_t *sys)
{
	max_power_penalty = 0;
	max_perf_penalty = 0;
	core_power_penalty = nullptr;
	task_ips_penalty = nullptr;

	_vitamins_sparta_map(sys);
}

void
vitamins_sparta_agingaware_map(model_sys_t *sys)
{
	max_power_penalty = max_aging_power_penalty;
	max_perf_penalty = max_aging_perf_penalty;
	core_power_penalty = core_aging_power_penalty;
	task_ips_penalty = task_aging_ips_penalty;

	_vitamins_sparta_map(sys);
}
