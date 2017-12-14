#define HASDEBUG
#include "core.h"

typedef enum {
    HAS_NEXT_LAYER=0,
    HAS_FINAL_RESULT=1,
} layer_type ;

typedef struct {
    uint32_t srcArch;
    uint32_t srcFreq;
    uint32_t tgtArch;
    uint32_t tgtFreq;
    uint32_t numFreqs;
} file_header_t;

static uint32_t checkSum(file_header_t* h){
    return h->srcArch + h->srcFreq + h->tgtArch  + h->tgtFreq + h->numFreqs;
}

static int plevel = 0;

static void write_layer(vfile_t *file, bin_pred_layer_t *layer)
{
    int i;
    file_wr_word(file,vitamins_get_bin_pred_func_id(layer->metric));
    file_wr_word(file,layer->num_of_bins);
    //file_wr(file,(const char*)(layer->bins),layer->num_of_bins*sizeof(uint32_t));
    for(i = 0; i < layer->num_of_bins; ++i) file_wr_word(file,layer->bins[i]);
    if(layer->bin_result != nullptr){
        BUG_ON(layer->next_layer != nullptr);
        file_wr_word(file,HAS_FINAL_RESULT);
        for(i = 0; i < layer->num_of_bins; ++i){
            file_wr_word(file,layer->bin_result[i].ipcActive);
            file_wr_word(file,layer->bin_result[i].powerActive);
        }
    }
    else{
        file_wr_word(file,HAS_NEXT_LAYER);
        BUG_ON(layer->next_layer == nullptr);
    }

    //for(i = 0; i < plevel; ++i){
    //    pdebug("\t");
    //}
    //pdebug("Layer %d %d %d\n",layer->num_of_bins,(layer->bin_result==nullptr)?0:1,(layer->next_layer==nullptr)?0:1);
}

static void read_layer(vfile_t *file, bin_pred_layer_t *layer)
{
    int i;
    layer_type type;

    layer->metric = vitamins_get_bin_pred_func((bin_pred_func_id)file_rd_word(file));
    layer->num_of_bins = file_rd_word(file);
    layer->bins = new uint32_t[layer->num_of_bins];
    //file_rd(file,(char*)(layer->bins),layer->num_of_bins*sizeof(uint32_t));
    for(i = 0; i < layer->num_of_bins; ++i) layer->bins[i] = file_rd_word(file);
    type = (layer_type)file_rd_word(file);
    if(type == HAS_FINAL_RESULT){
        layer->next_layer = nullptr;
        layer->bin_result = new bin_pred_result_t[layer->num_of_bins];
        for(i = 0; i < layer->num_of_bins; ++i){
            layer->bin_result[i].ipcActive = file_rd_word(file);
            layer->bin_result[i].powerActive = file_rd_word(file);
        }
    }
    else{
        BUG_ON(type != HAS_NEXT_LAYER);
        layer->next_layer = new bin_pred_layer_t*[layer->num_of_bins];
        layer->bin_result = nullptr;
        for(i = 0; i < layer->num_of_bins; ++i){
            layer->next_layer[i] = nullptr;
        }
    }
    //for(i = 0; i < plevel; ++i){
    //    pdebug("\t");
    //}
    //pdebug("Layer %d %d %d\n",layer->num_of_bins,(layer->bin_result==nullptr)?0:1,(layer->next_layer==nullptr)?0:1);
}

static void check_layer(bin_pred_layer_t *layer, bin_pred_layer_t *verify)
{
    int i;
    BUG_ON(layer->metric != verify->metric);
    BUG_ON(layer->num_of_bins != verify->num_of_bins);
    for(i = 0; i < layer->num_of_bins; ++i){
        BUG_ON(layer->bins[i] != verify->bins[i]);
    }
    if(layer->bin_result != nullptr){
        BUG_ON(layer->next_layer != nullptr);
        BUG_ON(verify->next_layer != nullptr);
        BUG_ON(verify->bin_result == nullptr);
        for(i = 0; i < layer->num_of_bins; ++i){
            BUG_ON(layer->bin_result[i].ipcActive != verify->bin_result[i].ipcActive);
            BUG_ON(layer->bin_result[i].powerActive != verify->bin_result[i].powerActive);
        }
    }
    else{
        BUG_ON(layer->next_layer == nullptr);
        BUG_ON(verify->next_layer == nullptr);
    }
}

