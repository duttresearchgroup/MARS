########################################################
# Sets the governor of all cores to the specified one 
# Params:
#  - The governor to be used for the other cores
########################################################

GOVERNOR=$1

echo $GOVERNOR | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor >/dev/null
