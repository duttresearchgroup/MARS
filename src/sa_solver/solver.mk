TARGET = sasolver
TARGET_LIB_HOST = lib$(TARGET)host.a
TARGET_LIB_ALPHA = lib$(TARGET)alpha.a
TARGET_LIB_ARM = lib$(TARGET)arm.a
SHELL = /bin/sh
.PHONY: all depend clean
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

library_host: lib/$(TARGET_LIB_HOST)

library_alpha: lib/$(TARGET_LIB_ALPHA)

library_arm: lib/$(TARGET_LIB_ARM)

$(TARGET) : $(OBJS_MAIN) lib/$(TARGET_LIB_HOST) $(HEADERS)
	$(CXX) $(OBJS_MAIN) -o $@ $(CXXFLAGS) -L./lib -l$(TARGET)host -pthread

lib:
	mkdir lib

lib/$(TARGET_LIB_HOST) : $(OBJS) $(HEADERS) lib
	$(AR) rvs $@ $(OBJS)
	
lib/$(TARGET_LIB_ALPHA) : $(OBJS) $(HEADERS) lib
	$(AR) rvs $@ $(OBJS)

lib/$(TARGET_LIB_ARM) : $(OBJS) $(HEADERS) lib
	$(AR) rvs $@ $(OBJS)

obj_$(ARCH)_$(TAG)/%.o : %.cc
	$(CXX) $(KERNEL) $(CXXFLAGS) -c $< -o $@

clean:
	-rm -f *.o $(TARGET)
	rm -f lib/$(TARGET_LIB_HOST)
	rm -f lib/$(TARGET_LIB_ALPHA)


