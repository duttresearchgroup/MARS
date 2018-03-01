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

TARGET = sasolver
TARGET_LIB = lib$(TARGET)$(ARCH).a
SHELL = /bin/sh
.PHONY: all depend clean library
.SUFFIXES: .cc .o

ifeq ($(TAG),dbg)
  DBG = -Wall -Werror
  OPT = -g -O0
  KERNEL = 
else
  DBG = -Wall -Werror
  OPT = -O0 #-g
#  OPT = -O1 #\
-fno-omit-frame-pointer \
-fno-merge-constants \
-fno-zero-initialized-in-bss \
-fno-delete-null-pointer-checks #\
-fno-defer-pop
  KERNEL = -DNDEBUG #-nostdlib -nodefaultlibs -fno-rtti -fno-exceptions -DNDEBUG
endif
 
CXXFLAGS = $(DBG) $(OPT) $(EXTRAFLAGS)
CXX = $(CROSS_COMPILE)g++
CC  = $(CROSS_COMPILE)gcc 
AR  = $(CROSS_COMPILE)ar

HEADERS = \
	custom_math.h \
	custom_random.h \
	globals.h \
	globals_alloc.h \
	solver_defines.h \
	solver_cinterface.h \
	solver.h \
	solver_interface.h

SRCS  = \
	solver_cinterface.cc \
	globals.cc \
	custom_random.cc
    
SRCS_MAIN  = \
	testbench.cc

OBJS = $(patsubst %.cc,obj_$(ARCH)_$(TAG)/%.o,$(SRCS))
OBJS_MAIN = $(patsubst %.cc,obj_$(ARCH)_$(TAG)/%.o,$(SRCS_MAIN))

all: $(TARGET)

library: lib/$(TARGET_LIB)

$(TARGET) : $(OBJS_MAIN) lib/$(TARGET_LIB) $(HEADERS)
	$(CXX) $(OBJS_MAIN) lib/$(TARGET_LIB) -o $@ $(CXXFLAGS) -pthread

lib:
	mkdir lib

lib/$(TARGET_LIB) : $(OBJS) $(HEADERS) lib
	$(AR) rvs $@ $(OBJS)

obj_$(ARCH)_$(TAG)/%.o : %.cc
	$(CXX) $(KERNEL) $(CXXFLAGS) -c $< -o $@

clean:
	-rm -f *.o $(TARGET)
	rm -f lib/$(TARGET_LIB)


