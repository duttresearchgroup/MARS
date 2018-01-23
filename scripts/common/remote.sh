######################################################
# This script provides helper functions to run other
# scripts on a remote device.
# This script connects with the device using ssh
######################################################

# Includes the confs
source $SPARTA_SCRIPTDIR/confs.sh


######################################################
# Runs a remote shell command
######################################################

function R_SHELL(){
    if [ -n "$1" ]; then
        sshpass $R_VERBOSE -p $R_PASS ssh $R_USER@$R_HOST $@
    fi
}
export -f R_SHELL

######################################################
# Waits until the remote device is ready
######################################################

function R_WAIT(){
    __SSHSLEEP="R_SHELL sleep 0.1"
    while ! $__SSHSLEEP
    do
       echo "Waiting for device"
       sleep 2
    done
}
export -f R_WAIT


######################################################
# Copies a file form the remote device
######################################################

function R_PULL(){
    __SRC=$1
    __DEST=$2
    if [ -n "$__DEST" ]; then
        sshpass $R_VERBOSE -p $R_PASS scp $R_USER@$R_HOST:$__SRC $__DEST
    else
        sshpass $R_VERBOSE -p $R_PASS scp $R_USER@$R_HOST:$__SRC .
    fi
    
}
export -f R_PULL


######################################################
# Synchs a local directory with a remote directory
# Data in the remote directory may be OVERWRITTEN
######################################################

function R_SYNCH(){
    __SRC=$1
    __DEST=$2
    if [ -e "$__SRC" ]; then
        sshpass $R_VERBOSE -p $R_PASS rsync --progress -avz $__SRC/ $R_USER@$R_HOST:$__DEST/
    else
        echo $__SRC does not exist
        exit
    fi
}
export -f R_SYNCH

