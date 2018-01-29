
#########################################################
# Sources (updete if new directory with sources is added#
#########################################################
     
SRCS_DAEMONS = $(wildcard src/daemons/*.cc)

##############
# Other stuff#
##############

BINS_DAEMONS = $(patsubst src/daemons/%.cc,bin_$(ARCH)_$(PLAT)/daemons/%,$(SRCS_DAEMONS))

ifeq ($(DAEMONS),)
BINS_DAEMONS_FILTERED = $(BINS_DAEMONS)
else
_subst_comma:= ,
DAEMONS_FILTER=%$(subst $(_subst_comma), %,$(DAEMONS))
BINS_DAEMONS_FILTERED = $(filter $(DAEMONS_FILTER),$(BINS_DAEMONS))
endif

ifeq ($(BINS_DAEMONS_FILTERED),)
$(info No daemons to build. Is the filter correct ?)
endif
     
bin_$(ARCH)_$(PLAT)/daemons/%: src/daemons/%.cc lib_$(ARCH)_$(PLAT)/libruntime.a lib_$(ARCH)_$(PLAT)/libcore.a lib_$(ARCH)_$(PLAT)/libcpulimit.a
	$(CXX) -static $(CXXFLAGS) $^ -o $@
	
bin_$(ARCH)_$(PLAT)/daemons:
	mkdir -p bin_$(ARCH)_$(PLAT)/daemons

.PHONY: daemons
daemons: bin_$(ARCH)_$(PLAT)/daemons $(BINS_DAEMONS_FILTERED)

.PHONY: daemons_clean
daemons_clean:
	rm -rf bin_$(ARCH)_$(PLAT)/daemons