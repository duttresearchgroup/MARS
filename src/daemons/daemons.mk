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

SRCS_DAEMONS_COMMON = $(wildcard src/daemons/common/*.cc)    
SRCS_DAEMONS = $(wildcard src/daemons/*.cc)

##############
# Other stuff#
##############

OBJS_DAEMONS_COMMON = $(patsubst %.cc,obj_$(ARCH)_$(PLAT)/%.o,$(SRCS_DAEMONS_COMMON))

OBJS_DEPS += $(OBJS_DAEMONS_COMMON:%.o=%.d)

BINS_DAEMONS = $(patsubst src/daemons/%.cc,bin_$(ARCH)_$(PLAT)/daemons/%,$(SRCS_DAEMONS))

ifeq ($(DAEMONS),)
BINS_DAEMONS_FILTERED = $(BINS_DAEMONS)
else
_subst_comma:= ,
DAEMONS_FILTER=%$(subst $(_subst_comma), %,$(DAEMONS))
BINS_DAEMONS_FILTERED = $(filter $(DAEMONS_FILTER),$(BINS_DAEMONS))
endif

ifeq ($(BINS_DAEMONS_FILTERED),)
$(info No daemons to build. Is the filter correct ?)
endif
     
bin_$(ARCH)_$(PLAT)/daemons/%: src/daemons/%.cc $(OBJS_DAEMONS_COMMON) lib_$(ARCH)_$(PLAT)/libruntime.a lib_$(ARCH)_$(PLAT)/libbase.a lib_$(ARCH)_$(PLAT)/libcpulimit.a
	$(CXX) -static $(CXXFLAGS) $^ -o $@
	
bin_$(ARCH)_$(PLAT)/daemons:
	mkdir -p bin_$(ARCH)_$(PLAT)/daemons

.PHONY: daemons
daemons: bin_$(ARCH)_$(PLAT)/daemons $(BINS_DAEMONS_FILTERED)

.PHONY: daemons_clean
daemons_clean:
	rm -rf bin_$(ARCH)_$(PLAT)/daemons $(OBJS_DAEMONS_COMMON) $(OBJS_DAEMONS_COMMON:%.o=%.d)
