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

SRCS_CXX_SIM = $(wildcard src/offline_sim/*.cc)


##############
# Other stuff#
##############

OBJS_SIM = $(patsubst %.cc,obj_$(ARCH)_$(PLAT)/%.o,$(SRCS_CXX_SIM))

OBJS_DEPS += $(OBJS_SIM:%.o=%.d)

lib_$(ARCH)_$(PLAT)/libofflinesim.a: $(OBJS_SIM)  
	mkdir -p lib_$(ARCH)_$(PLAT)
	$(AR) rvs $@  $(OBJS_SIM)

.PHONY: offline_sim_lib
offline_sim_lib: lib_$(ARCH)_$(PLAT)/libofflinesim.a lib_$(ARCH)_$(PLAT)/libmcpat.a lib_$(ARCH)_$(PLAT)/liblinsched.a

.PHONY: offline_sim_lib_clean
offline_sim_lib_clean:
	rm -f lib_$(ARCH)_$(PLAT)/libofflinesim.a $(OBJS_SIM) $(OBJS_SIM:%.o=%.d)
	

