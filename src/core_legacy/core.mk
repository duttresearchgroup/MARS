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

SRCS_CXX_CORE = $(wildcard src/core_legacy/*.cc)

##############
# Other stuff#
##############

OBJS_CORE = $(patsubst %.cc,obj_$(ARCH)_$(PLAT)/%.o,$(SRCS_CXX_CORE))

OBJS_DEPS += $(OBJS_CORE:%.o=%.d)

lib_$(ARCH)_$(PLAT)/liblegacycore.a: $(OBJS_CORE)
	mkdir -p lib_$(ARCH)_$(PLAT)
	$(AR) rvs $@  $(OBJS_CORE)

lib_$(ARCH)_$(PLAT)/libsasolver.a: src/sa_solver/lib/libsasolver$(ARCH).a
	mkdir -p lib_$(ARCH)_$(PLAT)
	cp $< $@

src/sa_solver/lib/libsasolver$(ARCH).a:
	cd src/sa_solver; make kernel_lib ARCH=$(ARCH)

.PHONY: core_lib
core_lib: lib_$(ARCH)_$(PLAT)/liblegacycore.a lib_$(ARCH)_$(PLAT)/libsasolver.a 

.PHONY: core_lib_clean
core_lib_clean:
	cd src/sa_solver; make clean ARCH=$(ARCH)
	rm -f lib_$(ARCH)_$(PLAT)/liblegacycore.a lib_$(ARCH)_$(PLAT)/libsasolver.a
	rm -f $(OBJS_CORE) $(OBJS_CORE:%.o=%.d)
