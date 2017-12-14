#ifndef __core_base_base_h
#define __core_base_base_h

//Include this header to use base defs in both kernel,daemon, and applications
//This is the only header that is safe to include in both C/C++ code
//All other 'core' headers are C++ only

#include "portability.h"
#include "_exceptions.h"

CBEGIN

#include "_defs.h"
#include "_converters.h"
#include "_dvfs.h"
#include "_lists.h"
#include "_power.h"
#include "_scaling.h"
#include "_info_init.h"
#include "_fileio.h"

CEND

#endif
