####################################################
# Runs all the predictor training ubenchmarks
# The training application is first run in
# calibration mode and then again to for training.
# Parameters
#    - The core to be used for training
#    - The frequency in kHz
################################################

TRAINING_CORE=$1
TRAINING_FREQUENCY=$2

# Includes the confs
source $SPARTA_SCRIPTDIR/ubenchmarks/common.sh


PRED_BIN=$UBENCH_BIN_DIR/predictor_master

#Calibration if not available
CALIB_FILE=$PRED_BIN.calib.$RTS_ARCH.$RTS_PLAT.$CALIB_CORE.$CALIB_FREQUENCY
if [ -e $CALIB_FILE ]
then
    echo "Calibration file present"
    CALIBRATION_ARGS=$(cat $CALIB_FILE)
else
    echo "Calibration file not found!"
    echo "Running calibration on core $CALIB_CORE @ $CALIB_FREQUENCY kHz"
    sudosh $SPARTA_SCRIPTDIR/tracing/trace_set_freq.sh $CALIB_CORE $CALIB_FREQUENCY ondemand
    CALIBRATION_ARGS=$(taskset -c $CALIB_CORE $PRED_BIN | grep calibration_args | cut -f2- -d' ')    
    sudosh $SPARTA_SCRIPTDIR/tracing/trace_setgov.sh ondemand
    echo $CALIBRATION_ARGS > $CALIB_FILE
fi
echo "Calibration args $CALIBRATION_ARGS"

echo "Training on core $TRAINING_CORE @ $TRAINING_FREQUENCY kHz"

# TODO enable tracing. Put result on a different file
# Add a test script for the odroid that runs the training set
taskset -c $TRAINING_CORE $PRED_BIN $CALIBRATION_ARGS






