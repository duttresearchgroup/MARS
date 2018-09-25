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

sh $MARS_SCRIPTDIR/training/run_training-test.sh 2 200000
sh $MARS_SCRIPTDIR/training/run_training-test.sh 6 200000

sh $MARS_SCRIPTDIR/training/run_training-test.sh 2 800000
sh $MARS_SCRIPTDIR/training/run_training-test.sh 6 1100000

sh $MARS_SCRIPTDIR/training/run_training-test.sh 2 1400000
sh $MARS_SCRIPTDIR/training/run_training-test.sh 6 2000000

sh $MARS_SCRIPTDIR/training/run_training-all.sh 6 2000000
