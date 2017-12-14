//#define HASDEBUG
#include "core.h"

bin_pred_ptr_t vitamins_bin_predictor_alloc_new()
{
    int srcCore,srcFreq,tgtCore,tgtFreq;
    bin_pred_ptr_t predictors =
            new bin_pred_layer_t ****[SIZE_COREARCH];

    for(srcCore = 0; srcCore < SIZE_COREARCH; ++srcCore){
    	predictors[srcCore] = new bin_pred_layer_t ***[SIZE_COREFREQ];
    	for(srcFreq = 0; srcFreq < SIZE_COREFREQ; ++srcFreq){
    		predictors[srcCore][srcFreq] = new bin_pred_layer_t **[SIZE_COREARCH];
    		for(tgtCore = 0; tgtCore < SIZE_COREARCH; ++tgtCore){
    			predictors[srcCore][srcFreq][tgtCore] = new bin_pred_layer_t *[SIZE_COREFREQ];
    			for(tgtFreq = 0; tgtFreq < SIZE_COREFREQ; ++tgtFreq){
    				predictors[srcCore][srcFreq][tgtCore][tgtFreq] = nullptr;
    			}
    		}
    	}
    }
    return predictors;
}


static inline uint32_t bin_predictor_func_ipcActive(task_sensed_data_t *data)
{
    return data->ipc_active;
}
static inline uint32_t bin_predictor_func_sumL1ILIDL2misses(task_sensed_data_t *data)
{
    return data->icache_missrate + data->dcache_missrate + data->l2cache_misses_perinstr;
}
static inline uint32_t bin_predictor_func_LIDmissesPerInstr(task_sensed_data_t *data)
{
	return data->dcache_misses_perinstr;
}
static inline uint32_t bin_predictor_func_L2missesPerInstr(task_sensed_data_t *data)
{
	return data->l2cache_misses_perinstr;
}
static inline uint32_t bin_predictor_func_sumLIDL2missesPerInstr(task_sensed_data_t *data)
{
	return data->dcache_misses_perinstr + data->l2cache_misses_perinstr;
}
static inline uint32_t bin_predictor_func_brMisspredrate(task_sensed_data_t *data)
{
    return data->br_misspreds_perinstr;
}
static inline uint32_t bin_predictor_func_procTimeShare(task_sensed_data_t *data)
{
    return data->proc_time_share;
}

bin_pred_func vitamins_get_bin_pred_func(bin_pred_func_id id)
{
    switch (id) {
    case BIN_PRED_FUNC_ID_ipcActive:            return bin_predictor_func_ipcActive;
    case BIN_PRED_FUNC_ID_sumL1ILIDL2misses:    return bin_predictor_func_sumL1ILIDL2misses;
    case BIN_PRED_FUNC_ID_LIDmissesPerInstr:    return bin_predictor_func_LIDmissesPerInstr;
    case BIN_PRED_FUNC_ID_L2missesPerInstr:    return bin_predictor_func_L2missesPerInstr;
    case BIN_PRED_FUNC_ID_sumLIDL2missesPerInstr:    return bin_predictor_func_sumLIDL2missesPerInstr;
    case BIN_PRED_FUNC_ID_brMisspredrate:       return bin_predictor_func_brMisspredrate;
    case BIN_PRED_FUNC_ID_procTimeShare:        return bin_predictor_func_procTimeShare;
    default:
        BUG_ON("Invalid id");
        return 0;
    }
}
const char* vitamins_get_bin_pred_func_name(bin_pred_func_id id)
{
    switch (id) {
    case BIN_PRED_FUNC_ID_ipcActive:            return "ipcActive";
    case BIN_PRED_FUNC_ID_sumL1ILIDL2misses:    return "sumL1ILIDL2misses";
    case BIN_PRED_FUNC_ID_LIDmissesPerInstr:    return "LIDmissesPerInstr";
    case BIN_PRED_FUNC_ID_L2missesPerInstr:    return "L2missesPerInstr";
    case BIN_PRED_FUNC_ID_sumLIDL2missesPerInstr:    return "sumLIDL2missesPerInstr";
    case BIN_PRED_FUNC_ID_brMisspredrate:       return "brMisspredrate";
    case BIN_PRED_FUNC_ID_procTimeShare:        return "procTimeShare";
    case BIN_PRED_FUNC_ID_powerActive:        return "powerActive";
    default:
        BUG_ON("Invalid id");
        return 0;
    }
}
bin_pred_func_id vitamins_get_bin_pred_func_id(bin_pred_func f)
{
    if      (f == bin_predictor_func_ipcActive)            return BIN_PRED_FUNC_ID_ipcActive;
    else if (f == bin_predictor_func_sumL1ILIDL2misses)          return BIN_PRED_FUNC_ID_sumL1ILIDL2misses;
    else if (f == bin_predictor_func_LIDmissesPerInstr)          return BIN_PRED_FUNC_ID_LIDmissesPerInstr;
    else if (f == bin_predictor_func_L2missesPerInstr)          return BIN_PRED_FUNC_ID_L2missesPerInstr;
    else if (f == bin_predictor_func_sumLIDL2missesPerInstr)          return BIN_PRED_FUNC_ID_sumLIDL2missesPerInstr;
    else if (f == bin_predictor_func_brMisspredrate)   return BIN_PRED_FUNC_ID_brMisspredrate;
    else if (f == bin_predictor_func_procTimeShare)        return BIN_PRED_FUNC_ID_procTimeShare;
    else{
        BUG_ON("Invalid func");
        return SIZE_BIN_PRED_FUNC_ID;
    }
}

static inline int absolute(int a) { return (a<0) ? a*-1 : a;}

void vitamins_bin_predictor_init(const char* filepath)
{
	//int srcCore,srcFreq,tgtCore,tgtFreq;

    BUG_ON("Predictor already set"&&bin_predictors_ipcpower!=nullptr);
    bin_predictors_ipcpower = vitamins_bin_predictor_alloc_new();
    vitamins_bin_predictor_readfile(filepath,bin_predictors_ipcpower,false,nullptr);

    //not all arch/freq prediction info must be available
    //we need at least one freq for each arch in the bin_predictors_ipcpower
    //this fills in the gap for all vitamins_arch_freq_available
    /*for(srcCore = 0; srcCore < SIZE_COREARCH; ++srcCore){
    	for(srcFreq = 0; srcFreq < SIZE_COREFREQ; ++srcFreq){
    		if(!vitamins_arch_freq_available_nomask(srcCore,srcFreq)) continue;

    		for(tgtCore = 0; tgtCore < SIZE_COREARCH; ++tgtCore){
    			for(tgtFreq = 0; tgtFreq < SIZE_COREFREQ; ++tgtFreq){
    				if(!vitamins_arch_freq_available_nomask(tgtCore,tgtFreq)) continue;

    				if(bin_predictors_ipcpower[srcCore][srcFreq][tgtCore][tgtFreq] == nullptr){
    					int nearestSrcFreq=SIZE_COREFREQ,nearestTgtFreq=SIZE_COREFREQ;
    					int nearestFreqDist=SIZE_COREFREQ*2;
    					int srcFreqAux,tgtFreqAux;
    					for(srcFreqAux = 0; srcFreqAux < SIZE_COREFREQ; ++srcFreqAux){
    						for(tgtFreqAux = 0; tgtFreqAux < SIZE_COREFREQ; ++tgtFreqAux){
    							if(bin_predictors_ipcpower[srcCore][srcFreqAux][tgtCore][tgtFreqAux] != nullptr){
    								int aux = absolute(srcFreqAux-srcFreq)+absolute(tgtFreqAux-tgtFreq);
    								if(aux <= nearestFreqDist){
    									nearestFreqDist = aux;
    									nearestSrcFreq = srcFreqAux;
    									nearestTgtFreq = tgtFreqAux;
    								}
    							}
    						}
    					}
    					BUG_ON(nearestTgtFreq==SIZE_COREFREQ);
    					BUG_ON(nearestSrcFreq==SIZE_COREFREQ);
    					pinfo("VITAMINS predictor for %s@%s->%s@%s not available, using %s@%s->%s@%s instead\n",
    							archToString(srcCore),freqToString(srcFreq),archToString(tgtCore),freqToString(tgtFreq),
    							archToString(srcCore),freqToString(nearestSrcFreq),archToString(tgtCore),freqToString(nearestTgtFreq));

    					BUG_ON(bin_predictors_ipcpower[srcCore][nearestSrcFreq][tgtCore][nearestTgtFreq] == nullptr);
    					BUG_ON(bin_predictors_ipcpower[srcCore][srcFreq][tgtCore][tgtFreq] != nullptr);

    					bin_predictors_ipcpower[srcCore][srcFreq][tgtCore][tgtFreq] = bin_predictors_ipcpower[srcCore][nearestSrcFreq][tgtCore][nearestTgtFreq];

    				}
    			}
    		}
    	}
    }*/


}


static void _vitamins_bin_predictor_cleanup_layer(bin_pred_layer_t *layer){
	if(layer->next_layer != nullptr){
		int i;
		for(i = 0; i < layer->num_of_bins; ++i)
			_vitamins_bin_predictor_cleanup_layer(layer->next_layer[i]);

		delete[] layer->next_layer;
	}
	else{
		delete[] layer->bin_result;
	}
	delete[] layer->bins;

	delete layer;
}

static void _vitamins_bin_predictor_cleanup(bin_pred_ptr_t pred){
	int srcCore,srcFreq,tgtCore,tgtFreq;
	BUG_ON("Predictor not set"&&(pred==nullptr));

	for(srcCore = 0; srcCore < SIZE_COREARCH; ++srcCore){
		BUG_ON(pred[srcCore] == nullptr);
		for(srcFreq = 0; srcFreq < SIZE_COREFREQ; ++srcFreq){
			BUG_ON(pred[srcCore][srcFreq] == nullptr);
			for(tgtCore = 0; tgtCore < SIZE_COREARCH; ++tgtCore){
				BUG_ON(pred[srcCore][srcFreq][tgtCore] == nullptr);
				for(tgtFreq = 0; tgtFreq < SIZE_COREFREQ; ++tgtFreq){
					if(pred[srcCore][srcFreq][tgtCore][tgtFreq] != nullptr)
						_vitamins_bin_predictor_cleanup_layer(pred[srcCore][srcFreq][tgtCore][tgtFreq]);
				}
				delete[] pred[srcCore][srcFreq][tgtCore];
			}
			delete[] pred[srcCore][srcFreq];
		}
		delete[] pred[srcCore];
	}
    delete[] pred;
    pred=nullptr;
}

void vitamins_bin_predictor_cleanup(){
	_vitamins_bin_predictor_cleanup(bin_predictors_ipcpower);
	bin_predictors_ipcpower = nullptr;
}

