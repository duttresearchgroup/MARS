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
#export R_VERBOSE=-v


###########################################
# Confs for tracing scripts

# perfcnts to trace
# (note: intrs and busyCy are always enabled when tracing,
#        so it's not necessary to include in the list)
export TRACE_PERFCNTS=(brMisspred)
#export TRACE_PERFCNTS=(branchInstr memRdWrInstr memRdInstr memWrInstr brMisspred dtlbMiss itlbMiss l1dcAccess l1dcMiss l1icAccess l1icMiss llcAccess llcMiss busAccess busCy)

# Maximum number of perfcnts to collect in a single run.
# If the number of counters listed by TRACE_PERFCNTS is
# greater than this number, multiple runs will be used 
# to collect all counters.
# This number cannot be greater than the maximum counters
# supported by the platform
export TRACE_MAXPERFCNT=3 

# Number of times to run the same program.
# Multiple runs can be combined to generate
# a trace with the average measurements across
# all runs
export TRACE_RUNITERS=4

# Where the traces are stored
export TRACE_OUTPUT_DIR=$SPARTA_ROOT/traces
export R_TRACE_OUTPUT_DIR=$R_SPARTA_ROOT/traces

