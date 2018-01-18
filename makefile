
#################################
#Makefile build params          #
#################################
#   ARCH: ISA used for compiling. Affects the GCC being used. Default is ARCH=host if undefined
#   PLAT: platform to use when compiling modules with platform-specific code. Set based on the value for ARCH if undefined
#   CROSS_COMPILE_usr: Which GCC to use for user space applications. Set based on the value for ARCH if undefined
#   CROSS_COMPILE_krn: Which GCC to use for kernel modules. Set based on the value for ARCH if undefined
#   EXTRAFLAGS: Extra GCC options. Set based on the value for ARCH if undefined
#   KERNEL_SRC: Path to kernel source used when compiling linux kernel modules. Set based on the value for ARCH if undefined
#   MODULE: linux module to compile when running 'make exp_module'. No default value.


#Options not set when calling make are set to the values defined in the 'makefile.buildopts'

#The file is generated after the first time make is called and can be modified afterwards
MAKEOPTS := $(shell cp -n .makefile.buildopts makefile.buildopts)
include makefile.buildopts

#####################################
#####################################

ifndef ARCH
ARCH=$(ARCH_DEFAULT)
endif

ifndef PLAT
PLAT=$(PLAT_DEFAULT_$(ARCH))
endif

ifndef CROSS_COMPILE_usr
CROSS_COMPILE_usr=$(CROSS_COMPILE_usr_DEFAULT_$(ARCH)_$(PLAT))
endif

ifndef CROSS_COMPILE_krn
CROSS_COMPILE_krn=$(CROSS_COMPILE_krn_DEFAULT_$(ARCH)_$(PLAT))
endif

ifndef EXTRAFLAGS
EXTRAFLAGS=$(EXTRAFLAGS_DEFAULT_$(ARCH))
endif

ifndef KERNEL_SRC
KERNEL_SRC=$(KERNEL_SRC_DEFAULT_$(ARCH))
endif


$(info ARCH = ${ARCH})
$(info PLAT = ${PLAT})
$(info CROSS_COMPILE_usr = ${CROSS_COMPILE_usr})
$(info CROSS_COMPILE_krn = ${CROSS_COMPILE_krn})
$(info EXTRAFLAGS = ${EXTRAFLAGS})
$(info KERNEL_SRC = ${KERNEL_SRC})
$(info MODULE = ${MODULE})

#####################################
#####################################

