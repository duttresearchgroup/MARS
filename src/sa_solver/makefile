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

TAR = solver

ifndef ARCH
 $(error ARCH not defined, pass ARCH=alpha|arm|x86 to makefile)
endif

.PHONY: all dbg opt depend clean clean_dbg clean_opt kernel_lib

all: opt

dbg: $(TAR).mk obj_$(ARCH)_dbg
	@$(MAKE) EXTRAFLAGS=-std=c++11 TAG=dbg -C . -f $(TAR).mk

opt: $(TAR).mk obj_$(ARCH)_opt
	@$(MAKE) EXTRAFLAGS=-std=c++11 TAG=opt -C . -f $(TAR).mk

kernel_lib: $(TAR).mk obj_$(ARCH)_opt
	@$(MAKE) CROSS_COMPILE=$(CROSS_COMPILE) EXTRAFLAGS=$(EXTRAFLAGS) TAG=opt -C . -f $(TAR).mk library

kernel_lib_dbg: $(TAR).mk obj_$(ARCH)_dbg
	@$(MAKE) CROSS_COMPILE=$(CROSS_COMPILE) EXTRAFLAGS=$(EXTRAFLAGS) TAG=dbg -C . -f $(TAR).mk library
	
obj_$(ARCH)_dbg:
	mkdir $@

obj_$(ARCH)_opt:
	mkdir $@

clean: clean_dbg clean_opt

clean_dbg: obj_$(ARCH)_dbg
	@$(MAKE) TAG=dbg -C . -f $(TAR).mk clean
	rm -rf $<

clean_opt: obj_$(ARCH)_opt
	@$(MAKE) TAG=opt -C . -f $(TAR).mk clean
	rm -rf $<

