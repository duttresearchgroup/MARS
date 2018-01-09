#ifndef __arm_rt_sensing_setup_h
#define __arm_rt_sensing_setup_h


/*
 * We should stop using the stuff from pal_setup.h in daemon code.
 * So the used-level sensing interfaces setup are defined in this file.
 *
 */


template<typename SensingModule>
void pal_sensing_setup(SensingModule *m);


template<typename SensingModule>
void pal_sensing_teardown(SensingModule *m);

#endif
