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

# Min
sh $MARS_SCRIPTDIR/training/run_training-all.sh 2 200000
sh $MARS_SCRIPTDIR/training/run_training-all.sh 6 200000

# Max
sh $MARS_SCRIPTDIR/training/run_training-all.sh 2 1400000
sudosh $MARS_SCRIPTDIR/odroid/fan_max.sh # Gonna get Hot!
sleep 5 # cooldown time
sh $MARS_SCRIPTDIR/training/run_training-all.sh 6 2000000
sleep 5 # cooldown time
sudosh $MARS_SCRIPTDIR/odroid/fan_auto.sh

# Middle
sh $MARS_SCRIPTDIR/training/run_training-all.sh 2 800000
sh $MARS_SCRIPTDIR/training/run_training-all.sh 6 1100000

# Some other freqs for each
sh $MARS_SCRIPTDIR/training/run_training-all.sh 2 500000
sh $MARS_SCRIPTDIR/training/run_training-all.sh 6 500000
sh $MARS_SCRIPTDIR/training/run_training-all.sh 6 800000
sh $MARS_SCRIPTDIR/training/run_training-all.sh 2 1100000
sh $MARS_SCRIPTDIR/training/run_training-all.sh 6 1400000
sh $MARS_SCRIPTDIR/training/run_training-all.sh 6 1700000





