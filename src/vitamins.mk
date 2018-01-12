.SUFFIXES: .c .cc .o .a .d

#OPT_FLAGS= -O0 -g
OPT_FLAGS= -O2 -g
FLAGS = -Wno-psabi -Wextra -Wall -Werror -Wno-unused-parameter -Wno-error=cpp -Wno-error=format -Wno-error=unused-result $(OPT_FLAGS) $(EXTRAFLAGS) -Isrc -DPLAT_DEF=$(PLAT)
CXXFLAGS = $(FLAGS) -std=c++11 -pthread -Isrc/external/linsched/tools/linsched/include -Isrc/external/mcpat -Isrc/external/tclap/include
CCFLAGS = $(FLAGS) -std=c99 -pedantic-errors -Wstrict-prototypes #-nostdlib -nodefaultlibs -fno-exceptions -DNDEBUG

CXX = $(CROSS_COMPILE)g++
CC  = $(CROSS_COMPILE)gcc 
AR  = $(CROSS_COMPILE)ar


include src/core/core.mk
include src/apps/apps.mk
include src/offline_sim/sim.mk
include src/runtime/runtime.mk
include src/runtime/uapi/tests/uapi_tests.mk
include src/daemons/daemons.mk
include src/external/external.mk

-include $(OBJS_DEPS)

obj_$(ARCH)_$(PLAT)/%.o : %.cc
	mkdir -p $(dir $@)
	$(CXX) -MP -MD $(CXXFLAGS) -c $< -o $@

obj_$(ARCH)_$(PLAT)/%.o : %.c
	mkdir -p $(dir $@)
	$(CC) -MP -MD $(CCFLAGS) -c $< -o $@

.PHONY: clean
clean: core_lib_clean runtime_lib_clean offline_sim_lib_clean apps_clean daemons_clean
	rm -rf lib_$(ARCH)_$(PLAT)
	rm -rf obj_$(ARCH)_$(PLAT)
	rm -rf bin_$(ARCH)_$(PLAT)


