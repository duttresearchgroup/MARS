
######################################################
# This script synchs the bin_*, scripts, and trace 
# folders on a remote device with the ones on this 
# machine.
# DATA ON THE REMOTE DEVICE WILL BE OVERWRITTEN
######################################################


######################################################
# Extracts the script folder from the current script
######################################################
__SOURCE="${BASH_SOURCE[0]}"
while [ -h "$__SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  __DIR="$( cd -P "$( dirname "$__SOURCE" )" && pwd )"
  __SOURCE="$(readlink "$__SOURCE")"
  [[ $__SOURCE != /* ]] && __SOURCE="$__DIR/$__SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
SCRIPTDIR="$( cd -P "$( dirname "$__SOURCE" )" && cd .. && pwd )"


# Remote functions
source $SCRIPTDIR/common/remote.sh

# This build dir
BUILD_DIR="$SCRIPTDIR/.."



SYNCH_DIRS=$(ls $BUILD_DIR 2>/dev/null | xargs -n 1 | grep bin_)
SYNCH_DIRS="$SYNCH_DIRS scripts"

echo Synching: $SYNCH_DIRS

R_WAIT

for i in $SYNCH_DIRS; do
    R_SYNCH $BUILD_DIR/$i $R_BUILD_DIR/$i
done


