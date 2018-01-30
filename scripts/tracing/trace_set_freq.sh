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
# Sets the governor of all cores to the specified one 
# and the governor of a specific core the given frequency
# Params:
#  - The target core
#  - The target freq
#  - The governor to be used for the other cores
########################################################

TRACED_CORE=$1
TRACE_FREQUENCY=$2
OTHER_CORES_GOVERNOR=$3

#echo $TRACED_CORE
#echo $TRACE_FREQUENCY
#echo $OTHER_CORES_GOVERNOR

echo $OTHER_CORES_GOVERNOR | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor >/dev/null
echo userspace | tee /sys/devices/system/cpu/cpu$TRACED_CORE/cpufreq/scaling_governor >/dev/null
echo $TRACE_FREQUENCY | tee /sys/devices/system/cpu/cpu$TRACED_CORE/cpufreq/scaling_setspeed >/dev/null
