/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * Copyright (C) 2018 Bryan Donyanavard <bdonyana@uci.edu>
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

#include "deamonizer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>


static void daemon_init(int argc, char * argv[]);
static void daemon_start(System* sys);
static void daemonize();

void daemon_setup(int argc, char * argv[]) {
	try {
		//becomes a demon
		daemonize();

		//all inits
		daemon_init(argc,argv);

	} arm_catch(ARM_CATCH_NO_EXIT);
}

void daemon_run_sys(System* sys) {
	try {
        daemon_start(sys);

        //sensing window threads are running. Quits from signal handler
        for (;;) pause();

		//should not reach here
        pinfo("Very unnexpected error at %s:%d\n",__FILE__,__LINE__);

    } arm_catch(ARM_CATCH_NO_EXIT);

    //should not reach here, unless exiting from exception
    exit(EXIT_FAILURE);
}


static System *rtsys = nullptr;

static void daemon_exit(int sig) {
	try {
		switch (sig) {
		case SIGQUIT:
			pinfo("Exit signal received...\n");
			rtsys->stop();
			delete rtsys;
			pinfo("Cleaning up done\n");
			exit(EXIT_SUCCESS);
			break;
		default:
			pinfo("Wasn't expecting signal %d !!!\n",sig);
			exit(EXIT_FAILURE);
		}
	} arm_catch(ARM_CATCH_NO_EXIT);

	exit(EXIT_FAILURE);
}

static void daemon_init(int argc, char * argv[]) {

	//grab SIGQUIT for clean exit
	signal(SIGQUIT, daemon_exit);

	//init params

	if(!init_rt_config_params(argc, (const char **)argv))
		arm_throw(DaemonInitException,"Error parsing daemon initialization params");

	rt_param_print();


    if(rtsys != nullptr)
    	arm_throw(DaemonInitException,"Only one Daemon system is allowed");
}

static void daemon_start(System* sys) {
    if(sys == nullptr)
        arm_throw(DaemonInitException,"Given system is null");

    rtsys = sys;
    rtsys->start();
}

static void daemonize() {
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0) arm_throw(DaemonInitException,"Pid < 0");

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0) arm_throw(DaemonInitException,"setsid() < 0");

    /* Catch, ignore and handle signals */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0) arm_throw(DaemonInitException,"Pid < 0");

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    if(chdir("/")!=0) arm_throw(DaemonInitException,"chdir error. errno = %d",errno);
#if (PLAT==gem5)
    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>0; x--)
    {
        close (x);
    }

    /* Open the log file */
	//redirects printfs to the ring buffer. Must always flush after every print
    stdout = fopen("/dev/kmsg","w");
    if(stdout == NULL)
    	arm_throw(DaemonInitException,"Couldn't point stdout to /dev/ksmg errno=%d",errno);
    stderr = fopen("/dev/kmsg","w");
    if(stderr == NULL)
    	arm_throw(DaemonInitException,"Couldn't point stderr to /dev/ksmg errno=%d",errno);
#endif
    pinfo("Process deamoninzed. pid %d\n",getpid());
}

