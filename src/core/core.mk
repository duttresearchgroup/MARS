
#########################################################
# Sources (updete if new directory with sources is added#
#########################################################

SRCS_CXX_CORE = $(wildcard src/core/*.cc)
SRCS_CC_CORE = $(wildcard src/core/base/*.c)

##############
# Other stuff#
##############

OBJS_CORE = $(patsubst %.cc,obj_$(ARCH)_$(PLAT)/%.o,$(SRCS_CXX_CORE))
OBJS_CORE += $(patsubst %.c,obj_$(ARCH)_$(PLAT)/%.o,$(SRCS_CC_CORE))

OBJS_DEPS += $(OBJS_CORE:%.o=%.d)

lib_$(ARCH)_$(PLAT)/libcore.a: $(OBJS_CORE)
	mkdir -p lib_$(ARCH)_$(PLAT)
	$(AR) rvs $@  $(OBJS_CORE)

lib_$(ARCH)_$(PLAT)/libsasolver.a: src/sa_solver/lib/libsasolver$(ARCH).a
	mkdir -p lib_$(ARCH)_$(PLAT)
	cp $< $@

src/sa_solver/lib/libsasolver$(ARCH).a:
	cd src/sa_solver; make kernel_lib_$(ARCH) ARCH=$(ARCH)

.PHONY: core_lib
core_lib: lib_$(ARCH)_$(PLAT)/libcore.a lib_$(ARCH)_$(PLAT)/libsasolver.a 

.PHONY: core_lib_clean
core_lib_clean:
	cd src/sa_solver; make clean ARCH=$(ARCH)
	rm -f lib_$(ARCH)_$(PLAT)/libcore.a lib_$(ARCH)_$(PLAT)/libsasolver.a
	rm -f $(OBJS_CORE) $(OBJS_CORE:%.o=%.d)
