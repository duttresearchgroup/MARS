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

##################################################################
# Parses all **training** traces saved at 
# $TRACE_OUTPUT_DIR-$RTS_ARCH-$RTS_ARCH (as defined in the confs.sh)
#
# Only traces whose *.raw directory has files newer than the
# corresponding .csv file will be parsed.
#
##################################################################

# Source the files we need
source $MARS_SCRIPTDIR/runtime/common.sh

TGT_TRACES_DIR=$TRACE_OUTPUT_DIR-$RTS_ARCH-$RTS_PLAT

if [ ! -d $TGT_TRACES_DIR ]
then
    echo "$TGT_TRACES_DIR is not a valid directory"
    exit 1
fi

RAWS=$(find  $TGT_TRACES_DIR -type d -name "training_singleapp*.raw" -print0 | xargs -0)
for raw in $RAWS
do
    _RAW_OUTPUT_DIR=$(basename $raw .raw)
    OUTPUT_TRACE_NAME=$TGT_TRACES_DIR/$_RAW_OUTPUT_DIR.csv
    
    if [ ! -f $OUTPUT_TRACE_NAME ]
    then
        echo "$OUTPUT_TRACE_NAME does not exists. Parsing..."
        sh $MARS_SCRIPTDIR/training/training_trace_parse.sh $raw
        echo -e "\n"
    else
        NEWERFILES=$(find $raw -type f -newer $OUTPUT_TRACE_NAME -print0 | xargs -0)
        if [ ! -z "$NEWERFILES" ]
        then
            echo "$raw has files newer than $OUTPUT_TRACE_NAME. Parsing..."
            sh $MARS_SCRIPTDIR/training/training_trace_parse.sh $raw
            echo -e "\n"
        fi
    fi
    
    if [ ! -f $OUTPUT_TRACE_NAME ]
    then
        echo "$OUTPUT_TRACE_NAME not generated !"
        echo "Some error happened when parsing $raw"
        exit 1
    fi
done

