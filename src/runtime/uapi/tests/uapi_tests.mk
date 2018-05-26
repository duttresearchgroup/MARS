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

SRCS_CXX_UAPI_TESTS = $(wildcard src/runtime/uapi/tests/*.cc)


##############
# Other stuff#
##############

BINS_UAPI_TESTS = $(patsubst src/runtime/uapi/tests/%.cc,bin_$(ARCH)_$(PLAT)/uapitests/%,$(SRCS_CXX_UAPI_TESTS))


bin_$(ARCH)_$(PLAT)/uapitests/%: src/runtime/uapi/tests/%.cc
	$(CXX) -static $(CXXFLAGS) $< -o $@
	
bin_$(ARCH)_$(PLAT)/uapitests:
	mkdir -p bin_$(ARCH)_$(PLAT)/uapitests 

.PHONY: uapi_tests
uapi_tests: bin_$(ARCH)_$(PLAT)/uapitests $(BINS_UAPI_TESTS)

.PHONY: clean_uapi_tests
clean_uapi_tests:
	rm -rf bin_$(ARCH)_$(PLAT)/uapitests
	
