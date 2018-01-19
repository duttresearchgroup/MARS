
##########################################################
# We might wanna dump benchmark outputs to a ram-based 
# fs to avoid slow sdcard interference.
# This scrip will create a tmpfs on top of the specified 
# directory
############################################################

MOUNT_DIR=$1

mkdir -p $MOUNT_DIR
mount -t tmpfs tmpfs $MOUNT_DIR

# to check this later at tmpfs_umount
touch $MOUNT_DIR/.XXXsomerandomfilenameXXX

