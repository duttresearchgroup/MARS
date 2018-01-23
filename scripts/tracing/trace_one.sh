###########################################################
# Wrapper for doing trace_one-run.sh and trace_one-parse.sh
# together.
###########################################################

sudosh $SPARTA_SCRIPTDIR/tracing/trace_one-run.sh $@
sh $SPARTA_SCRIPTDIR/tracing/trace_one-parse.sh $@

