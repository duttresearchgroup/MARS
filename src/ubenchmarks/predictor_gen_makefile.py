#!/usr/bin/python
import sys

predictors=(
"\n"
".PHONY: predictors\n"
"predictors: "
)

predNameTemplate="$(BINDIR)/XXpredictornameXX "
makePredTemplate=(
"\n"
"$(BINDIR)/XXpredictornameXX: $(OBJDIR)/predictor.o $(KERN_OBJS) $(SRCDIR)/predictor_main.cc\n"
"\t$(CC) $(CFLAGS) -static -DITER_high_ilp_cache_good_int=XXhigh_ilp_cache_good_intXX -DITER_high_ilp_cache_bad_int=XXhigh_ilp_cache_bad_intXX -DITER_low_ilp_cache_good_int=XXlow_ilp_cache_good_intXX -DITER_low_ilp_cache_bad_int=XXlow_ilp_cache_bad_intXX -DITER_low_ilp_cache_good_float=XXlow_ilp_cache_good_floatXX -DITER_low_ilp_cache_bad_float=XXlow_ilp_cache_bad_floatXX -DITER_high_ilp_cache_good_float=XXhigh_ilp_cache_good_floatXX -DITER_high_ilp_cache_bad_float=XXhigh_ilp_cache_bad_floatXX -DITER_low_ilp_icache_bad=XXlow_ilp_icache_badXX -DITER_low_ilp_branches_deep=XXlow_ilp_branches_deepXX -DITER_matrix_mult=0 $(SRCDIR)/predictor_main.cc $(OBJDIR)/predictor.o $(KERN_OBJS) -o $(BINDIR)/XXpredictornameXX\n"
)

make  = "\n"

benchname = []
benchIters = []

benchname.append("high_ilp_cache_good_int")
benchIters.append(["0","3840000","7680000"])

benchname.append("high_ilp_cache_bad_int")
benchIters.append(["0","480000"])

benchname.append("low_ilp_cache_good_int")
benchIters.append(["0","1290000"])

benchname.append("low_ilp_cache_bad_int")
benchIters.append(["0","990000"])

benchname.append("low_ilp_cache_good_float")
benchIters.append(["0","1070000"])

benchname.append("low_ilp_cache_bad_float")
benchIters.append(["0","880000"])

benchname.append("high_ilp_cache_good_float")
benchIters.append(["0","2560000"])

benchname.append("high_ilp_cache_bad_float")
benchIters.append(["0","480000"])

benchname.append("low_ilp_icache_bad")
benchIters.append(["0","1900000"])

benchname.append("low_ilp_branches_deep")
benchIters.append(["0","3500000"])

predIdx = 0
def combine(benchIdx,template):
    if(benchIdx >= len(benchname)):
        global predictors
        global predIdx
        global predNameTemplate
        global make
        predName = "predictor{:04d}".format(predIdx)
        template = template.replace("XXpredictornameXX", predName)
        make += template
        predictors += predNameTemplate.replace("XXpredictornameXX", predName)
        predIdx += 1
    else:
        for nIter in benchIters[benchIdx]:
            name = benchname[benchIdx]
            templateNew = template.replace("XX"+name+"XX",nIter)
            combine(benchIdx+1,templateNew)

combine(0,makePredTemplate)

make = predictors + make

print(make)            



    