static void write_predictor_aux(vfile_t *file, bin_pred_layer_t *layer)
{
    int i;

    plevel += 1;

    write_layer(file,layer);

    if(layer->next_layer != nullptr){
        BUG_ON(layer->bin_result != nullptr);
        for(i = 0; i < layer->num_of_bins; ++i)
            write_predictor_aux(file,layer->next_layer[i]);
    }

    plevel -= 1;
}

static void read_predictor_aux(vfile_t *file, bin_pred_layer_t *layer)
{
    int i;

    plevel += 1;

    read_layer(file,layer);

    if(layer->next_layer != nullptr){
        BUG_ON(layer->bin_result != nullptr);
        for(i = 0; i < layer->num_of_bins; ++i){
            BUG_ON(layer->next_layer[i] != nullptr);
            layer->next_layer[i] = new bin_pred_layer_t;
            read_predictor_aux(file,layer->next_layer[i]);
        }
    }

    plevel -= 1;
}

static void check_predictor_aux(bin_pred_layer_t *layer, bin_pred_layer_t *verify)
{
    int i;

    check_layer(layer,verify);

    if(layer->next_layer != nullptr){
        BUG_ON(verify->next_layer == nullptr);
        BUG_ON(layer->bin_result != nullptr);
        BUG_ON(verify->bin_result != nullptr);

        for(i = 0; i < layer->num_of_bins; ++i)
            check_predictor_aux(layer->next_layer[i], verify->next_layer[i]);
    }
}

void vitamins_bin_predictor_writefile(const char* filepath, bin_pred_ptr_t pred)
{
    uint32_t srcCore,srcFreq,tgtCore,tgtFreq,count;

    vfile_t file = open_file_wr(filepath);

    plevel = 0;

    count = 0;
    for(srcCore = 0; srcCore < SIZE_COREARCH; ++srcCore)
    	for(srcFreq = 0; srcFreq < SIZE_COREFREQ; ++srcFreq)
    		for(tgtCore = 0; tgtCore < SIZE_COREARCH; ++tgtCore)
    			for(tgtFreq = 0; tgtFreq < SIZE_COREFREQ; ++tgtFreq)
    				if(pred[srcCore][srcFreq][tgtCore][tgtFreq] != nullptr)
    					++count;


    file_wr_word(&file,count);

    for(srcCore = 0; srcCore < SIZE_COREARCH; ++srcCore){
    	for(srcFreq = 0; srcFreq < SIZE_COREFREQ; ++srcFreq){
    		for(tgtCore = 0; tgtCore < SIZE_COREARCH; ++tgtCore){
    			for(tgtFreq = 0; tgtFreq < SIZE_COREFREQ; ++tgtFreq){
    				if(pred[srcCore][srcFreq][tgtCore][tgtFreq] != nullptr){
    					file_header_t h = {srcCore,srcFreq,tgtCore,tgtFreq,SIZE_COREFREQ};
    					file_wr_word(&file,h.srcArch);
    					file_wr_word(&file,h.srcFreq);
    					file_wr_word(&file,h.tgtArch);
    					file_wr_word(&file,h.tgtFreq);
    					file_wr_word(&file,h.numFreqs);
    					file_wr_word(&file,checkSum(&h));
    					//pdebug("Wrote %d %d %d %d / %u %u %u %u \n",srcCore,srcFreq,tgtCore,tgtFreq,h.srcArch,h.srcFreq,h.tgtArch,h.tgtFreq);
    					write_predictor_aux(&file,pred[srcCore][srcFreq][tgtCore][tgtFreq]);
    				}
    			}
    		}
    	}
    }

    close_file(&file);
}


static inline void add_level(int level){
	int i;
	for(i = 0; i < level; ++i) pdebug("  ");
}

static void print_layer(bin_pred_layer_t *layer, int level)
{
	int i;
	for(i = 0; i < layer->num_of_bins; ++i){

		add_level(level);
		pdebug("bin[%d]: %s < %u\n",i,vitamins_get_bin_pred_func_name(vitamins_get_bin_pred_func_id(layer->metric)),layer->bins[i]);

		if(layer->next_layer != nullptr){
			print_layer(layer->next_layer[i],level+1);
		}
		else{
			add_level(level+1);
			pdebug("result: ipcActive=%u\n",layer->bin_result[i].ipcActive);
			add_level(level+1);
			pdebug("result: powerActive=%u\n",layer->bin_result[i].powerActive);
		}
	}


}

