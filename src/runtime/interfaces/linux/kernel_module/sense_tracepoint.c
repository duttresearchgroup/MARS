#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0))
    #include "sense_tracepoint_3_15.h"
#endif