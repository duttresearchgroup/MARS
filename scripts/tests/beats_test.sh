#!/bin/sh
#-------------------------------------------------------------------------------
# Copyright (C) 2018 Biswadip Maity <biswadip.maity@gmail.com>
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

# Runs a couple of benchmarks using the beatsmonitor daemon

source $MARS_SCRIPTDIR/runtime/common.sh

sudosh $MARS_SCRIPTDIR/runtime/start.sh beatsmonitor
$MARS_ROOT/"bin_"$RTS_ARCH"_"$RTS_PLAT/uapitests/simple_test &
wait
sudosh $MARS_SCRIPTDIR/runtime/stop.sh