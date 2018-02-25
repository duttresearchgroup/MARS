#!/usr/bin/python
#-------------------------------------------------------------------------------
# Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#-------------------------------------------------------------------------------

import sys

training_apps=(
"\n"
".PHONY: training_apps\n"
"training_apps: "
)

predNameTemplate="$(BINDIR)/XXtrainingnameXX "
makePredTemplate=(
"\n"
"$(BINDIR)/XXtrainingnameXX: $(OBJDIR)/training.o $(KERN_OBJS) $(SRCDIR)/training_singleapp.cc\n"
"\t$(CC) $(CFLAGS) -static -DITER_high_ilp_cache_good_int=XXhigh_ilp_cache_good_intXX -DITER_high_ilp_cache_bad_int=XXhigh_ilp_cache_bad_intXX -DITER_low_ilp_cache_good_int=XXlow_ilp_cache_good_intXX -DITER_low_ilp_cache_bad_int=XXlow_ilp_cache_bad_intXX -DITER_low_ilp_cache_good_float=XXlow_ilp_cache_good_floatXX -DITER_low_ilp_cache_bad_float=XXlow_ilp_cache_bad_floatXX -DITER_high_ilp_cache_good_float=XXhigh_ilp_cache_good_floatXX -DITER_high_ilp_cache_bad_float=XXhigh_ilp_cache_bad_floatXX -DITER_low_ilp_icache_bad=XXlow_ilp_icache_badXX -DITER_low_ilp_branches_deep=XXlow_ilp_branches_deepXX -DITER_matrix_mult=0 $(SRCDIR)/training_singleapp.cc $(OBJDIR)/training.o $(KERN_OBJS) -o $(BINDIR)/XXtrainingnameXX\n"
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
        global training_apps
        global predIdx
        global predNameTemplate
        global make
        predName = "training{:04d}".format(predIdx)
        template = template.replace("XXtrainingnameXX", predName)
        make += template
        training_apps += predNameTemplate.replace("XXtrainingnameXX", predName)
        predIdx += 1
    else:
        for nIter in benchIters[benchIdx]:
            name = benchname[benchIdx]
            templateNew = template.replace("XX"+name+"XX",nIter)
            combine(benchIdx+1,templateNew)

combine(0,makePredTemplate)

make = training_apps + make

print(make)            



    
