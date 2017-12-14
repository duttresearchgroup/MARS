
#include <runtime/daemon/deamonizer.h>
#include <runtime/systems/basic.h>

int main(int argc, char * argv[]){
	daemon_setup(argc,argv);
	daemon_run_sys<InterfaceTest>();
	return 0;
}
