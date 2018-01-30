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
#   1) Core where the traced process executes
#   2) Core frequency (in kHz)
#   3) Path to traced application binary.
#      Remaining parameters are passed as the application
#      parameters
# As a side effect, all cores will be using the ondemand
# dvfs governor after running this script
#########################################################


TRACED_CORE=$1
shift
TRACE_FREQUENCY=$1
shift
TRACED_PROGRAM=$1
shift
TRACED_PROGRAM_ARGS="$@"

# Source the files we need
source $SPARTA_SCRIPTDIR/runtime/common.sh

PREV_DMESG=$TRACE_OUTPUT_DIR/trace_dmesg_prev.txt
CURR_DMESG=$TRACE_OUTPUT_DIR/trace_dmesg.txt
TRACED_PROGRAM_OUT=$TRACE_OUTPUT_DIR/trace_outerr.txt
STATUS_FILE=$TRACE_OUTPUT_DIR/trace_status.txt

DAEMON_OUTFILE=$RTS_DAEMON_OUTDIR/total.csv

# Cleans up / makes the trace dir
mkdir -p $TRACE_OUTPUT_DIR
rm -rf $TRACE_OUTPUT_DIR/*.txt
rm -rf $TRACE_OUTPUT_DIR/*.csv

# Set the governor of all cores to ondemand and the governor of the core we need to the given frequency
$SPARTA_SCRIPTDIR/tracing/trace_set_freq.sh $TRACED_CORE $TRACE_FREQUENCY ondemand

echo "Tracing " $TRACED_PROGRAM " at core " $TRACED_CORE " at freq " $(cat /sys/devices/system/cpu/cpu$TRACED_CORE/cpufreq/cpuinfo_cur_freq)
dmesg -c > $PREV_DMESG

echo "RUNNING" > $STATUS_FILE

#since we cannot get all counters at once, does multiple iterations collecting different counters each iteration
cntRun=0
while [ ! -z "$TRACE_PERFCNTS" ]
do
    #actual counters to be collected in this iteration
    ACTUAL_TRACE_PERFCNTS=""
    for cntiter in $(seq 1 $TRACE_MAXPERFCNT)
    do
        ACTUAL_TRACE_PERFCNTS="$ACTUAL_TRACE_PERFCNTS ${TRACE_PERFCNTS[0]}"
        unset TRACE_PERFCNTS[0]
        TRACE_PERFCNTS=( "${TRACE_PERFCNTS[@]}" )
    done
    #echo $cntRun $ACTUAL_TRACE_PERFCNTS
    
    #may run multiple time so we can then average out the counters later
    _TRACE_RUNITERS=$(($TRACE_RUNITERS-1))
    for iter in $(seq 0 $_TRACE_RUNITERS)
    do
        CSV_OUT=$TRACE_OUTPUT_DIR/trace_result--cnt$cntRun--iter$iter.csv
        PERIODIC_CSV_OUT=$TRACE_OUTPUT_DIR/periodic_trace_result--cnt$cntRun--iter$iter.csv
        #interference may cause it to fail
        #we may retry up to 10 times until no interference is detected
        for retry in $(seq 1 10)
        do            
            echo "perfcnt run $cntRun (iter $iter out of $_TRACE_RUNITERS)"
            sh $SPARTA_SCRIPTDIR/runtime/start.sh tracing trace_core=$TRACED_CORE $ACTUAL_TRACE_PERFCNTS
            $TRACED_PROGRAM $TRACED_PROGRAM_ARGS >$TRACED_PROGRAM_OUT 2>&1 
            sh $SPARTA_SCRIPTDIR/runtime/stop.sh
            dmesg -c > $CURR_DMESG
           
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

#since stuff might crash, this signals the host that it actually executed
#host deletes this file after reading it
echo "DONE" > $STATUS_FILE

# Makes sure all cores are ondemand
$SPARTA_SCRIPTDIR/tracing/trace_setgov.sh ondemand



