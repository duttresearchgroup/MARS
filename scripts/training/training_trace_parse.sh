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

##########################################################
# This needs to be used instead of trace_parse.sh
# to parse training traces. It first sanitizes the traces
# (generating one sample per training app) and only then
# calls trace_parse.sh for the aggregation.
#
# Usage:
#   Same as trace_parse.sh
################################################

RAW_OUTPUT_DIR=$1

if [[ $RAW_OUTPUT_DIR != *.raw ]]
then
    echo "Invalid trace path $RAW_OUTPUT_DIR"
    echo "Expect path ending with .raw"
    exit 1
fi
if [ ! -d $RAW_OUTPUT_DIR ]
then
    echo "$RAW_OUTPUT_DIR is not a valid directory"
    exit 1
fi

EXPECTED_TRAINING_APP_NAME="*/training_singleapp*.raw"
if [[ $RAW_OUTPUT_DIR != $EXPECTED_TRAINING_APP_NAME ]]
then
    echo "$RAW_OUTPUT_DIR is not a training trace!"
    exit 1
fi

# Source the files we need
source $SPARTA_SCRIPTDIR/runtime/common.sh

echo "Sanitizing periodic traces"
PERIDIC_TRACES=$(ls $RAW_OUTPUT_DIR/periodic_trace_result*.csv | xargs)
for trace in $PERIDIC_TRACES
do
    destfile="${trace/%.csv/.$TP_SANITIZE_EXT}"
    python3 $SPARTA_SCRIPTDIR/training/sanitize.py --srcfile $trace --destfile $destfile
done

# Now parses periodic traces
sh $SPARTA_SCRIPTDIR/tracing/trace_parse.sh $RAW_OUTPUT_DIR







