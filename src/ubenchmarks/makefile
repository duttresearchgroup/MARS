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

CC=$(CROSS_COMPILE)g++
AS=$(CROSS_COMPILE)as
LD=$(CROSS_COMPILE)ld

SRCDIR=src/ubenchmarks
BINDIR=bin_$(ARCH)_$(PLAT)/ubenchmarks
BINDIR_TRAIN=$(BINDIR)/training
OBJDIR=obj_$(ARCH)_$(PLAT)/ubenchmarks

CFLAGS=-O2 -Wall -Werror -std=c++11 $(EXTRAFLAGS)
OBJFLAGS=-static -pthread $(CFLAGS)

.PHONY: all clean veryclean trainingapps_clean training_test training_multiapp training_singleapp

all: test

clean:
	rm -rf $(BINDIR) $(OBJDIR)
	
trainingapps_clean:
	rm -rf $(BINDIR_TRAIN)

veryclean: trainingapps_clean clean

dirs: $(BINDIR) $(OBJDIR) $(BINDIR_TRAIN)
$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)
$(BINDIR_TRAIN):
	mkdir -p $(BINDIR_TRAIN)

$(OBJDIR)/%.o: $(SRCDIR)/%.cc | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/big_switch.h: $(SRCDIR)/gen_big_switch.py | dirs
	python $(SRCDIR)/gen_big_switch.py > $(OBJDIR)/big_switch.h

$(OBJDIR)/kernels_icache.o: $(OBJDIR)/big_switch.h $(SRCDIR)/kernels_icache.cc | dirs
	$(CC) -I$(OBJDIR) $(CFLAGS) -c $(SRCDIR)/kernels_icache.cc -o $(OBJDIR)/kernels_icache.o

KERN_INT_SRC=$(SRCDIR)/kernels_macros.cc $(SRCDIR)/kernels_int.cc
KERN_FLOAT_SRC=$(KERN_INT_SRC) $(SRCDIR)/kernels_float.cc
KERN_TIMED_SRC=$(KERN_FLOAT_SRC) $(SRCDIR)/kernels_timed.cc
KERN_ICACHE_SRC=$(KERN_TIMED_SRC) $(SRCDIR)/kernels_icache.cc

KERN_INT_OBJ=$(patsubst $(SRCDIR)/%.cc,$(OBJDIR)/%.o,$(KERN_INT_SRC))
KERN_FLOAT_OBJ=$(patsubst $(SRCDIR)/%.cc,$(OBJDIR)/%.o,$(KERN_FLOAT_SRC))
KERN_TIMED_OBJ=$(patsubst $(SRCDIR)/%.cc,$(OBJDIR)/%.o,$(KERN_TIMED_SRC))
KERN_ICACHE_OBJ=$(patsubst $(SRCDIR)/%.cc,$(OBJDIR)/%.o,$(KERN_ICACHE_SRC))
export KERN_OBJS=$(KERN_ICACHE_OBJ) #needed by makefile.training_multiapp and we cannot pass as param so we use export

test: $(BINDIR)/mem_test $(BINDIR)/ubenchmark_periodic $(BINDIR)/ubenchmark_timed $(BINDIR)/training_test

training_test: $(BINDIR)/training_test
	perf stat -e cycles,instructions,cache-references,cache-misses,branches,branch-misses -r 10 $(BINDIR)/training_test_noprint

$(BINDIR)/mem_test: $(OBJDIR)/mem_test.o
	$(CC) $(OBJFLAGS) $^ -o $@

$(BINDIR)/strider: $(OBJDIR)/strider.o
	$(CC) $(OBJFLAGS) $^ -o $@

$(BINDIR)/strider2: $(OBJDIR)/strider2.o
	$(CC) $(OBJFLAGS) $^ -o $@

$(BINDIR)/ubenchmark_periodic: $(SRCDIR)/ubenchmark_periodic.cc $(OBJDIR)/ubenchmark_periodic_main.o $(OBJDIR)/ubenchmark_funcs.o $(KERN_INT_OBJ)
	$(CC) $(OBJFLAGS) $^ -o $@

$(BINDIR)/ubenchmark_periodic_beats: $(SRCDIR)/ubenchmark_periodic.cc $(OBJDIR)/ubenchmark_periodic_main.o $(OBJDIR)/ubenchmark_funcs.o $(KERN_INT_OBJ)
	$(CC) -DHAS_BEATS -I../mash/src $(OBJFLAGS) $^ -o $@

$(BINDIR)/ubenchmark_timed: $(OBJDIR)/ubenchmark_timed.o $(OBJDIR)/ubenchmark_timed_main.o $(OBJDIR)/ubenchmark_funcs.o $(KERN_INT_OBJ)
	$(CC) $(OBJFLAGS) $^ -o $@	

$(BINDIR)/training_test: $(OBJDIR)/training.o $(KERN_OBJS)
	$(CC) $(CFLAGS) -static -DITER_PRINT -DITER_high_ilp_cache_good_int=500 -DITER_high_ilp_cache_bad_int=500 -DITER_low_ilp_cache_good_int=500 -DITER_low_ilp_cache_bad_int=500 -DITER_low_ilp_cache_good_float=500 -DITER_low_ilp_cache_bad_float=500 -DITER_high_ilp_cache_good_float=500 -DITER_high_ilp_cache_bad_float=500 -DITER_low_ilp_icache_bad=500 -DITER_low_ilp_branches_deep=500 -DITER_matrix_mult=50 $(SRCDIR)/training_multiapp.cc $^ -o $@
	$(CC) $(CFLAGS) -static -DITER_high_ilp_cache_good_int=500 -DITER_high_ilp_cache_bad_int=500 -DITER_low_ilp_cache_good_int=500 -DITER_low_ilp_cache_bad_int=500 -DITER_low_ilp_cache_good_float=500 -DITER_low_ilp_cache_bad_float=500 -DITER_high_ilp_cache_good_float=500 -DITER_high_ilp_cache_bad_float=500 -DITER_low_ilp_icache_bad=500 -DITER_low_ilp_branches_deep=500 -DITER_matrix_mult=50 $(SRCDIR)/training_multiapp.cc $^ -o $(BINDIR)/training_test_noprint

$(BINDIR)/training_singleapp: $(OBJDIR)/training.o $(KERN_OBJS) $(SRCDIR)/training_singleapp.cc
	$(CC) $(OBJFLAGS) -I../mash/src $^ -o $@

$(BINDIR)/training_singleapp_test: $(OBJDIR)/training.o $(KERN_OBJS) $(SRCDIR)/training_singleapp.cc
	$(CC) $(OBJFLAGS) -I../mash/src -D__ISLITE $^ -o $@

training_multiapp: $(OBJDIR)/training.o $(KERN_OBJS) $(OBJDIR)/makefile.training_multiapp
	@$(MAKE) CC=$(CC) BINDIR=$(BINDIR_TRAIN) OBJDIR=$(OBJDIR) SRCDIR=$(SRCDIR) -C . -f $(OBJDIR)/makefile.training_multiapp training_apps

training_singleapp: $(BINDIR)/training_singleapp $(BINDIR)/training_singleapp_test

$(OBJDIR)/makefile.training_multiapp: $(SRCDIR)/training_gen_makefile.py | dirs
	python $(SRCDIR)/training_gen_makefile.py > $(OBJDIR)/makefile.training_multiapp 

