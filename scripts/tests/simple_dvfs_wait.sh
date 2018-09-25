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

source $MARS_SCRIPTDIR/runtime/common.sh

MODELS=$(readlink -f $MODEL_DIR/arm_exynos5422)

high_load(){
    taskset $1 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null
    taskset $1 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null
    taskset $1 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null
    taskset $1 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null
    taskset $1 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_high_load.sh > /dev/null
}
low_load(){
    taskset $1 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null
    taskset $1 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null
    taskset $1 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null
    taskset $1 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null
    taskset $1 sh $MARS_SCRIPTDIR/ubenchmarks/high_ipc_low_load.sh > /dev/null
}

sudosh $MARS_SCRIPTDIR/runtime/start.sh simple_dvfs model_path=$MODELS

high_load 0x10 &
high_load 0x40 &
low_load 0x02 &
low_load 0x04 &

sudosh $MARS_SCRIPTDIR/runtime/wait_for_stop.sh

while [ 1 ]
do
    pid=$(jobs -p)
    echo $pid
    if [ -z "$pid" ]
    then
        exit
    else
        sudo kill $pid
    fi
    sleep 1
done

