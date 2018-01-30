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


