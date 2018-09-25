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
# This script synchs the bin_*, scripts, and models 
# folders on a remote device with the ones on this 
# machine.
# DATA ON THE REMOTE DEVICE WILL BE OVERWRITTEN
######################################################

# Remote functions
source $MARS_SCRIPTDIR/common/remote.sh

SYNCH_DIRS=$(ls $MARS_ROOT 2>/dev/null | xargs -n 1 | grep bin_"$RTS_ARCH"_"$RTS_PLAT")
SYNCH_DIRS="$SYNCH_DIRS scripts models src"

echo Synching: $SYNCH_DIRS

R_WAIT

for i in $SYNCH_DIRS; do
    R_SYNCH $MARS_ROOT/$i $R_MARS_ROOT/$i
done

