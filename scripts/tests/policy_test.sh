#!/bin/sh
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

source $SPARTA_SCRIPTDIR/runtime/common.sh

MODELS=$(readlink -f $MODEL_DIR/arm_exynos5422)

sudosh $SPARTA_SCRIPTDIR/runtime/start.sh policy_test model_path=$MODELS
sleep 1
sudosh $SPARTA_SCRIPTDIR/runtime/stop.sh


