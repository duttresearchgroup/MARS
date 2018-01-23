######################################################
# This script provides common definitions for the
# other runtime scripts
######################################################

# Includes the confs
source $SPARTA_SCRIPTDIR/confs.sh

######################################################
# Common definitions
######################################################

# binaries path
RTS_BIN_DIR_NAME="bin_"$RTS_ARCH"_"$RTS_PLAT
RTS_BINS_PATH=$SPARTA_ROOT/$RTS_BIN_DIR_NAME

# path and name of the kernel sensing module
RTS_MODULE_NAME=vitamins
RTS_MODULE_PATH=$RTS_BINS_PATH/sensing_module/$RTS_MODULE_NAME.ko

# directory where the daemon will dump output
RTS_DAEMON_OUTDIR=$SPARTA_ROOT/outdir

# directory where the daemons binaries are
RTS_DAEMON_BIN_DIR=$RTS_BINS_PATH/daemons

# file used to indicate to the stop script that
# a daemon is running. Stores the pid of the
# daemon when running
RTS_DAEMON_PID=$RTS_DAEMON_BIN_DIR/.current_active_daemon.pid

echo SPARTA_SCRIPTDIR=$SPARTA_SCRIPTDIR
echo SPARTA_ROOT=$SPARTA_ROOT
echo RTS_BINS_PATH=$RTS_BINS_PATH
echo RTS_MODULE_NAME=$RTS_MODULE_NAME
echo RTS_MODULE_PATH=$RTS_MODULE_PATH
echo RTS_DAEMON_OUTDIR=$RTS_DAEMON_OUTDIR
echo RTS_DAEMON_BIN_DIR=$RTS_DAEMON_BIN_DIR
echo RTS_DAEMON_PID=$RTS_DAEMON_PID

######################################################
# Helper functions
######################################################


# Searches for active daemon processes and kills them

function RTS_SEEK_DAEMONS_AND_KILL(){

    # Searches for processes whose name matches a
    # binary at $RTS_DAEMON_BIN_DIR and kill it 
    # if it has $RTS_BINS_PATH in its path

    daemons_bin_files=$(ls $RTS_DAEMON_BIN_DIR 2>/dev/null | xargs)
    for dbin in $daemons_bin_files; do
        daemon_pid=$(pgrep -x -U $(id -ur) $dbin)
        daemon_path=$(find /proc/$daemon_pid/exe -printf "%l\n" 2>/dev/null)
        if [[ $daemon_path == *"$RTS_BIN_DIR_NAME"* ]]; then
          echo "Match"
          kill -9 $daemon_pid
        fi
    done
}
export -f RTS_SEEK_DAEMONS_AND_KILL


