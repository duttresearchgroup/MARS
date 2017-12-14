
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
	
