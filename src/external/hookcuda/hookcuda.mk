TARGET = hookcuda
TARGET_LIB = lib$(TARGET).so
SHELL = /bin/sh
.PHONY: all depend clean
.SUFFIXES: .cc .o

ifndef NTHREADS
  NTHREADS = 4
endif

TARGET_CUDA_HOME ?= $(CUDA_HOME)
LIBS = -shared -L$(TARGET_CUDA_HOME)/lib64 -L$(TARGET_CUDA_HOME)/lib64/stubs -L$(TARGET_CUDA_HOME)/extras/CUPTI/lib64 -lcuda -lcupti -ldl -lrt -lpthread -lrt
INCS = -lm

ifeq ($(TAG),dbg)
  DBG = -Wall 
  OPT = -ggdb -g -O0 -DNTHREADS=1 -Icacti
else
  DBG = 
  OPT = -O3 -msse2 -mfpmath=sse -DNTHREADS=$(NTHREADS)
endif

CXXFLAGS = -ccbin aarch64-unknown-linux-gnu-g++ -Xcompiler "-fPIC -O2 -std=c++0x" -O2 -I$(TARGET_CUDA_HOME)/include -I$(TARGET_CUDA_HOME)/extras/CUPTI/include -arch=sm_62

CXX = nvcc
CC  = nvcc

SRCS  = \
  hookcuda.cc \
  cupti_wrapper.cc \
  nvidia_counters.cc
  
OBJS = $(patsubst %.cc,obj_$(TAG)/%.o,$(SRCS))

$(TARGET_LIB) : $(OBJS)
	$(CXX) $(OBJS_MAIN) -o $@ $^ $(INCS) $(CXXFLAGS) $(LIBS)

obj_$(TAG)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-rm -rf obj_$(TAG) $(TARGET_LIB)


