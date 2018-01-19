######################################################
# This script provides helper functions to run other
# scripts on a remote device.
# This script connects with the device using ssh
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

######################################################
# Checks if the remote file conf exists. If it does
# not, creates one with default information at
# scripts/common/remote_conf.sh
# Edit the generated file with the correct remote host
# info
######################################################

__R_CONF=$SCRIPTDIR/common/remote_conf.sh
__R_CONFDEFAULT=$SCRIPTDIR/common/.remote_conf.sh
if [ -e $__R_CONF ]
then
    source $__R_CONF
else
    echo "$__R_CONF not found. Creating default file..."
    cp $__R_CONFDEFAULT $__R_CONF
    echo "Please edit $__R_CONF with your remote host info and rerun this script"
    exit
fi


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

