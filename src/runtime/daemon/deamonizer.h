#ifndef __arm_daemon_daemonizer_h
#define __arm_daemon_daemonizer_h

#include <runtime/framework/system.h>

/*
 * Converts the current process into a daemon.
 *
 * If succesfull, the current process will exit from within this functions
 * (therefore, from the perspective of the caller, it never returns)
 * while a forked process will continue execution as a background processs
 * (henceforth, the daemon).
 *
 * It also initializes and parses input parameters (see rt_config_params.h)
 *
 * If the daemon process cannot be created the original process exits from
 * whithin this function with an exit value != 0
 *
 * If input parameters cannot be parsed, raises an exception in the daemon
 * process and exits with an exit value != 0
 *
 * The daemonized process stdout and stderr is redirected to /dev/kmsg
 */
void daemon_setup(int argc, char * argv[]);

/*
 * Runs the given system.
 * Should be called after daemon_setup returns in the daemon process.
 * This function never returns and the daemon process runs until SIGQUIT
 * is received and exits directly from the signal handler.
 *
 * The System object provide MUST be allocated using the new operator
 * since the daemon will always delete the object when SIGQUIT is
 * received.
 *
 * For convenience, a template version that creates the System object
 * of the specified type using the default constructor is provided.
 *
 */
void daemon_run_sys(System* sys);

template<typename Sys>
void daemon_run_sys()
{
	return daemon_run_sys(new Sys());
}


#endif
