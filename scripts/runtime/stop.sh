######################################################
# Stops a daemon started with start.sh and removes the
# kernel sensing module
######################################################

######################################################
# First Extracts the script folder from the current
# script path and includes the common defs
######################################################
__SOURCE="${BASH_SOURCE[0]}"
while [ -h "$__SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  __DIR="$( cd -P "$( dirname "$__SOURCE" )" && pwd )"
  __SOURCE="$(readlink "$__SOURCE")"
  [[ $__SOURCE != /* ]] && __SOURCE="$__DIR/$__SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
SCRIPTDIR="$( cd -P "$( dirname "$__SOURCE" )" && cd .. && pwd )"
# Common defs
source $SCRIPTDIR/runtime/common.sh
####################################################

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
