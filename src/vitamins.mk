#-------------------------------------------------------------------------------
# Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#-------------------------------------------------------------------------------

.SUFFIXES: .c .cc .o .a .d

#OPT_FLAGS= -O0 -g
OPT_FLAGS= -O2 -g
FLAGS = -Wno-psabi -Wextra -Wall -Werror -Wno-unused-parameter -Wno-error=cpp -Wno-error=format -Wno-error=unused-result $(OPT_FLAGS) $(EXTRAFLAGS) -Isrc -DPLAT_DEF=$(PLAT)
CXXFLAGS = $(FLAGS) -std=c++11 -pthread -Isrc/external/linsched/tools/linsched/include -Isrc/external/mcpat
CCFLAGS = $(FLAGS) -std=c99 -pedantic-errors -Wstrict-prototypes #-nostdlib -nodefaultlibs -fno-exceptions -DNDEBUG

CXX = $(CROSS_COMPILE)g++
CC  = $(CROSS_COMPILE)gcc 
AR  = $(CROSS_COMPILE)ar


include src/base/base.mk
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
clean: base_lib_clean runtime_lib_clean offline_sim_lib_clean apps_clean daemons_clean
	rm -rf lib_$(ARCH)_$(PLAT)
	rm -rf obj_$(ARCH)_$(PLAT)
	rm -rf bin_$(ARCH)_$(PLAT)


