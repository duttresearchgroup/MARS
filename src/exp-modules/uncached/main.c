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

#include "user_if.h"



static int __init sensingmodule_module_init(void)
{
	pinfo("Module loaded\n");

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

	pinfo("kernel module cleanup complete\n");
}

MODULE_LICENSE("GPL");

module_init(sensingmodule_module_init);
module_exit(sensingmodule_module_exit);
