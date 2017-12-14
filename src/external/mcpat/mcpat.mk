TARGET = mcpat
TARGET_LIB = lib$(TARGET).a
SHELL = /bin/sh
.PHONY: all depend clean
.SUFFIXES: .cc .o

ifndef NTHREADS
  NTHREADS = 4
endif


LIBS = 
INCS = -lm

ifeq ($(TAG),dbg)
  DBG = -Wall 
  OPT = -ggdb -g -O0 -DNTHREADS=1 -Icacti
else
  DBG = 
  OPT = -O3 -msse2 -mfpmath=sse -DNTHREADS=$(NTHREADS)
  #OPT = -O0 -DNTHREADS=$(NTHREADS)
endif

#CXXFLAGS = -Wall -Wno-unknown-pragmas -Winline $(DBG) $(OPT) 
CXXFLAGS = -std=c++11 -Wno-unknown-pragmas $(DBG) $(OPT) 
#CXX = g++ -m32 -L/usr/lib32
#CC  = gcc -m32 -L/usr/lib32
CXX = ${CROSS_COMPILE}g++
CC  = ${CROSS_COMPILE}gcc

VPATH = cacti

SRCS  = \
  Ucache.cc \
  XML_Parse.cc \
  arbiter.cc \
  area.cc \
  array.cc \
  bank.cc \
  basic_circuit.cc \
  basic_components.cc \
  cacti_interface.cc \
  component.cc \
  core.cc \
  crossbar.cc \
  decoder.cc \
  htree2.cc \
  interconnect.cc \
  io.cc \
  iocontrollers.cc \
  logic.cc \
  mat.cc \
  memoryctrl.cc \
  noc.cc \
  nuca.cc \
  parameter.cc \
  processor.cc \
  router.cc \
  sharedcache.cc \
  subarray.cc \
  technology.cc \
  uca.cc \
  wire.cc \
  xmlParser.cc \
  powergating.cc
  
SRCS_MAIN  = \
  main.cc

OBJS = $(patsubst %.cc,obj_$(TAG)/%.o,$(SRCS))
OBJS_MAIN = $(patsubst %.cc,obj_$(TAG)/%.o,$(SRCS_MAIN))

all: $(TARGET)

$(TARGET) : $(OBJS_MAIN) lib/$(TARGET_LIB)
	$(CXX) $(OBJS_MAIN) -o $@ $(INCS) $(CXXFLAGS) $(LIBS) -L./lib -l$(TARGET) -pthread

lib/$(TARGET_LIB) : $(OBJS)
	ar rvs $@ $(OBJS)

#obj_$(TAG)/%.o : %.cc
#	$(CXX) -c $(CXXFLAGS) $(INCS) -o $@ $<

obj_$(TAG)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-rm -f *.o $(TARGET)
	rm -rf lib


