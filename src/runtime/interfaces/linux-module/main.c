
#include <linux/module.h>
#include <linux/kernel.h>

#include "../linux-module/core.h"
#include "../linux-module/sense.h"
#include "../linux-module/sensing_window.h"
#include "../linux-module/setup.h"
#include "../linux-module/user_if.h"


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
