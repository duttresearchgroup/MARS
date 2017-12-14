TAR = solver

ifndef ARCH
 $(error ARCH not defined, pass ARCH=alpha|arm|host to makefile)
endif

.PHONY: all dbg opt depend clean clean_dbg clean_opt kernel_lib_alpha

all: opt

dbg: $(TAR).mk obj_$(ARCH)_dbg
	@$(MAKE) EXTRAFLAGS=-std=c++11 TAG=dbg -C . -f $(TAR).mk

opt: $(TAR).mk obj_$(ARCH)_opt
	@$(MAKE) EXTRAFLAGS=-std=c++11 TAG=opt -C . -f $(TAR).mk

kernel_lib_alpha: $(TAR).mk obj_$(ARCH)_opt
	@$(MAKE) CROSS_COMPILE=alphaev67-unknown-linux-gnu- EXTRAFLAGS='-mcpu=ev67 -mtune=ev67' TAG=opt -C . -f $(TAR).mk library_alpha
	
kernel_lib_arm: $(TAR).mk obj_$(ARCH)_opt
	@$(MAKE) CROSS_COMPILE=arm-linux-gnueabi- TAG=opt -C . -f $(TAR).mk library_arm
	
kernel_lib_host: $(TAR).mk obj_$(ARCH)_opt
	@$(MAKE) EXTRAFLAGS=-std=c++11 TAG=opt -C . -f $(TAR).mk library_host

kernel_lib_alpha_dbg: $(TAR).mk obj_$(ARCH)_dbg
	@$(MAKE) CROSS_COMPILE=alphaev67-unknown-linux-gnu- TAG=dbg -C . -f $(TAR).mk library_alpha
	
kernel_lib_arm_dbg: $(TAR).mk obj_$(ARCH)_dbg
	@$(MAKE) CROSS_COMPILE=arm-linux-gnueabi- TAG=opt -C . -f $(TAR).mk library_arm



obj_$(ARCH)_dbg:
	mkdir $@

obj_$(ARCH)_opt:
	mkdir $@

clean: clean_dbg clean_opt

clean_dbg: obj_$(ARCH)_dbg
	@$(MAKE) TAG=dbg -C . -f $(TAR).mk clean
	rm -rf $<

clean_opt: obj_$(ARCH)_opt
	@$(MAKE) TAG=opt -C . -f $(TAR).mk clean
	rm -rf $<

