CC=$(CROSS_COMPILE)g++
AS=$(CROSS_COMPILE)as
LD=$(CROSS_COMPILE)ld

SRCDIR=src/ubenchmarks
BINDIR=bin_$(ARCH)_$(PLAT)/ubenchmarks
BINDIR_PRED=$(BINDIR)/predictors
OBJDIR=obj_$(ARCH)_$(PLAT)/ubenchmarks

CFLAGS=-O2 -Wall -Werror -std=c++11 $(EXTRAFLAGS)
OBJFLAGS=-static -pthread $(CFLAGS)

.PHONY: all clean veryclean predictors_clean predictors

all: test

clean:
	rm -rf $(BINDIR) $(OBJDIR)
	
predictors_clean:
	rm -rf $(BINDIR_PRED)

veryclean: predictors_clean clean

dirs: $(BINDIR) $(OBJDIR) $(BINDIR_PRED)
$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)
$(BINDIR_PRED):
	mkdir -p $(BINDIR_PRED)

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
export KERN_OBJS=$(KERN_ICACHE_OBJ) #needed by makefile.predictors and we cannot pass as param so we use export

test: $(BINDIR)/mem_test $(BINDIR)/ubenchmark_periodic $(BINDIR)/ubenchmark_timed $(BINDIR)/predictor_test

predictor_test: $(BINDIR)/predictor_test
	perf stat -e cycles,instructions,cache-references,cache-misses,branches,branch-misses -r 10 $(BINDIR)/predictor_test_noprint

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

$(BINDIR)/predictor_test: $(OBJDIR)/predictor.o $(KERN_OBJS)
	$(CC) $(CFLAGS) -static -DITER_PRINT -DITER_high_ilp_cache_good_int=500 -DITER_high_ilp_cache_bad_int=500 -DITER_low_ilp_cache_good_int=500 -DITER_low_ilp_cache_bad_int=500 -DITER_low_ilp_cache_good_float=500 -DITER_low_ilp_cache_bad_float=500 -DITER_high_ilp_cache_good_float=500 -DITER_high_ilp_cache_bad_float=500 -DITER_low_ilp_icache_bad=500 -DITER_low_ilp_branches_deep=500 -DITER_matrix_mult=50 $(SRCDIR)/predictor_main.cc $^ -o $@
	$(CC) $(CFLAGS) -static -DITER_high_ilp_cache_good_int=500 -DITER_high_ilp_cache_bad_int=500 -DITER_low_ilp_cache_good_int=500 -DITER_low_ilp_cache_bad_int=500 -DITER_low_ilp_cache_good_float=500 -DITER_low_ilp_cache_bad_float=500 -DITER_high_ilp_cache_good_float=500 -DITER_high_ilp_cache_bad_float=500 -DITER_low_ilp_icache_bad=500 -DITER_low_ilp_branches_deep=500 -DITER_matrix_mult=50 $(SRCDIR)/predictor_main.cc $^ -o $(BINDIR)/predictor_test_noprint

$(BINDIR)/predictor_master: $(OBJDIR)/predictor.o $(KERN_OBJS) $(SRCDIR)/predictor_master.cc
	$(CC) $(CFLAGS) -static $^ -o $@

predictors: $(OBJDIR)/predictor.o $(KERN_OBJS) $(OBJDIR)/makefile.predictors $(BINDIR)/predictor_master
	@$(MAKE) CC=$(CC) BINDIR=$(BINDIR_PRED) OBJDIR=$(OBJDIR) SRCDIR=$(SRCDIR) -C . -f $(OBJDIR)/makefile.predictors predictors

$(OBJDIR)/makefile.predictors: $(SRCDIR)/predictor_gen_makefile.py | dirs
	python $(SRCDIR)/predictor_gen_makefile.py > $(OBJDIR)/makefile.predictors 

