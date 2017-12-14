#ifndef __task_map_top_level_cmds_h
#define __task_map_top_level_cmds_h

#define SASOLVER_MAX_ITER_HARD	 10000

enum {
	SD_SOLVER_STATUS_UNITIALIZED,
	SD_SOLVER_STATUS_IDLE,
	SD_SOLVER_STATUS_RUNNING,
	SD_SOLVER_STATUS_STOP_MAX_ITERATIONS,
	SD_SOLVER_STATUS_STOP_CRIT_MET
};

typedef void* (*vit_allocator_f)(long unsigned int);
typedef void (*vit_deallocator_f)(void*);
typedef void (*vit_assertion_f)(int,const char *,int,const char *);


#ifndef __KERNEL__
	#define LOG_RESULTS
#endif

#endif


