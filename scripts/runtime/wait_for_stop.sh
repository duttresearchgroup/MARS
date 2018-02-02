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
# Similar to stop.sh, except that this script does not
# send sigquit to the daemon. It expects that the daemon
# will signal itself to quit, so it block and wait 
# until the daemon finishes
######################################################

# Common defs
source $SPARTA_SCRIPTDIR/runtime/common.sh

#Checks if we run this command as root (needed to insert and interface with the module)
if [[ $(id -u) -ne 0 ]] ; then echo "Please run as root" ; exit ; fi

#waits for the daemon
if [ -f  "$RTS_DAEMON_PID" ]; then
    pid=$(cat $RTS_DAEMON_PID)
    if [ ! -z "$pid" ]; then
        #just wait
        while [ -e /proc/$pid ]; do sleep 0.1; done
    else
        echo "Daemon pid=$pid at $RTS_DAEMON_PID was invalid!"    
    fi  
fi
#no error if daemon was not running (it may have exited before calling this script)

#remove the module
rmmod $RTS_MODULE_NAME &> /dev/null
#makes sure everything was stoped
rmmod $RTS_MODULE_NAME &> /dev/null

# cleans up .ready/.pid file
rm -rf $RTS_DAEMON_BIN_DIR/*.ready
rm -rf $RTS_DAEMON_PID
