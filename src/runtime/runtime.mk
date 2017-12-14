
#########################################################
# Sources (updete if new directory with sources is added#
#########################################################

SRCS_CXX_RUNTIME = $(wildcard src/runtime/systems/*.cc)
SRCS_CXX_RUNTIME += $(wildcard src/runtime/daemon/*.cc)
SRCS_CXX_RUNTIME += $(wildcard src/runtime/common/*.cc)
SRCS_CXX_RUNTIME += $(wildcard src/runtime/framework/*.cc)

SRCS_CXX_RUNTIME += $(wildcard src/runtime/interfaces/*.cc)
SRCS_CXX_RUNTIME += $(wildcard src/runtime/interfaces/linux/*.cc)
ifeq ($(PLAT),offline)
	SRCS_CXX_RUNTIME += $(wildcard src/runtime/interfaces/offline/*.cc)
endif
SRCS_CXX_RUNTIME += $(wildcard src/runtime/interfaces/common/pal/$(PLAT)/*.cc)
SRCS_CC_RUNTIME += $(wildcard src/runtime/interfaces/common/pal/$(PLAT)/*.c)

SRCS_CXX_RUNTIME += $(wildcard src/runtime/systems/controll/*.cc)
SRCS_CXX_RUNTIME += $(wildcard src/runtime/systems/controll/micro17/*.cc)
SRCS_CXX_RUNTIME += $(wildcard src/runtime/systems/controll/iccad17/*.cc)
SRCS_CXX_RUNTIME += $(wildcard src/runtime/systems/controll/asplos18/*.cc)
SRCS_CXX_RUNTIME += $(wildcard src/runtime/systems/controll/date18/*.cc)


##############
# Other stuff#
##############

OBJS_RUNTIME = $(patsubst %.cc,obj_$(ARCH)_$(PLAT)/%.o,$(SRCS_CXX_RUNTIME))
OBJS_RUNTIME += $(patsubst %.c,obj_$(ARCH)_$(PLAT)/%.o,$(SRCS_CC_RUNTIME))

OBJS_DEPS += $(OBJS_RUNTIME:%.o=%.d)

lib_$(ARCH)_$(PLAT)/libruntime.a: $(OBJS_RUNTIME)
	mkdir -p lib_$(ARCH)_$(PLAT)
	$(AR) rvs $@  $(OBJS_RUNTIME)

.PHONY: runtime_lib
runtime_lib: lib_$(ARCH)_$(PLAT)/libruntime.a lib_$(ARCH)_$(PLAT)/libcpulimit.a

.PHONY: runtime_lib_clean
runtime_lib_clean:
	rm -f lib_$(ARCH)_$(PLAT)/libruntime.a $(OBJS_RUNTIME) $(OBJS_RUNTIME:%.o=%.d)

