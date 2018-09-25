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
# Start the specified daemon. 
# The first argument is the binary name of the 
# daemon (without path). The binary must be available
# at the binaries folder defined at confs.sh
# Remaining arguments will be forwarded to the daemon
######################################################

# Common defs
source $MARS_SCRIPTDIR/runtime/common.sh

# First command line argument is the daemon binary
RTS_DAEMON_BIN=$1
shift
# Remaining command line arguments are forwarded as daemon parameters
RTS_PARAMS="$@"


#Checks if we run this command as root (needed to insert and interface with the module)
if [[ $(id -u) -ne 0 ]] ; then echo "Please run as root" ; exit ; fi

# Make sure if we already have any daemon running, kill it
RTS_SEEK_DAEMONS_AND_KILL
# If the sensing module is already inserted, remove it so we can start with a fresh one
rmmod $RTS_MODULE_NAME &> /dev/null

# the daemon creates the following file to
# indicate it's ready
# (note: this is fixed by the C++ implementation)
RTS_DAEMON_READY=$RTS_DAEMON_BIN_DIR/$RTS_DAEMON_BIN.ready

# cleans up output directory and any previous .ready/.pid file
rm -rf $RTS_DAEMON_OUTDIR
mkdir $RTS_DAEMON_OUTDIR
rm -rf $RTS_DAEMON_READY
rm -rf $RTS_DAEMON_PID

# makes sure debugfs is available to non sudo apps
sudo chmod a+rx /sys/kernel/debug

# inserts kernel sensing module
insmod $RTS_MODULE_PATH

_DAEMON_CMD="$RTS_DAEMON_BIN_DIR/$RTS_DAEMON_BIN outdir=$RTS_DAEMON_OUTDIR mode=$RTS_DAEMON_BIN $RTS_PARAMS"

# starts daemon proc and waits until ready
if [ "$RTS_VALGRIND" -eq "1" ]; then
    echo "Running with valgrind"
    $RTS_VALGRIND_CMD $_DAEMON_CMD
else
    $_DAEMON_CMD
fi

while [ ! -f  "$RTS_DAEMON_READY" ]; do sleep 0.001; done

# RTS_DAEMON_READY and RTS_DAEMON_PID have the same content so just link
ln -s $RTS_DAEMON_READY $RTS_DAEMON_PID


