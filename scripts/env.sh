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

########################################################
# Extracts the script folder from the current script

__SOURCE="${BASH_SOURCE[0]}"
while [ -h "$__SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  __DIR="$( cd -P "$( dirname "$__SOURCE" )" && pwd )"
  __SOURCE="$(readlink "$__SOURCE")"
  [[ $__SOURCE != /* ]] && __SOURCE="$__DIR/$__SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
export SPARTA_SCRIPTDIR="$( cd -P "$( dirname "$__SOURCE" )" && pwd )"

########################################################
# The root dir
export SPARTA_ROOT=$(readlink -f $SPARTA_SCRIPTDIR/..)

######################################################
# The new path
export PATH=$SPARTA_SCRIPTDIR:$PATH

######################################################
# Checks if the remote confs exists. If it does
# not, creates one with default information at
# scripts/confs.sh
# Edit the generated file with the correct values for
# Your case

__CONF=$SPARTA_SCRIPTDIR/confs.sh
__CONFDEFAULT=$SPARTA_SCRIPTDIR/.confs.sh
if [ -e $__CONF ]
then
    echo Found $(basename $__CONF)
else
    echo $(basename $__CONF) not found
    cp $__CONFDEFAULT $__CONF
    echo $(basename $__CONF) created with default values
fi

echo Please make sure $__CONF
echo has the correct setupt before running scripts
