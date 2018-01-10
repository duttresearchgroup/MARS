obj-m := vitamins.o
vitamins-objs := main.o helpers.o setup.o sense.o sense_data.o user_if.o user_if_ioctl.o sensing_window.o
vitamins-objs += pal/$(PLAT)/perf_sense.o ../common/pal/$(PLAT)/setup_info.o
vitamins-objs += ../../../core/base/info_init.o
 
PWD := $(shell pwd)

all:
	    KCPPFLAGS="-DPLAT_DEF=$(PLAT)" make -C $(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules
clean:
	    KCPPFLAGS="-DPLAT_DEF=$(PLAT)" make -C $(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean
