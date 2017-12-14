
#include "core.h"

static uint32_t _core_idle_power_scaled[SIZE_COREARCH][SIZE_COREFREQ];
static bool _core_idle_power_set[SIZE_COREARCH][SIZE_COREFREQ];
static bool _core_idle_power_set_mask[SIZE_COREARCH][SIZE_COREFREQ];

uint32_t
arch_idle_power_scaled(core_arch_t arch, core_freq_t freq)
{
    BUG_ON((_core_idle_power_set[arch][freq] && _core_idle_power_set_mask[arch][freq]) != true);
    return _core_idle_power_scaled[arch][freq];
}

bool
vitamins_arch_freq_available(core_arch_t arch, core_freq_t freq)
{
    return ((freq == COREFREQ_0000MHz) || (_core_idle_power_set[arch][freq] && _core_idle_power_set_mask[arch][freq]));
}
bool
vitamins_arch_freq_available_nomask(core_arch_t arch, core_freq_t freq)
{
    return _core_idle_power_set[arch][freq];
}

core_freq_t vitamins_arch_highest_freq_available(core_arch_t arch)
{
    for_enum(core_freq_t,freq,1,SIZE_COREFREQ,+1){
        if(vitamins_arch_freq_available(arch,freq)) return freq;
    }
    BUG_ON(true);
    return (core_freq_t)0;
}

void
vitamins_reset_archs(void)
{
    int arch,freq;
    for(arch = 0; arch < SIZE_COREARCH; ++arch)
        for(freq = 0; freq < SIZE_COREFREQ; ++freq){
            _core_idle_power_set[arch][freq] = false;
            _core_idle_power_set_mask[arch][freq] = true;
        }
}

void
vitamins_init_idle_power(core_arch_t arch, core_freq_t freq, uint32_t power)
{
    _core_idle_power_scaled[arch][freq] = power;
    _core_idle_power_set[arch][freq] = true;
}

void
vitamins_arch_freq_available_set(core_arch_t arch, core_freq_t *freq_list,int freq_list_size)
{
    int i;
    int freq;
    for(i = 0; i < freq_list_size; ++i) BUG_ON(!_core_idle_power_set[arch][freq_list[i]]);

    for(freq = 0; freq < SIZE_COREFREQ; ++freq)
        _core_idle_power_set_mask[arch][freq] = false;

    for(i = 0; i < freq_list_size; ++i)
        _core_idle_power_set_mask[arch][freq_list[i]] = true;
}

void
vitamins_writefile_idle_power(const char* filepath)
{
	uint32_t arch,freq;
	uint32_t entries = SIZE_COREARCH*SIZE_COREFREQ;

	vfile_t file = open_file_wr(filepath);

	file_wr_word(&file,entries);

	for(arch = 0; arch < SIZE_COREARCH; ++arch){
		for(freq = 0; freq < SIZE_COREFREQ; ++freq){
			file_wr_word(&file,arch);
			file_wr_word(&file,freq);
			file_wr_word(&file,_core_idle_power_set[arch][freq]);
			file_wr_word(&file,_core_idle_power_scaled[arch][freq]);
		}
	}

	close_file(&file);
}

void
vitamins_init_idle_power_fromfile(const char* filepath)
{
	uint32_t arch,freq;
	vfile_t file = open_file_rd(filepath);

	bool good = file_rd_word(&file) == (SIZE_COREFREQ*SIZE_COREARCH);
	if(good){
		vitamins_reset_archs();
		for(arch = 0; arch < SIZE_COREARCH; ++arch){
			for(freq = 0; freq < SIZE_COREFREQ; ++freq){
				good = good && (file_rd_word(&file) == arch);
				good = good && (file_rd_word(&file) == freq);
				_core_idle_power_set[arch][freq] = file_rd_word(&file);
				_core_idle_power_scaled[arch][freq] = file_rd_word(&file);
				if(!good) break;
			}
		}
	}

	close_file(&file);

	if(!good) BUG_ON("Missmatch bewteen code and file");
}
