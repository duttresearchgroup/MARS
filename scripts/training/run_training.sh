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

####################################################
# Runs all the predictor training ubenchmarks
# The training application is first run in
# calibration mode and then again to for training.
# Parameters
#    - The binary used for training
#         Notice that this is a very specific binary
#         that supports the calibrations args.
#         See run_training-all.sh and run_training-test.sh
#    - The core to be used for training
#    - The frequency in kHz
#
# Usage:
#   Always called from run_training-all.sh and run_training-test.sh
#   Check those scripts for usage
################################################

PRED_BIN=$1
TRAINING_CORE=$2
TRAINING_FREQUENCY=$3

if [ -z "$PRED_BIN" ]; then echo "PRED_BIN unset" && exit; fi
if [ -z "$TRAINING_CORE" ]; then echo "TRAINING_CORE unset" && exit; fi
if [ -z "$TRAINING_FREQUENCY" ]; then echo "TRAINING_FREQUENCY unset" && exit; fi

#Calibration if not available
CALIB_FILE=$PRED_BIN.calib.$RTS_ARCH.$RTS_PLAT.$CALIB_CORE.$CALIB_FREQUENCY
if [ -e $CALIB_FILE ]
then
    echo "Calibration file present"
    CALIBRATION_ARGS=$(cat $CALIB_FILE)
else
    echo "Calibration file not found!"
    echo "Running calibration on core $CALIB_CORE @ $CALIB_FREQUENCY kHz"
    sudosh $MARS_SCRIPTDIR/tracing/trace_set_freq.sh $CALIB_CORE $CALIB_FREQUENCY ondemand
    CALIBRATION_ARGS=$(taskset -c $CALIB_CORE $PRED_BIN | grep calibration_args | cut -f2- -d' ')    
    sudosh $MARS_SCRIPTDIR/tracing/trace_setgov.sh ondemand
    echo $CALIBRATION_ARGS > $CALIB_FILE
fi
echo "Calibration args $CALIBRATION_ARGS"

echo "Training on core $TRAINING_CORE @ $TRAINING_FREQUENCY kHz"
TRACE_NAME=$(basename $PRED_BIN)
sh $MARS_SCRIPTDIR/tracing/trace_one.sh $TRACE_NAME $TRAINING_CORE $TRAINING_FREQUENCY $PRED_BIN $CALIBRATION_ARGS




