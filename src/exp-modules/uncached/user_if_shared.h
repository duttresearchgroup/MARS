#ifndef __arm_rt_user_shared_if
#define __arm_rt_user_shared_if

//Interface shared between this module and daemons

#define MODULE_SYSFS_PATH "/sys/kernel/debug/uncached"

#define SHARED_DATA_SIZE (sizeof(char)*1024*1024*2)

typedef enum {
	_IOCTLCMD_CACHEDALLOC=0,
	_IOCTLCMD_UNCACHEDALLOC_COHERENT,
	_IOCTLCMD_UNCACHEDALLOC_NONCOHERENT,
	SIZE_IOCTLCMD
} user_if_ioctl_cmds_t;

#define IOCTLCMD_CACHEDALLOC				_IOR('v', _IOCTLCMD_CACHEDALLOC, uint32_t)
#define IOCTLCMD_UNCACHEDALLOC_COHERENT		_IOR('v', _IOCTLCMD_UNCACHEDALLOC_COHERENT, uint32_t)
#define IOCTLCMD_UNCACHEDALLOC_NONCOHERENT	_IOR('v', _IOCTLCMD_UNCACHEDALLOC_NONCOHERENT, uint32_t)

#endif

