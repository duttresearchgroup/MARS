
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