static void print_predictor(bin_pred_ptr_t predictors){
	int srcCore,srcFreq,tgtCore,tgtFreq;
	for(srcCore = 0; srcCore < SIZE_COREARCH; ++srcCore){
		for(srcFreq = 0; srcFreq < SIZE_COREFREQ; ++srcFreq){
			for(tgtCore = 0; tgtCore < SIZE_COREARCH; ++tgtCore){
				for(tgtFreq = 0; tgtFreq < SIZE_COREFREQ; ++tgtFreq){
					if(predictors[srcCore][srcFreq][tgtCore][tgtFreq] != nullptr){
						pdebug("Pred %s_%s -> %s_%s\n",archToString((core_arch_t)srcCore),freqToString((core_freq_t)srcFreq),archToString((core_arch_t)tgtCore),freqToString((core_freq_t)tgtFreq));
						print_layer(predictors[srcCore][srcFreq][tgtCore][tgtFreq],1);
					}
				}
			}
		}
	}
}


void vitamins_bin_predictor_readfile(const char* filepath, bin_pred_ptr_t pred, bool print, bin_pred_ptr_t verify)
{
    int srcCore,srcFreq,tgtCore,tgtFreq,count,i;

    vfile_t file = open_file_rd(filepath);

    plevel = 0;

    count = file_rd_word(&file);

    for(i = 0; i < count; ++i){
        file_header_t h;
        uint64_t sum;
        bin_pred_layer_t *layer;
        h.srcArch = file_rd_word(&file);
        h.srcFreq = file_rd_word(&file);
        h.tgtArch = file_rd_word(&file);
        h.tgtFreq = file_rd_word(&file);
        h.numFreqs = file_rd_word(&file);
        //pdebug("Read %u %u %u %u \n",h.srcArch,h.srcFreq,h.tgtArch,h.tgtFreq);
        sum = file_rd_word(&file);
        BUG_ON(sum != checkSum(&h));
        BUG_ON(pred[h.srcArch][h.srcFreq][h.tgtArch][h.tgtFreq] != nullptr);

        layer = new bin_pred_layer_t;
        read_predictor_aux(&file,layer);
        pred[h.srcArch][h.srcFreq][h.tgtArch][h.tgtFreq] = layer;
    }

    close_file(&file);

    if(print) print_predictor(pred);

    if(verify != nullptr){
    	for(srcCore = 0; srcCore < SIZE_COREARCH; ++srcCore){
    		for(srcFreq = 0; srcFreq < SIZE_COREFREQ; ++srcFreq){
    			for(tgtCore = 0; tgtCore < SIZE_COREARCH; ++tgtCore){
    				for(tgtFreq = 0; tgtFreq < SIZE_COREFREQ; ++tgtFreq){
    					if(pred[srcCore][srcFreq][tgtCore][tgtFreq] != nullptr){
    						BUG_ON(verify[srcCore][srcFreq][tgtCore][tgtFreq] == nullptr);
    						check_predictor_aux(pred[srcCore][srcFreq][tgtCore][tgtFreq],verify[srcCore][srcFreq][tgtCore][tgtFreq]);
    					}
    				}
    			}
    		}
    	}
    }
}

void vitamins_bin_predictor_task_error_reset()
{
    all_taks_error_acc.error_ips_active.pred_error_acc_s = 0;
    all_taks_error_acc.error_ips_active.pred_error_acc_u = 0;
    all_taks_error_acc.error_ips_active.pred_error_cnt = 0;
    all_taks_error_acc.error_load.pred_error_acc_s = 0;
    all_taks_error_acc.error_load.pred_error_acc_u = 0;
    all_taks_error_acc.error_load.pred_error_cnt = 0;
}

