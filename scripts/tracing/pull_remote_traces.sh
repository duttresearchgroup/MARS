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

##################################################################
# Pulls remote traces from remote device (as defined in confs.sh)
# DATA ON THE CALLER DEVICE MAY BE OVERWRITTEN
##################################################################

# Remote functions
source $SPARTA_SCRIPTDIR/common/remote.sh

TGT_TRACES_DIR=$TRACE_OUTPUT_DIR-$RTS_ARCH-$RTS_PLAT
R_TGT_TRACES_DIR=$R_TRACE_OUTPUT_DIR-$RTS_ARCH-$RTS_PLAT

echo Synching: $TGT_TRACES_DIR with $R_TGT_TRACES_DIR 

R_WAIT

R_SYNCH_LOCAL $R_TGT_TRACES_DIR $TGT_TRACES_DIR

