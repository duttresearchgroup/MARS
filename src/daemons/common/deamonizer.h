/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef __arm_daemon_daemonizer_h
#define __arm_daemon_daemonizer_h

#include <type_traits>

#include <runtime/framework/policy.h>

/*
 * Converts the current process into a daemon.
 *
 * If successful, the current process will exit from within this functions
 * (therefore, from the perspective of the caller, it never returns)
 * while a forked process will continue execution as a background process
 * (henceforth, the daemon).
 *
 * It also initializes and parses input parameters (see rt_config_params.h)
 *
 * If the daemon process cannot be created the original process exits from
 * within this function with an exit value != 0
 *
 * If input parameters cannot be parsed, raises an exception in the daemon
 * process and exits with an exit value != 0
 *
 * The daemonized process stdout and stderr is redirected to /dev/kmsg
 * according to platform settings.
 */
void _daemon_setup(int argc, char * argv[]);

/*
 * Runs the given system.
 * Should be called after _daemon_setup returns in the daemon process.
 * This function never returns and the daemon process runs until SIGQUIT
 * is received and exits directly from the signal handler.
 *
 * The PolicyManager object provide MUST be allocated using the new operator
 * since the daemon will always delete the object when SIGQUIT is
 * received.
 *
 * For safety, always use the daemon_run() template. Calling _daemon_setup
 * and _daemon_run should be only for special cases when the PolicyManager
 * cannot be constructed in the usual way.
 *
 */
void _daemon_run(PolicyManager* sys);

/*
 * Forks into a daemon process and creates the sensing interface and policy
 * manager
 */
template<typename Sys>
void daemon_run(int argc, char * argv[])
{
    _daemon_setup(argc,argv);

    Sys *pm = nullptr;
    try {
        pm = new Sys(new SensingModule());
    } arm_catch(exit,EXIT_FAILURE)

    _daemon_run(pm);
}


#endif
