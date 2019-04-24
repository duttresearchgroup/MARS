#!/bin/sh
#-------------------------------------------------------------------------------
# Copyright (C) 2019 Saehanseul Yi <saehansy@gmail.com>
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

sudosh $MARS_SCRIPTDIR/runtime/start.sh gpuperfctrtest
sudo LD_LIBRARY_PATH=/usr/local/cuda/extras/CUPTI/lib64:$LD_LIBRARY_PATH LD_PRELOAD=$MARS_SCRIPTDIR/../lib_arm64_jetsontx2/libhookcuda.so /usr/local/cuda-9.0/samples/0_Simple/matrixMul/matrixMul wA=4096 hA=4096 wB=4096 hB=4096 > /dev/null &
wait
sudosh $MARS_SCRIPTDIR/runtime/stop.sh

cp $RTS_DAEMON_OUTDIR/execTraceFine.csv gpu_perfctr_test-fine_trace.csv
cp $RTS_DAEMON_OUTDIR/execTraceCoarse.csv gpu_perfctr_test-coarse_trace.csv

