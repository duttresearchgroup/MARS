##########################################################
# Configuration variables used by the scripts
############################################################


###########################################
# Build confs

# Arch/Plat of the binaries to be used
export RTS_ARCH=arm
export RTS_PLAT=exynos5422


###########################################
# Confs for remote*.sh scripts

# Remote user credentials
export R_USER=odroid
export R_PASS=odroid

# Remote host ip address
export R_HOST=128.195.55.158

# Build dir in the remote host
export R_SPARTA_ROOT=/home/odroid/sparta_dev

# Uncomment for verbose remote commands
# export R_VERBOSE=-v


###########################################
# Confs for tracing scripts

# perfcnts to trace
# (note: intrs and busyCy are always enabled when tracing,
#        so it's not necessary to include in the list)
TRACE_PERFCNTS=(brMisspred)
#TRACE_PERFCNTS=(branchInstr memRdWrInstr memRdInstr memWrInstr brMisspred dtlbMiss itlbMiss l1dcAccess l1dcMiss l1icAccess l1icMiss llcAccess llcMiss busAccess busCy)

# Maximum number of perfcnts to collect in a single run.
# If the number of counters listed by TRACE_PERFCNTS is
# greater than this number, multiple runs will be used 
# to collect all counters.
# This number cannot be greater than the maximum counters
# supported by the platform
TRACE_MAXPERFCNT=3 

# Number of times to run the same program.
# Multiple runs can be combined to generate
# a trace with the average measurements across
# all runs
TRACE_RUNITERS=1

# Where the traces are stored
TRACE_OUTPUT_DIR=$SPARTA_ROOT/traces


