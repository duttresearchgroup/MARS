
##########################################################
# Unmount a tmpfs mount point using tmpfs_mount.sh using
# the following steps.
# - Copy the tmpfs to another tmp directory 
# - Unmount the tmpfs
# - Copy the data back to the directory where the tmpfs was
#   mounted and deletes the tmp directory
############################################################

MOUNT_DIR=$1

if [ -e "$MOUNT_DIR/.XXXsomerandomfilenameXXX" ]
then
    rm $MOUNT_DIR/.XXXsomerandomfilenameXXX
else
    echo "ERROR: $MOUNT_DIR was not mounted using tmpfs_mount.sh"
    exit
fi


#undo the bench_run thing

TMPDIR="$MOUNT_DIR"_tmp_xxx_tmpfs

rm -rf $TMPDIR
mkdir -p $TMPDIR
#copy all exept symbolic links
FILES=$(find $MOUNT_DIR -type f | xargs)
for f in $FILES
do
    tgt="$TMPDIR${f#$MOUNT_DIR}"
    mkdir -p $(dirname $tgt)
    cp $f $tgt
done
umount $MOUNT_DIR
mv $TMPDIR/* $MOUNT_DIR/ &> /dev/null
rm -rf $TMPDIR

