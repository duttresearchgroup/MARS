######################################################
# Stops a daemon started with start.sh and removes the
# kernel sensing module
######################################################

# Common defs
source $SPARTA_SCRIPTDIR/runtime/common.sh

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
