CXX = $(CROSS_COMPILE)gcc
ARCH = $(ARCH)
CXXFLAGS = -Wall -Wno-unknown-pragmas -Iinc -Llib -O6
DBG = -g
DEFINES ?= 
#LDFLAGS = -lpthread -lrt -lhb-file -lhrm-file
LDFLAGS = -static -lpthread -lrt -lhb-shared$(ARCH) -lhrm-shared$(ARCH)

DOCDIR = doc
BINDIR = bin_$(ARCH)
LIBDIR = lib
INCDIR = ./inc
SCRATCH = ./scratch
OUTPUT = ./output
SRCDIR = ./src
ROOTS = heart_rate_monitor_test
TEST_ROOTS = test1 test2
BINS = $(ROOTS:%=$(BINDIR)/%)
TESTS = $(TEST_ROOTS:%=$(BINDIR)/%)
OBJS = $(ROOTS:%=$(BINDIR)/%.o)
TEST_OBJS = $(TEST_ROOTS:%=$(BINDIR)/%.o)

all: hb-shared$(ARCH)

$(BINDIR):
	-mkdir -p $(BINDIR)

$(LIBDIR):
	-mkdir -p $(LIBDIR)

$(SCRATCH):
	-mkdir -p $(SCRATCH)

$(OUTPUT):
	-mkdir -p $(OUTPUT)


$(BINDIR)/%.o : $(SRCDIR)/%.c
	$(CXX) -c $(CXXFLAGS) $(DEFINES) $(DBG) -o $@ $<


$(BINS) : $(OBJS) 

$(BINS) : % : %.o
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

$(TESTS) : $(TEST_OBJS) 

$(TESTS) : % : %.o
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)



# Heartbeat shared memory version
hb-shared$(ARCH): $(BINDIR) $(LIBDIR) $(SCRATCH) hblib-shared$(ARCH) $(OUTPUT) $(BINS)

hblib-shared$(ARCH): $(LIBDIR)/libhb-shared$(ARCH).a $(LIBDIR)/libhrm-shared$(ARCH).a

$(LIBDIR)/libhb-shared$(ARCH).a: $(SRCDIR)/heartbeat-shared.c $(INCDIR)/heartbeat.h
	$(MAKE) $(BINDIR)/heartbeat-shared.o
	ar r $(LIBDIR)/libhb-shared$(ARCH).a $(BINDIR)/heartbeat-shared.o
	ranlib $(LIBDIR)/libhb-shared$(ARCH).a

$(LIBDIR)/libhrm-shared$(ARCH).a: $(SRCDIR)/heart_rate_monitor-shared.c $(INCDIR)/heart_rate_monitor.h
	$(MAKE) $(BINDIR)/heart_rate_monitor-shared.o
	ar r $(LIBDIR)/libhrm-shared$(ARCH).a $(BINDIR)/heart_rate_monitor-shared.o
	ranlib $(LIBDIR)/libhrm-shared$(ARCH).a




## cleaning
clean:
	-rm -rf $(BINDIR) $(LIBDIR) $(SCRATCH) *.log *~ $(SRCDIR)/*~

squeaky: clean
	-rm -rf $(OUTPUT)

# Documentation
documentation: 
	doxygen heartbeats_doc

clean-documentation:
	-rm -rf $(DOCDIR)/html/*
	-rm -rf $(DOCDIR)/latex/*
