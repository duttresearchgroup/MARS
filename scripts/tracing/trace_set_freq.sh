########################################################
# Sets the governor of all cores to the specified one 
# and the governor of a specific core the given frequency
# Params:
#  - The target core
#  - The target freq
#  - The governor to be used for the other cores
########################################################

TRACED_CORE=$1
TRACE_FREQUENCY=$2
OTHER_CORES_GOVERNOR=$3

#echo $TRACED_CORE
#echo $TRACE_FREQUENCY
#echo $OTHER_CORES_GOVERNOR

echo $OTHER_CORES_GOVERNOR | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor >/dev/null
echo userspace | tee /sys/devices/system/cpu/cpu$TRACED_CORE/cpufreq/scaling_governor >/dev/null
echo $TRACE_FREQUENCY | tee /sys/devices/system/cpu/cpu$TRACED_CORE/cpufreq/scaling_setspeed >/dev/null
