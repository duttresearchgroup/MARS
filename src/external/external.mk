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

.PHONY: external_libs
external_libs: lib_$(ARCH)_$(PLAT)/libcpulimit.a lib_$(ARCH)_$(PLAT)/libmcpat.a lib_$(ARCH)_$(PLAT)/liblinsched.a

.PHONY: external_clean
external_clean: ext_cpulimit_clean ext_mcpat_clean ext_linsched_clean
	rm -f lib_$(ARCH)_$(PLAT)/libcpulimit.a
	rm -f lib_$(ARCH)_$(PLAT)/liblinsched.a
	rm -f lib_$(ARCH)_$(PLAT)/libmcpat.a

## External with not per arch lib/objs support,
## so we always make sure to clean before building
## a new lib.
## Note the whole thing won't rebuild automatically
## if external code is modified 	 

## cpulimit ##

.INTERMEDIATE: src/external/cpulimit/src/libcpulimit.a	

lib_$(ARCH)_$(PLAT)/libcpulimit.a: src/external/cpulimit/src/libcpulimit.a
	mkdir -p lib_$(ARCH)_$(PLAT)
	cp src/external/cpulimit/src/libcpulimit.a $@
	rm src/external/cpulimit/src/libcpulimit.a
	rm src/external/cpulimit/src/cpulimit

src/external/cpulimit/src/libcpulimit.a:
	cd src/external/cpulimit/src; make CC=$(CC) AR=$(AR) clean
	cd src/external/cpulimit/src; make CC=$(CC) AR=$(AR) cpulimit

PHONY: ext_cpulimit_clean
ext_cpulimit_clean:
	cd src/external/cpulimit/src; make CC=$(CC) AR=$(AR) clean
	
## linsched ##

.INTERMEDIATE: src/external/linsched/tools/linsched/liblinsched.a

lib_$(ARCH)_$(PLAT)/liblinsched.a: src/external/linsched/tools/linsched/liblinsched.a
	mkdir -p lib_$(ARCH)_$(PLAT)
	cp src/external/linsched/tools/linsched/liblinsched.a $@
	rm src/external/linsched/tools/linsched/liblinsched.a
	
src/external/linsched/tools/linsched/liblinsched.a:
	$(MAKE) clean -C src/external/linsched/tools/linsched
	$(MAKE) CROSS_COMPILE=$(CROSS_COMPILE) lib -C src/external/linsched/tools/linsched

PHONY: ext_linsched_clean
ext_linsched_clean:
	$(MAKE) clean -C src/external/linsched/tools/linsched



## mcpat ##

.INTERMEDIATE: src/external/mcpat/lib/libmcpat.a

lib_$(ARCH)_$(PLAT)/libmcpat.a: src/external/mcpat/lib/libmcpat.a
	mkdir -p lib_$(ARCH)_$(PLAT)
	cp src/external/mcpat/lib/libmcpat.a $@
	rm src/external/mcpat/lib/libmcpat.a

src/external/mcpat/lib/libmcpat.a:
	$(MAKE) clean -C src/external/mcpat
	$(MAKE) CROSS_COMPILE=$(CROSS_COMPILE) -C src/external/mcpat

PHONY: ext_mcpat_clean
ext_mcpat_clean:
	$(MAKE) clean -C src/external/mcpat

	
