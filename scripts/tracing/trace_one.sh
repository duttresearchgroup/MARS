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

########################################################
# Traces a single process / single threaded application.
# Parameters:
#   1) Trace app name
#   2) Core where the traced process executes
#   3) Core frequency (in kHz)
#   4) Path to traced application binary.
#      Remaining parameters are passed as the application
#      parameters
# As a side effect, all cores will be using the ondemand
# dvfs governor after running this script.
#
# Usage:
#   trace_one.sh <trace_app_name> <traced_core> <traced_core_freq_kHz> <traced_app_bin_path> <traced_app_args ...>
#########################################################

TRACE_NAME=$1
shift
TRACED_CORE=$1
shift
TRACE_FREQUENCY=$1
shift
TRACED_PROGRAM=$1
shift
TRACED_PROGRAM_ARGS="$@"

# Source the files we need
source $MARS_SCRIPTDIR/runtime/common.sh

# Trace directory according for the target configuration
TGT_TRACES_DIR=$TRACE_OUTPUT_DIR-$RTS_ARCH-$RTS_PLAT
mkdir -p $TGT_TRACES_DIR

# We are gonna need idle powers later for parsing and a sys_info to figure
# out the path to store the traces,  so make sure it's collected now before
# we start
IDLE_POWER_DIR=$TGT_TRACES_DIR/idle_power
if [ ! -d $IDLE_POWER_DIR ]
then
    echo "Idle power directory $IDLE_POWER_DIR is needed for tracing"
    echo "Running get_idle_power.sh before tracing"
    sh $MARS_SCRIPTDIR/tracing/get_idle_power.sh $IDLE_POWER_DIR
    if [ ! -d $IDLE_POWER_DIR ]
    then
        echo "Error geting idle power !"
        exit 1
    fi
fi


# Where to store this program trace (cleans it up if already exists)
#     - Creates a path with name--trace_core--trace_freq
#     - Uses a python script that uses the sys_info.json to replace the
#       core num. by the core arch
RAW_OUTPUT_DIR=$TGT_TRACES_DIR/$TRACE_NAME--$TRACED_CORE--$TRACE_FREQUENCY.raw
RAW_OUTPUT_DIR=$(python3 $MARS_SCRIPTDIR/tracing/trace_name.py --input_file $RAW_OUTPUT_DIR --sysinfo $IDLE_POWER_DIR/sys_info.json)
rm -rf $RAW_OUTPUT_DIR
mkdir $RAW_OUTPUT_DIR

# dmesg is gonna be cleaned up after every run, so store current content
# for debug
PREV_DMESG=$RAW_OUTPUT_DIR/trace_dmesg--prev.txt
sudo dmesg -c > $PREV_DMESG


# Set the governor of all cores to ondemand and the governor of the core we need to the given frequency
sudosh $MARS_SCRIPTDIR/tracing/trace_set_freq.sh $TRACED_CORE $TRACE_FREQUENCY ondemand

echo "Tracing " $TRACED_PROGRAM " at core " $TRACED_CORE " at freq " $(sudo cat /sys/devices/system/cpu/cpu$TRACED_CORE/cpufreq/cpuinfo_cur_freq)
echo "  storing files at $RAW_OUTPUT_DIR"

#since we cannot get all counters at once, does multiple iterations collecting different counters each iteration
cntRun=0
while [ ! -z "$TRACE_PERFCNTS" ]
do
    #actual counters to be collected in this iteration
    ACTUAL_TRACE_PERFCNTS="perfcnts="
    for cntiter in $(seq 1 $TRACE_MAXPERFCNT)
    do
        ACTUAL_TRACE_PERFCNTS="$ACTUAL_TRACE_PERFCNTS${TRACE_PERFCNTS[0]},"
        unset TRACE_PERFCNTS[0]
        TRACE_PERFCNTS=( "${TRACE_PERFCNTS[@]}" )
    done
    #echo $cntRun $ACTUAL_TRACE_PERFCNTS
    
    #may run multiple time so we can then average out the counters later
    _TRACE_RUNITERS=$(($TRACE_RUNITERS-1))
    for iter in $(seq 0 $_TRACE_RUNITERS)
    do
        CSV_OUT=$RAW_OUTPUT_DIR/trace_result--cnt$cntRun--iter$iter.csv
        PERIODIC_CSV_OUT=$RAW_OUTPUT_DIR/periodic_trace_result--cnt$cntRun--iter$iter.csv
        #interference may cause it to fail
        #we may retry up to 10 times until no interference is detected
        for retry in $(seq 1 10)
        do            
            TRACED_PROGRAM_OUT=$RAW_OUTPUT_DIR/trace_outerr--cnt$cntRun--iter$iter.txt
            TASKSET_MASK=0x$(echo "obase=16;$((1<<$TRACED_CORE))" | bc)

            echo "perfcnt run $cntRun (iter $iter out of $_TRACE_RUNITERS)"
            sudosh $MARS_SCRIPTDIR/runtime/start.sh tracing trace_core=$TRACED_CORE $ACTUAL_TRACE_PERFCNTS
            taskset $TASKSET_MASK $TRACED_PROGRAM $TRACED_PROGRAM_ARGS >$TRACED_PROGRAM_OUT 2>&1
            sudosh $MARS_SCRIPTDIR/runtime/stop.sh

            CURR_DMESG=$RAW_OUTPUT_DIR/trace_dmesg--cnt$cntRun--iter$iter.txt
            sudo dmesg -c > $CURR_DMESG

            DAEMON_OUTFILE=$RTS_DAEMON_OUTDIR/total.csv
            grep -q "traced.pid" $DAEMON_OUTFILE
            RET="$?"
            if [ $RET -eq 0 ]; then
                #keep only the header and the line for the traced task
                grep "component;" $DAEMON_OUTFILE > $CSV_OUT
                grep "traced.pid" $DAEMON_OUTFILE >> $CSV_OUT
                
                #gets the periodic trace
                TRACEDTASK_TRACE_NAME=$(grep traced $DAEMON_OUTFILE | cut -d ';' -f1 | sed 's/traced.//g')
                cp $RTS_DAEMON_OUTDIR/trace.$TRACEDTASK_TRACE_NAME.csv $PERIODIC_CSV_OUT
                break
            fi
        done
    done
    
    cntRun=$((cntRun+1))
done

# Makes sure all cores are ondemand
sudosh $MARS_SCRIPTDIR/tracing/trace_setgov.sh ondemand



