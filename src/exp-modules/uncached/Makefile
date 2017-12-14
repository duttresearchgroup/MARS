obj-m := uncached.o
uncached-objs := main.o shared_data.o user_if.o 
 
PWD := $(shell pwd)

all:
	make -C $(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules
clean:
	make -C $(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean
