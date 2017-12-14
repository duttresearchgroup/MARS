
#########################################################
# Sources (updete if new directory with sources is added#
#########################################################
     
SRCS_DAEMONS = $(wildcard src/daemons/*.cc)

##############
# Other stuff#
##############

BINS_DAEMONS = $(patsubst src/daemons/%.cc,bin_$(ARCH)_$(PLAT)/daemons/%,$(SRCS_DAEMONS))
     
bin_$(ARCH)_$(PLAT)/daemons/%: src/daemons/%.cc lib_$(ARCH)_$(PLAT)/libruntime.a lib_$(ARCH)_$(PLAT)/libcore.a lib_$(ARCH)_$(PLAT)/libcpulimit.a
	$(CXX) -static $(CXXFLAGS) $^ -o $@
	
bin_$(ARCH)_$(PLAT)/daemons:
	mkdir -p bin_$(ARCH)_$(PLAT)/daemons

.PHONY: daemons
daemons: bin_$(ARCH)_$(PLAT)/daemons $(BINS_DAEMONS)

.PHONY: daemons_clean
daemons_clean:
	rm -rf bin_$(ARCH)_$(PLAT)/daemons