
#########################################################
# Sources (updete if new directory with sources is added#
#########################################################
    
SRCS_CXX_APPS_COMMON = $(wildcard src/apps/common/*.cc) 
SRCS_CXX_APPS = $(wildcard src/apps/*.cc)


##############
# Other stuff#
##############

OBJS_CXX_APPS_COMMON = $(patsubst %.cc,obj_$(ARCH)_$(PLAT)/%.o,$(SRCS_CXX_APPS_COMMON))

OBJS_DEPS += $(OBJS_CXX_APPS_COMMON:%.o=%.d)

APPS_LIBS_DEPS = lib_$(ARCH)_$(PLAT)/libsasolver.a	
APPS_LIBS_DEPS += lib_$(ARCH)_$(PLAT)/libofflinesim.a 	
APPS_LIBS_DEPS += lib_$(ARCH)_$(PLAT)/libruntime.a
APPS_LIBS_DEPS += lib_$(ARCH)_$(PLAT)/liblinsched.a 
APPS_LIBS_DEPS += lib_$(ARCH)_$(PLAT)/libcore.a
APPS_LIBS_DEPS += lib_$(ARCH)_$(PLAT)/libmcpat.a

bin_$(ARCH)_$(PLAT)/apps/%: src/apps/%.cc $(OBJS_CXX_APPS_COMMON) $(APPS_LIBS_DEPS)
	$(CXX) $(CXXFLAGS) $^ -o $@
	
bin_$(ARCH)_$(PLAT)/apps:
	mkdir -p bin_$(ARCH)_$(PLAT)/apps

BINS_APPS = $(patsubst src/apps/%.cc,bin_$(ARCH)_$(PLAT)/apps/%,$(SRCS_CXX_APPS))

.PHONY: apps
apps: bin_$(ARCH)_$(PLAT)/apps $(BINS_APPS)

.PHONY: apps_clean
apps_clean:
	rm -rf bin_$(ARCH)_$(PLAT)/apps $(OBJS_CXX_APPS_COMMON) $(OBJS_CXX_APPS_COMMON:%.o=%.d)