MODULEDIR=src/exp-modules
ALLMODULES := $(wildcard $(MODULEDIR)/*/.)

.PHONY: all
all:
	@printf "\nSpecify one of the following targets\n"
	@printf "\tapps:\n\t\tbuilds host applications (offline simulation apps, tools, etc)\n"
	@printf "\tdeamons:\n\t\tbuilds the runtime daemons\n"
	@printf "\tclean:\n\t\tcleans daemons and apps\n"
	@printf "\tuapi_tests/uapi_tests_clean:\n\t\tbuilds/cleans tests for the daemons<->app interfaces\n"
	@printf "\tlin_sensing_module/lin_sensing_module_clean:\n\t\tbuilds/cleans the sensing kernel module used by the daemons\n"
	@printf "\truntime:\n\t\tdoes targets daemons lin_sensing_module uapi_tests\n"
	@printf "\texp_module/exp_module_clean:\n\t\tbuilds/cleans experimental kernel modules (specify using param)\n"
	@printf "\texternal_clean:\n\t\tcleans external dependencies\n"
	@printf "\tvery_clean:\n\t\tcleans everything very nicely\n"
	@exit

.PHONY: apps
apps: src/vitamins.mk
ifneq ($(ARCH),host)
	$(error 'make apps' is only valid for ARCH=host)
	@exit
endif
	@$(MAKE) ARCH=$(ARCH) PLAT=$(PLAT) CROSS_COMPILE=$(CROSS_COMPILE_usr) EXTRAFLAGS=$(EXTRAFLAGS) -C . -f src/vitamins.mk apps

.PHONY: exp_module
exp_module:
ifeq ($(ARCH),host)
	@$(MAKE) ARCH=x86 PLAT=$(PLAT) CROSS_COMPILE=$(CROSS_COMPILE_krn) EXTRAFLAGS=$(EXTRAFLAGS) KERNEL_DIR=$(KERNEL_SRC) -C src/exp-modules/$(MODULE) all	
else
	@$(MAKE) ARCH=$(ARCH) PLAT=$(PLAT) CROSS_COMPILE=$(CROSS_COMPILE_krn) EXTRAFLAGS=$(EXTRAFLAGS) KERNEL_DIR=$(KERNEL_SRC) -C src/exp-modules/$(MODULE) all
endif

.PHONY: exp_module_clean
exp_module_clean: $(ALLMODULES)

.PHONY: $(ALLMODULES)
$(ALLMODULES):
ifeq ($(ARCH),host)
	@$(MAKE) ARCH=x86 PLAT=$(PLAT) CROSS_COMPILE=$(CROSS_COMPILE_krn) EXTRAFLAGS=$(EXTRAFLAGS) KERNEL_DIR=$(KERNEL_SRC) -C $@ clean
else 
	@$(MAKE) ARCH=$(ARCH) PLAT=$(PLAT) CROSS_COMPILE=$(CROSS_COMPILE_krn) EXTRAFLAGS=$(EXTRAFLAGS) KERNEL_DIR=$(KERNEL_SRC) -C $@ clean
endif

.PHONY: lin_sensing_module
lin_sensing_module:
ifeq ($(ARCH),host)
	@$(MAKE) ARCH=x86 PLAT=$(PLAT) CROSS_COMPILE=$(CROSS_COMPILE_krn) EXTRAFLAGS=$(EXTRAFLAGS) KERNEL_DIR=$(KERNEL_SRC) -C src/runtime/interfaces/linux-module all	
else
	@$(MAKE) ARCH=$(ARCH) PLAT=$(PLAT) CROSS_COMPILE=$(CROSS_COMPILE_krn) EXTRAFLAGS=$(EXTRAFLAGS) KERNEL_DIR=$(KERNEL_SRC) -C src/runtime/interfaces/linux-module all
endif
	mkdir -p bin_$(ARCH)_$(PLAT)/sensing_module
	cp src/runtime/interfaces/linux-module/vitamins.ko bin_$(ARCH)_$(PLAT)/sensing_module/

.PHONY: lin_sensing_module_clean
lin_sensing_module_clean:
ifeq ($(ARCH),host)
	@$(MAKE) ARCH=x86 PLAT=$(PLAT) CROSS_COMPILE=$(CROSS_COMPILE_krn) EXTRAFLAGS=$(EXTRAFLAGS) KERNEL_DIR=$(KERNEL_SRC) -C src/runtime/interfaces/linux-module clean
else 
	@$(MAKE) ARCH=$(ARCH) PLAT=$(PLAT) CROSS_COMPILE=$(CROSS_COMPILE_krn) EXTRAFLAGS=$(EXTRAFLAGS) KERNEL_DIR=$(KERNEL_SRC) -C src/runtime/interfaces/linux-module clean
endif

.PHONY: daemons
daemons: src/vitamins.mk
	@$(MAKE) ARCH=$(ARCH) PLAT=$(PLAT) CROSS_COMPILE=$(CROSS_COMPILE_usr) EXTRAFLAGS=$(EXTRAFLAGS) -C . -f src/vitamins.mk daemons

.PHONY: uapi_tests
uapi_tests: src/vitamins.mk
	@$(MAKE) ARCH=$(ARCH) PLAT=$(PLAT) CROSS_COMPILE=$(CROSS_COMPILE_usr) EXTRAFLAGS=$(EXTRAFLAGS) -C . -f src/vitamins.mk uapi_tests

.PHONY: uapi_tests_clean
uapi_tests_clean: src/vitamins.mk
	@$(MAKE) ARCH=$(ARCH) PLAT=$(PLAT) CROSS_COMPILE=$(CROSS_COMPILE_usr) EXTRAFLAGS=$(EXTRAFLAGS) -C . -f src/vitamins.mk clean_uapi_tests

.PHONY: runtime
runtime: daemons lin_sensing_module uapi_tests

.PHONY: clean
clean:
	@$(MAKE) ARCH=$(ARCH) PLAT=$(PLAT) -C . -f src/vitamins.mk clean

.PHONY: external_clean
external_clean:
	@$(MAKE) ARCH=$(ARCH) PLAT=$(PLAT) -C . -f src/vitamins.mk external_clean

.PHONY: veryclean
veryclean:
	@$(MAKE) ARCH=host PLAT=$(PLAT) -C . clean lin_sensing_module_clean exp_module_clean external_clean
	@$(MAKE) ARCH=arm PLAT=exynos5422 -C . clean lin_sensing_module_clean exp_module_clean external_clean
	@$(MAKE) ARCH=arm PLAT=exynos5422 -C . clean lin_sensing_module_clean exp_module_clean external_clean
	rm -rf obj_*
	rm -rf lib_*
	rm -rf bin_*