uint32_t vitamins_bin_predictor_avg_task_ips_error()
{
	if(all_taks_error_acc.error_ips_active.pred_error_cnt == 0)
		return 0;
	else
		return all_taks_error_acc.error_ips_active.pred_error_acc_u / all_taks_error_acc.error_ips_active.pred_error_cnt;
}
uint32_t vitamins_bin_predictor_avg_task_load_error()
{
	if(all_taks_error_acc.error_load.pred_error_cnt == 0)
		return 0;
	else
		return all_taks_error_acc.error_load.pred_error_acc_u / all_taks_error_acc.error_load.pred_error_cnt;
}
uint32_t vitamins_bin_predictor_avg_core_load_error(model_sys_t *sys)
{
	uint32_t err_acc = 0;
	uint32_t err_cnt = 0;
	int i = 0;
	for(i = 0; i < sys->info->core_list_size; ++i){
		if(sys->info->core_list[i].this_core->pred_checker.error_load.pred_error_cnt==0) continue;
		err_acc += sys->info->core_list[i].this_core->pred_checker.error_load.pred_error_acc_u / sys->info->core_list[i].this_core->pred_checker.error_load.pred_error_cnt;
		err_cnt += 1;
	}
	if(err_cnt==0) return 0;
	else		   return err_acc / err_cnt;
}
uint32_t vitamins_bin_predictor_avg_freq_error(model_sys_t *sys)
{
	uint32_t err_acc = 0;
	uint32_t err_cnt = 0;
	int i = 0;
	for(i = 0; i < sys->info->freq_domain_list_size; ++i){
		if(sys->info->freq_domain_list[i].this_domain->pred_checker.error_freq.pred_error_cnt==0) continue;
		err_acc += sys->info->freq_domain_list[i].this_domain->pred_checker.error_freq.pred_error_acc_u / sys->info->freq_domain_list[i].this_domain->pred_checker.error_freq.pred_error_cnt;
		err_cnt += 1;
	}
	if(err_cnt==0) return 0;
	else		   return err_acc / err_cnt;
}
uint32_t vitamins_bin_predictor_avg_power_error(model_sys_t *sys)
{
	uint32_t err_acc = 0;
	uint32_t err_cnt = 0;
	int i = 0;
	for(i = 0; i < sys->info->power_domain_list_size; ++i){
		if(sys->info->power_domain_list[i].this_domain->pred_checker.error_power.pred_error_cnt==0) continue;
		err_acc += sys->info->power_domain_list[i].this_domain->pred_checker.error_power.pred_error_acc_u / sys->info->power_domain_list[i].this_domain->pred_checker.error_power.pred_error_cnt;
		err_cnt += 1;
	}
	if(err_cnt==0) return 0;
	else		   return err_acc / err_cnt;
}

#define err_str "%u/%d"
#define err_val(pred) (pred.pred_error_cnt!=0)?CONV_scaledINTany_INTany((pred.pred_error_acc_u / pred.pred_error_cnt)*100):0, (pred.pred_error_cnt!=0)?CONV_scaledINTany_INTany((pred.pred_error_acc_s / (int32_t)pred.pred_error_cnt)*100):0

void vitamins_bin_predictor_error_report(model_sys_t *sys)
{
	int i = 0;
	pdebug("Prdiction error report:\n");
	for(i = 0; i < sys->task_list_size; ++i){
		pdebug("\ttask %d ips=" err_str " load=" err_str " s=%u\n",sys->task_list[i]->id,err_val(sys->task_list[i]->pred_checker.error_ips_active),err_val(sys->task_list[i]->pred_checker.error_load),sys->task_list[i]->pred_checker.error_load.pred_error_cnt);
	}
	for(i = 0; i < sys->info->core_list_size; ++i){
		pdebug("\tcore %d load=" err_str " s=%u\n",sys->info->core_list[i].position,err_val(sys->info->core_list[i].this_core->pred_checker.error_load),sys->info->core_list[i].this_core->pred_checker.error_load.pred_error_cnt);
	}
	for(i = 0; i < sys->info->freq_domain_list_size; ++i){
		pdebug("\tfreq_domain %d freq=" err_str " s=%u\n",sys->info->freq_domain_list[i].domain_id,err_val(sys->info->freq_domain_list[i].this_domain->pred_checker.error_freq),sys->info->freq_domain_list[i].this_domain->pred_checker.error_freq.pred_error_cnt);
	}
	for(i = 0; i < sys->info->power_domain_list_size; ++i){
		pdebug("\tpower_domain %d power=" err_str " s=%u\n",sys->info->power_domain_list[i].domain_id,err_val(sys->info->power_domain_list[i].this_domain->pred_checker.error_power),sys->info->power_domain_list[i].this_domain->pred_checker.error_power.pred_error_cnt);
	}
	pdebug("\tavg task ips=%u\n",CONV_scaledINTany_INTany(vitamins_bin_predictor_avg_task_ips_error()*100));
	pdebug("\tavg task load=%u\n",CONV_scaledINTany_INTany(vitamins_bin_predictor_avg_task_load_error()*100));
	pdebug("\tavg core load=%u\n",CONV_scaledINTany_INTany(vitamins_bin_predictor_avg_core_load_error(sys)*100));
	pdebug("\tavg freq_domain freq=%u\n",CONV_scaledINTany_INTany(vitamins_bin_predictor_avg_freq_error(sys)*100));
	pdebug("\tavg power_domain power=%u\n",CONV_scaledINTany_INTany(vitamins_bin_predictor_avg_power_error(sys)*100));
}

