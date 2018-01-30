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
# Stops a daemon started with start.sh and removes the
# kernel sensing module
######################################################

# Common defs
source $SPARTA_SCRIPTDIR/runtime/common.sh

#Checks if we run this command as root (needed to insert and interface with the module)
if [[ $(id -u) -ne 0 ]] ; then echo "Please run as root" ; exit ; fi

#stops the deamon and waits
if [ -f  "$RTS_DAEMON_PID" ]; then
    pid=$(cat $RTS_DAEMON_PID)
    if [ ! -z "$pid" ]; then
        #send sigquit
        kill -s 3 $pid
        while [ -e /proc/$pid ]; do sleep 0.001; done
    else
        echo "Daemon pid=$pid at $RTS_DAEMON_PID was invalid!"    
    fi
    
else
    echo "ERROR: Daemon was not running!"
fi

#remove the module
rmmod $RTS_MODULE_NAME
#makes sure everything was stoped
rmmod $RTS_MODULE_NAME &> /dev/null

# cleans up .ready/.pid file
rm -rf $RTS_DAEMON_BIN_DIR/*.ready
rm -rf $RTS_DAEMON_PID
