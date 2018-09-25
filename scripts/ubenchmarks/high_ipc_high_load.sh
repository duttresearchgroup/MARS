#!/bin/sh

# Includes the confs
source $MARS_SCRIPTDIR/ubenchmarks/common.sh

$UBENCH_BIN_DIR/ubenchmark_periodic $P_HIGHIPC_HIGHLOAD

