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

###########################################
# Disables the board fan
###########################################

if [ -f /sys/devices/odroid_fan.13/fan_mode ]; then
   FAN=13
elif [ -f /sys/devices/odroid_fan.14/fan_mode ]; then
   FAN=14
else
   echo "This machine is not supported."
   exit 1
fi

FAN_MODE_FILE="/sys/devices/odroid_fan.$FAN/fan_mode"
FAN_SPEED_FILE="/sys/devices/odroid_fan.$FAN/pwm_duty"

if [ ! -f $FAN_MODE_FILE ]; then
  echo "no fan mode file"
  exit 1
elif [ ! -f $FAN_SPEED_FILE ]; then
  echo "no fan speed file"
  exit 1
fi

#to be sure we can manage fan
echo 0 > $FAN_MODE_FILE 
echo 0 > $FAN_SPEED_FILE

