
#########################################################
# Sources (updete if new directory with sources is added#
#########################################################

SRCS_CXX_SIM = $(wildcard src/offline_sim/*.cc)


##############
# Other stuff#
##############

OBJS_SIM = $(patsubst %.cc,obj_$(ARCH)_$(PLAT)/%.o,$(SRCS_CXX_SIM))

OBJS_DEPS += $(OBJS_SIM:%.o=%.d)

lib_$(ARCH)_$(PLAT)/libofflinesim.a: $(OBJS_SIM)  
	mkdir -p lib_$(ARCH)_$(PLAT)
	$(AR) rvs $@  $(OBJS_SIM)

.PHONY: offline_sim_lib
offline_sim_lib: lib_$(ARCH)_$(PLAT)/libofflinesim.a lib_$(ARCH)_$(PLAT)/libmcpat.a lib_$(ARCH)_$(PLAT)/liblinsched.a

.PHONY: offline_sim_lib_clean
offline_sim_lib_clean:
	rm -f lib_$(ARCH)_$(PLAT)/libofflinesim.a $(OBJS_SIM) $(OBJS_SIM:%.o=%.d)
	

