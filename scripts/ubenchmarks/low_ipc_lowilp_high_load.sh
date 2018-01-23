#!/bin/sh

# Includes the confs
source $SPARTA_SCRIPTDIR/ubenchmarks/common.sh

$UBENCH_BIN_DIR/ubenchmark_periodic $P_LOWIPC_HIGHLOAD2

