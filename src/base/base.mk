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

SRCS_CC_BASE = $(wildcard src/base/*.c)

##############
# Other stuff#
##############

OBJS_BASE = $(patsubst %.c,obj_$(ARCH)_$(PLAT)/%.o,$(SRCS_CC_BASE))

OBJS_DEPS += $(OBJS_BASE:%.o=%.d)

lib_$(ARCH)_$(PLAT)/libbase.a: $(OBJS_BASE)
	mkdir -p lib_$(ARCH)_$(PLAT)
	$(AR) rvs $@  $(OBJS_BASE)

.PHONY: base_lib
base_lib: lib_$(ARCH)_$(PLAT)/libbase.a 

.PHONY: base_lib_clean
base_lib_clean:
	rm -f lib_$(ARCH)_$(PLAT)/libbase.a
	rm -f $(OBJS_CORE) $(OBJS_BASE:%.o=%.d)
