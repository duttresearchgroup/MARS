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

#include <linux/module.h>
#include <linux/kernel.h>

#include "core.h"
#include "sense.h"
#include "sensing_window.h"
#include "setup.h"
#include "user_if.h"


static bool sensingmodule_init(void){
    init_system_info();

    //create base mechanisms for sensing
    if(create_queues() == false) return false;
    if(sense_create_mechanisms(system_info()) == false) return false;

    sense_init(system_info());

    return true;
}

static void sensingmodule_exit(void){
	sys_info_t* sys_info = system_info();

	sense_cleanup(sys_info);

	sense_destroy_mechanisms(sys_info);
	destroy_queues();
}

static int __init sensingmodule_module_init(void)
{
	//pinfo("sensingmodule_init()\n");

    if (sensingmodule_init() == false) return -1;

    //last thing we do is creating the user debugfs interface
    if(create_user_if() == false){
    	pinfo("Could not create debugfs interface!\n");
    	return -1;
    }

    pinfo("kernel module ready\n");

    return 0;
}

static void __exit sensingmodule_module_exit(void)
{
	//very first thing is disabling the debugfs stuff so the user won't mess up
	destroy_user_if();

	if(sensing_running()){
		pinfo("Module exiting with sensing still running!\n");
		stop_sensing_windows();
	}

    sensingmodule_exit();

    pinfo("kernel module cleanup complete\n");
}

MODULE_LICENSE("GPL");

module_init(sensingmodule_module_init);
module_exit(sensingmodule_module_exit);
