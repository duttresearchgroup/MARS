#!/bin/sh
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

# Runs a couple of benchmarks using the interfacetest daemon

source $MARS_SCRIPTDIR/runtime/common.sh

sudosh $MARS_SCRIPTDIR/runtime/start.sh interfacetest
taskset 0x10 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null &
taskset 0x40 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null &
taskset 0x02 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null &
taskset 0x04 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null &
wait
sudosh $MARS_SCRIPTDIR/runtime/stop.sh

cp $RTS_DAEMON_OUTDIR/execTraceFine.csv interface_test-fine_trace.csv
cp $RTS_DAEMON_OUTDIR/execTraceCoarse.csv interface_test-coarse_trace.csv

