######################################################
# This script synchs the bin_*, scripts, and trace 
# folders on a remote device with the ones on this 
# machine.
# DATA ON THE REMOTE DEVICE WILL BE OVERWRITTEN
######################################################

# Remote functions
source $SPARTA_SCRIPTDIR/common/remote.sh

SYNCH_DIRS=$(ls $SPARTA_ROOT 2>/dev/null | xargs -n 1 | grep bin_"$RTS_ARCH"_"$RTS_PLAT")
SYNCH_DIRS="$SYNCH_DIRS scripts"

echo Synching: $SYNCH_DIRS

R_WAIT

for i in $SYNCH_DIRS; do
    R_SYNCH $SPARTA_ROOT/$i $R_SPARTA_ROOT/$i
done

