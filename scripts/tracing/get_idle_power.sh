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

#######################################################
# Runs the IdlePowerChecker daemon to collect a trace
# with power measurements when the system is idle across
# multiple frequencies.
# Takes as input the directory where the idle power
# is to be saved (save to the curr. dir if not given).
# Usage:
#   get_idle_power.sh <idle_power_dir>
#   get_idle_power.sh
#######################################################

source $SPARTA_SCRIPTDIR/runtime/common.sh

if [ ! -z "$1" ]; then
    IDLE_POWER_DIR=$1
else
    IDLE_POWER_DIR=idle_power-$RTS_ARCH-$RTS_PLAT
fi

sudosh $SPARTA_SCRIPTDIR/runtime/start.sh idlepowerchecker
sudosh $SPARTA_SCRIPTDIR/runtime/wait_for_stop.sh

echo "Saving idle power traces to $IDLE_POWER_DIR"
mkdir -p $IDLE_POWER_DIR
cp $RTS_DAEMON_OUTDIR/idle_trace.* $IDLE_POWER_DIR/
cp $RTS_DAEMON_OUTDIR/sys_info.json $IDLE_POWER_DIR/

