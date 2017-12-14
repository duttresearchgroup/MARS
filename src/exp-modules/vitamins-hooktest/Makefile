obj-m := vitamins-hooktest.o
vitamins-hooktest-objs := vitamins_main.o
vitamins-hooktest-objs += \
	../../core/vitamins_sparta.o \
	../../core/vitamins_sparta_shared.o \
	../../core/vitamins_cfs.o \
	../../core/vitamins_dvfs.o \
	../../core/vitamins_mts.o \
	../../core/vitamins_gts.o \
	../../core/vitamins_vanilla.o \
	../../core/vitamins_random.o \
	../../core/vitamins_optimal.o \
	../../core/vitamins_bin_predictor.o \
	../../core/vitamins_bin_predictor_init.o \
	../../core/vitamins_bin_predictor_io.o \
	../../core/vitamins_idlepower.o \
	../../core/vitamins_fileio.o
 
PWD := $(shell pwd)

all:
	    make -C $(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules
clean:
	    make -C $(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean
