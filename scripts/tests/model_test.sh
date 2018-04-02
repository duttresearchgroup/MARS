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

source $SPARTA_SCRIPTDIR/runtime/common.sh

MODELS=$(readlink -f $MODEL_DIR/arm_exynos5422)

echo "Idle test"
sudosh $SPARTA_SCRIPTDIR/runtime/start.sh model_test model_path=$MODELS
sleep 5
sudosh $SPARTA_SCRIPTDIR/runtime/stop.sh
rm -rf model_test_outdir-idle
cp -R $RTS_DAEMON_OUTDIR model_test_outdir-idle


echo "One major task per core test"
sudosh $SPARTA_SCRIPTDIR/runtime/start.sh model_test model_path=$MODELS
taskset 0x01 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null &
taskset 0x02 sh $SPARTA_SCRIPTDIR/ubenchmarks/low_ipc_badcache_high_load.sh > /dev/null &
taskset 0x04 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_medium_load.sh > /dev/null &
taskset 0x08 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null &

taskset 0x10 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null &
taskset 0x20 sh $SPARTA_SCRIPTDIR/ubenchmarks/low_ipc_badcache_high_load.sh > /dev/null &
taskset 0x40 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_medium_load.sh > /dev/null &
taskset 0x80 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null &
wait
sudosh $SPARTA_SCRIPTDIR/runtime/stop.sh
rm -rf model_test_outdir-single_core
cp -R $RTS_DAEMON_OUTDIR model_test_outdir-single_core


echo "Multiple major task per core test"
sudosh $SPARTA_SCRIPTDIR/runtime/start.sh model_test model_path=$MODELS
taskset 0x01 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null &
taskset 0x02 sh $SPARTA_SCRIPTDIR/ubenchmarks/low_ipc_badcache_high_load.sh > /dev/null &
taskset 0x04 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_medium_load.sh > /dev/null &
taskset 0x08 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null &

taskset 0x10 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null &
taskset 0x20 sh $SPARTA_SCRIPTDIR/ubenchmarks/low_ipc_badcache_high_load.sh > /dev/null &
taskset 0x40 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_medium_load.sh > /dev/null &
taskset 0x80 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null &

taskset 0x08 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null &
taskset 0x04 sh $SPARTA_SCRIPTDIR/ubenchmarks/low_ipc_badcache_high_load.sh > /dev/null &
taskset 0x02 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_medium_load.sh > /dev/null &
taskset 0x01 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null &

taskset 0x80 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null &
taskset 0x40 sh $SPARTA_SCRIPTDIR/ubenchmarks/low_ipc_badcache_high_load.sh > /dev/null &
taskset 0x20 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_medium_load.sh > /dev/null &
taskset 0x10 sh $SPARTA_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null &
wait
sudosh $SPARTA_SCRIPTDIR/runtime/stop.sh
rm -rf model_test_outdir-multi_core
cp -R $RTS_DAEMON_OUTDIR model_test_outdir-multi_core

