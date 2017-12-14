
#include <runtime/daemon/deamonizer.h>
#include <runtime/systems/basic.h>

int main(int argc, char * argv[]){
	daemon_setup(argc,argv);
	const std::string& mode = rt_param_mode();
	daemon_run_sys(new OverheadTestSystem(mode));
	return 0;
}
