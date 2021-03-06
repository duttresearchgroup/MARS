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

####################################################
# Runs the training using the test training set
# Parameters
#    - The core to be used for training
#    - The frequency in kHz
#
# Usage:
#   run_training-test.sh <training_core> <training_core_freq>
####################################################

source $MARS_SCRIPTDIR/ubenchmarks/common.sh

sh $MARS_SCRIPTDIR/training/run_training.sh $UBENCH_BIN_DIR/training_singleapp_test $@


