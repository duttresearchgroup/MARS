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

OBJS_UAPI_TESTS = $(patsubst %.cc,obj_$(ARCH)/%.o,$(SRCS_CXX_UAPI_TESTS))

OBJS_DEPS += $(OBJS_UAPI_TESTS:%.o=%.d)

BINS_UAPI_TESTS = $(patsubst %.cc,%.uapitest,$(SRCS_CXX_UAPI_TESTS))

src/runtime/uapi/tests/%.uapitest : obj_$(ARCH)/src/runtime/uapi/tests/%.o
	$(CXX) -static $(CXXFLAGS) $< -o $@

.PHONY: uapi_tests
uapi_tests: $(BINS_UAPI_TESTS)

.PHONY: clean_uapi_tests
clean_uapi_tests:
	rm -f $(OBJS_UAPI_TESTS) $(BINS_UAPI_TESTS)
	
