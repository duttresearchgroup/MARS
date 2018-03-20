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

#########################################################
# Sources (updete if new directory with sources is added#
#########################################################
    
SRCS_CXX_APPS_COMMON = $(wildcard src/apps/common/*.cc) 
SRCS_CXX_APPS = $(wildcard src/apps/*.cc)


##############
# Other stuff#
##############

OBJS_CXX_APPS_COMMON = $(patsubst %.cc,obj_$(ARCH)_$(PLAT)/%.o,$(SRCS_CXX_APPS_COMMON))

OBJS_DEPS += $(OBJS_CXX_APPS_COMMON:%.o=%.d)

APPS_LIBS_DEPS = lib_$(ARCH)_$(PLAT)/libsasolver.a	
APPS_LIBS_DEPS += lib_$(ARCH)_$(PLAT)/libofflinesim.a 	
APPS_LIBS_DEPS += lib_$(ARCH)_$(PLAT)/libruntime.a
APPS_LIBS_DEPS += lib_$(ARCH)_$(PLAT)/liblinsched.a 
APPS_LIBS_DEPS += lib_$(ARCH)_$(PLAT)/liblegacycore.a
APPS_LIBS_DEPS += lib_$(ARCH)_$(PLAT)/libbase.a
APPS_LIBS_DEPS += lib_$(ARCH)_$(PLAT)/libmcpat.a

bin_$(ARCH)_$(PLAT)/apps/%: src/apps/%.cc $(OBJS_CXX_APPS_COMMON) $(APPS_LIBS_DEPS)
	$(CXX) $(CXXFLAGS) $^ -o $@
	
bin_$(ARCH)_$(PLAT)/apps:
	mkdir -p bin_$(ARCH)_$(PLAT)/apps

BINS_APPS = $(patsubst src/apps/%.cc,bin_$(ARCH)_$(PLAT)/apps/%,$(SRCS_CXX_APPS))

.PHONY: apps
apps: bin_$(ARCH)_$(PLAT)/apps $(BINS_APPS)

.PHONY: apps_clean
apps_clean:
	rm -rf bin_$(ARCH)_$(PLAT)/apps $(OBJS_CXX_APPS_COMMON) $(OBJS_CXX_APPS_COMMON:%.o=%.d)

