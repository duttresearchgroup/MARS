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

##############################################
# Makes sure dumping of core files is enabled
# and sets tthe core dump location to the given
# path
##############################################

CORE_DUMP_DIR=$1

mkdir -p $CORE_DUMP_DIR
echo "$CORE_DUMP_DIR/core.%e.%p" > /proc/sys/kernel/core_pattern
ulimit -c unlimited

cat /proc/sys/kernel/core_pattern
ulimit -a
