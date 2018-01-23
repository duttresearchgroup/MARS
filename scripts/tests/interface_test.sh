#!/bin/sh

source $SPARTA_SCRIPTDIR/runtime/common.sh

sh $SPARTA_SCRIPTDIR/runtime/start.sh interfacetest
taskset 0x10 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null &
taskset 0x40 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null &
taskset 0x02 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null &
taskset 0x04 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null &
wait
sh $SPARTA_SCRIPTDIR/runtime/stop.sh

cp $RTS_DAEMON_OUTDIR/total.csv quickTestInteraceTest-total.csv
cp $RTS_DAEMON_OUTDIR/execTraceFine.csv quickTestInteraceTest-fine_trace.csv
cp $RTS_DAEMON_OUTDIR/execTraceCoarse.csv quickTestInteraceTest-coarse_trace.csv

