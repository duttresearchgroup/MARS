/*
 * exceptions.h
 *
 *  Created on: Dec 8, 2016
 *      Author: tiago
 */

#ifndef __core_base_exceptions_h
#define __core_base_exceptions_h

#include "portability.h"

//not kernel and C++
#ifndef __KERNEL__
#ifdef __cplusplus

#include <system_error>
#include <string>
#include <cstdio>
#include <cstdarg>

class DaemonException : public std::exception
{
	static const int MSG_BUFFER_SIZE = 100;

	const char* _name;
	const char* _file;
	int 		_linen;
	char _msg[MSG_BUFFER_SIZE];

public:
	/** Takes a character string describing the error.  */
	explicit DaemonException(const char* name, const char* file, int linen,const char* fmt,...)
		:_name(name),_file(file),_linen(linen)
	{
		va_list ap;
		va_start (ap, fmt);
		vsnprintf(_msg,MSG_BUFFER_SIZE,fmt,ap);
		va_end (ap);
	}

	virtual ~DaemonException() noexcept {};

	/** Returns a C-style character string describing the general cause of
	 *  the current error (the same string passed to the ctor).  */
	virtual const char* what() const noexcept { return _msg;}

	const char* name() const noexcept { return _name;}
	const char* file() const noexcept { return _file;}
	int line() const noexcept { return _linen;}
};

#define arm_throw(name,msg,...) throw DaemonException(#name, __FILE__,__LINE__,msg,##__VA_ARGS__)

static inline void ARM_CATCH_NO_EXIT(){}

#define arm_catch(exit_func,...)\
    catch (const DaemonException& e) {\
	    pinfo("Exception: %s\n  thrown at %s:%d\n  caught at %s:%d\n  msg: %s\n",e.name(),e.file(),e.line(),__FILE__,__LINE__,e.what());\
	    exit_func(__VA_ARGS__);\
	} catch (const std::exception & e) {\
		pinfo("Exception: std::exception\n  thrown at ???\n  caught at %s:%d\n  what(): %s\n",__FILE__,__LINE__,e.what());\
	    exit_func(__VA_ARGS__);\
	} catch (...){\
		pinfo("Exception: ????\n  thrown at ???\n  caught at %s:%d\n",__FILE__,__LINE__);\
		exit_func(__VA_ARGS__);\
	}


#define assert_false(cond) if(cond) arm_throw(AssertFailed,"%s == true",#cond)
#define assert_true(cond) if(!(cond)) arm_throw(AssertFailed,"%s == false",#cond)

#define BUG_ON(cond) assert_false(cond)

#else //not c++

#define assert_false(cond) assert(!(cond))
#define assert_true(cond) assert(cond)

#define BUG_ON(cond) assert_false(cond)

#endif

#else //in KERNEL

#define assert_false(cond) BUG_ON(cond)
#define assert_true(cond) BUG_ON(!(cond))

#endif

#endif /* SENSING_MODULE_H_ */
