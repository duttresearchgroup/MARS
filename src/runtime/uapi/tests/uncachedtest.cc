
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdexcept>
#include <system_error>
#include <sys/time.h>

#include "../../../exp-modules/uncached/user_if_shared.h"

inline long int time_us(){
	struct timeval now;
	gettimeofday(&now, NULL);
	long int us_now = now.tv_sec * 1000000 + now.tv_usec;
	return us_now;
}


struct ModuleIF
{
	int _module_file_if;
	void* cached_mem_ptr;
	void* uncached_cohr_mem_ptr;
	//void* uncached_noncohr_mem_ptr;

	ModuleIF();

	~ModuleIF();

};

ModuleIF::ModuleIF()
	:_module_file_if(0), cached_mem_ptr(nullptr),uncached_cohr_mem_ptr(nullptr)//,uncached_noncohr_mem_ptr(nullptr)
{
	_module_file_if = open(MODULE_SYSFS_PATH, O_RDWR);
    if(_module_file_if < 0)
    	printf("Vitamins module %s not inserted errno=%d\n",MODULE_SYSFS_PATH,errno);

    if(ioctl(_module_file_if, IOCTLCMD_CACHEDALLOC,0) !=0)
    	printf("IOCTLCMD_CACHEDALLOC failed errno=%d\n",errno);
    cached_mem_ptr = mmap(NULL, SHARED_DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, _module_file_if, 0);
    if(cached_mem_ptr == MAP_FAILED)
    	printf("mmap error\n");

    if(ioctl(_module_file_if, IOCTLCMD_UNCACHEDALLOC_COHERENT,0) !=0)
    	printf("IOCTLCMD_UNCACHEDALLOC_COHERENT failed errno=%d\n",errno);
    uncached_cohr_mem_ptr = mmap(NULL, SHARED_DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, _module_file_if, 0);
    if(uncached_cohr_mem_ptr == MAP_FAILED)
    	printf("mmap error\n");

    //if(ioctl(_module_file_if, IOCTLCMD_UNCACHEDALLOC_NONCOHERENT,0) !=0)
    // 	printf("IOCTLCMD_UNCACHEDALLOC_NONCOHERENT failed errno=%d\n",errno);
    //uncached_noncohr_mem_ptr = mmap(NULL, SHARED_DATA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, _module_file_if, 0);
    //if(uncached_noncohr_mem_ptr == MAP_FAILED)
    // 	printf("mmap error\n");
}

ModuleIF::~ModuleIF()
{
	if(munmap(cached_mem_ptr,SHARED_DATA_SIZE) < 0)
    	printf("munmap failed with errno=%d!\n",errno);

	if(munmap(uncached_cohr_mem_ptr,SHARED_DATA_SIZE) < 0)
    	printf("munmap failed with errno=%d!\n",errno);

	//if(munmap(uncached_noncohr_mem_ptr,SHARED_DATA_SIZE) < 0)
    //	printf("munmap failed with errno=%d!\n",errno);

    if(close(_module_file_if) < 0)
    	printf("close failed with errno=%d!\n",errno);
}


void write(void* _data){
	auto start = time_us();
	int *data = (int*)_data;
	for(int val = 0; val < 2000; ++val){
		for(unsigned i = 0; i < (SHARED_DATA_SIZE/sizeof(int)); ++i){
			data[i] = val;
		}
	}
	auto end = time_us()-start;
	printf("Total time writing = %f secs\n",end/1000000.0);
}

void read(const char * type, void* _data){
	int *data = (int*)_data;
	printf("%s 0=%d, n/2=%d, n-1=%d\n",type,data[0],data[(SHARED_DATA_SIZE/sizeof(int))/2],data[(SHARED_DATA_SIZE/sizeof(int))-1]);
}


int main(int argc, char * argv[]){

	if(!((argc == 3) || (argc == 2))){
		printf("1st arg must be read|write; 2nd arg must be cached|uncached|uncached_nonc\n");
		return 0;
	}

	std::string readWrite(argv[1]);

	ModuleIF milf;

	if(readWrite == "write"){
		std::string cacheUncache(argv[2]);

		printf("Writer mode!\n");

		if(cacheUncache == "cached"){
			printf("Writing to cached data!\n");
			write(milf.cached_mem_ptr);
		}
		else if(cacheUncache == "uncached"){
			printf("Writing to uncached coherent data!\n");
			write(milf.uncached_cohr_mem_ptr);
		}
		else if(cacheUncache == "uncached_nonc"){
			//printf("Writing to uncached non coherent data!\n");
			//write(milf.uncached_noncohr_mem_ptr);
			printf("Writing to uncached non coherent data is not working!\n");
		}
		else{
			printf("1st arg must be read|write; 2nd arg must be cached|uncached|uncached_nonc\n");
			return 0;
		}
	}
	else if(readWrite == "read"){
		printf("Reader mode!\n");

		while(true){
			read("cc",milf.cached_mem_ptr);
			read("uc",milf.uncached_cohr_mem_ptr);
			//read("un",milf.uncached_noncohr_mem_ptr);
			sleep(1);
		}


		//printf("Reading from priv data!\n");
		//read(priv_data);

	}
	else{
		printf("1st arg must be read|write; 2nd arg must be cached|uncached|uncached_nonc\n");
	}

	return 0;
}
