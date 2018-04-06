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

# Runs the bin-based predictor training app and saves predictor information 
# in the models directory defined in confs.sh

source $SPARTA_SCRIPTDIR/confs.sh

TGT_MODELS_DIR=models/"$RTS_ARCH"_"$RTS_PLAT"
PARSED_TRACES_DIR=$TRACE_OUTPUT_DIR-$RTS_ARCH-$RTS_PLAT
POWER_INFO_FILE=$PARSED_TRACES_DIR/idle_power.json

if [ ! -e $POWER_INFO_FILE ]
then
    echo "Idle power file $POWER_INFO_FILE does not exist"
    echo "Are you sure you have training traces ?"
    exit 1
fi

TRACES_CNT=$(find $PARSED_TRACES_DIR -name "training_singleapp*.csv" | wc -l)
if [ "$TRACES_CNT" -lt "1" ]; then
    echo "No training traces found in $PARSED_TRACES_DIR"
    echo "Are you sure you have training traces ?"
    exit 1
fi

mkdir -p models/"$RTS_ARCH"_"$RTS_PLAT"

bin_"$APPS_ARCH"_"$APPS_PLAT"/apps/predictor_trainer models/"$RTS_ARCH"_"$RTS_PLAT"/bin_pred.json $PARSED_TRACES_DIR
cp $POWER_INFO_FILE models/"$RTS_ARCH"_"$RTS_PLAT"/
