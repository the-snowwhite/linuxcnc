#ifndef RTAPI_H
#define RTAPI_H

/** RTAPI is a library providing a uniform API for several real time
    operating systems.  As of ver 2.0, RTLinux and RTAI are supported.
*/
/********************************************************************
* Description:  rtapi.h
*               This file, 'rtapi.h', defines the RTAPI for both 
*               realtime and non-realtime code.
*
* Author: John Kasunich, Paul Corner
* License: LGPL Version 2.1
*
* Copyright (c) 2004 All rights reserved.
*
* Last change: 
********************************************************************/

/** This file, 'rtapi.h', defines the RTAPI for both realtime and
    non-realtime code.  This is a change from Rev 2, where the non-
    realtime (user space) API was defined in ulapi.h and used
    different function names.  The symbols RTAPI and ULAPI are used
    to determine which mode is being compiled, RTAPI for realtime
    and ULAPI for non-realtime.  The API is implemented in files
    named 'xxx_rtapi.c', where xxx is the RTOS.
*/

/** Copyright (C) 2003 John Kasunich
                       <jmkasunich AT users DOT sourceforge DOT net>
    Copyright (C) 2003 Paul Corner
                       <paul_c AT users DOT sourceforge DOT net>
    This library is based on version 1.0, which was released into
    the public domain by its author, Fred Proctor.  Thanks Fred!
*/

/** This library is free software; you can redistribute it and/or
    modify it under the terms of version 2.1 of the GNU Lesser General
    Public License as published by the Free Software Foundation.
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111 USA

    THE AUTHORS OF THIS LIBRARY ACCEPT ABSOLUTELY NO LIABILITY FOR
    ANY HARM OR LOSS RESULTING FROM ITS USE.  IT IS _EXTREMELY_ UNWISE
    TO RELY ON SOFTWARE ALONE FOR SAFETY.  Any machinery capable of
    harming persons must have provisions for completely removing power
    from all motors, etc, before persons enter any danger area.  All
    machinery must be designed to comply with local and national safety
    codes, and the authors of this software can not, and do not, take
    any responsibility for such compliance.

    This code was written as part of the EMC HAL project.  For more
    information, go to www.linuxcnc.org.
*/

/*
  RTAPI_SERIAL should be bumped with changes that break compatibility
  with previous versions.
*/
#define RTAPI_SERIAL 2
#include "config.h"

#if ( !defined RTAPI ) && ( !defined ULAPI )
#error "Please define either RTAPI or ULAPI!"
#endif
#if ( defined RTAPI ) && ( defined ULAPI )
#error "Can't define both RTAPI and ULAPI!"
#endif

#include <stddef.h> // provides NULL

/** Provide fixed length types of the form __u8, __s32, etc.  These
    can be used in both kernel and user space.  There are also types
    without the leading underscores, but they work in kernel space
    only.  Since we have a simulator that runs everything in user
    space, the non-underscore types should NEVER be used.
*/
#if defined(BUILD_SYS_USER_DSO)
#define __KERNEL_STRICT_NAMES
# include <linux/types.h>
#if !defined(__GNUC__) && defined(__STRICT_ANSI__)
# include <stdint.h>
#endif
# include <string.h>
typedef __u8		u8;
typedef __u16		u16;
typedef __u32		u32;
typedef __u64		u64;
typedef __s8		s8;
typedef __s16		s16;
typedef __s32		s32;
typedef __s64		s64;
#define __iomem		/* Nothing */
#else
# include <asm/types.h>
#endif


/* LINUX_VERSION_CODE for rtapi_{module,io}.c */
#ifdef MODULE
#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#endif
#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif
#endif

#include <rtapi_errno.h>
#include <rtapi_ring.h>
#include <rtapi_global.h>
#include <rtapi_exception.h>

#define RTAPI_NAME_LEN   31	/* length for module, etc, names */

#ifdef __cplusplus
#define RTAPI_BEGIN_DECLS extern "C" {
#define RTAPI_END_DECLS }
#else
#define RTAPI_BEGIN_DECLS
#define RTAPI_END_DECLS
#endif

RTAPI_BEGIN_DECLS



/***********************************************************************
*                    INIT AND EXIT FUNCTIONS                           *
************************************************************************/
/* implemented in $THREADS.c (rt-preempt.c, xenomai.c, etc.) */

/** 'rtapi_init() sets up the RTAPI.  It must be called by any
    module that intends to use the API, before any other RTAPI
    calls.
    'modname' can optionally point to a string that identifies
    the module.  The string will be truncated at RTAPI_NAME_LEN
    characters.  If 'modname' is NULL, the system will assign a
    name.
    On success, returns a positive integer module ID, which is
    used for subsequent calls to rtapi_xxx_new, rtapi_xxx_delete,
    and rtapi_exit.  On failure, returns an error code as defined
    above.  Call only from within user or init/cleanup code, not
    from realtime tasks.
*/
typedef int (*rtapi_init_t)(const char *);
#define rtapi_init(modname)			\
    rtapi_switch->rtapi_init(modname)
extern int _rtapi_init(const char *modname);

/** 'rtapi_exit()' shuts down and cleans up the RTAPI.  It must be
    called prior to exit by any module that called rtapi_init.
    'module_id' is the ID code returned when that module called
    rtapi_init().
    Returns a status code.  rtapi_exit() may attempt to clean up
    any tasks, shared memory, and other resources allocated by the
    module, but should not be relied on to replace proper cleanup
    code within the module.  Call only from within user or
    init/cleanup code, not from realtime tasks.
*/
typedef int (*rtapi_exit_t)(int);
#define rtapi_exit(module_id)			\
    rtapi_switch->rtapi_exit(module_id)
extern int _rtapi_exit(int module_id);

/** 'rtapi_next_module_id()' returns a globally unique int ID
    
 */
typedef int (*rtapi_next_module_id_t)(void);
#define rtapi_next_module_id()			\
    rtapi_switch->rtapi_next_module_id()
extern int _rtapi_next_module_id(void);


/***********************************************************************
*             MODULE LOADING AND CHECKING FUNCTIONS                    *
************************************************************************/

#if defined(ULAPI) || defined(BUILD_SYS_USER_DSO)
#include <limits.h> // provides PATH_MAX


extern int is_module_loaded(const char *module);
extern int load_module(const char *module, const char *modargs);
extern long int simple_strtol(const char *nptr, char **endptr, int base);

// kernel tests in rtapi_compat.c
extern int kernel_is_xenomai();
extern int kernel_is_rtai();
extern int kernel_is_rtpreempt();

#endif // ULAPI

enum flavor_flags {
    FLAVOR_DOES_IO=1,		// whether iopl() needs to be called
    FLAVOR_KERNEL_BUILD=2,	// set when defined(BUILD_SYS_KBUILD)
};

typedef struct {
    const char *name;
    const char *mod_ext;	// RTAPI module extensions, .ko/.so
    const char *so_ext;		// ulapi.so module extension
    const char *build_sys;
    int id;
    unsigned long flags;

} flavor_t, *flavor_ptr;

extern flavor_t flavors[];

extern flavor_ptr flavor_byname(const char *flavorname);
extern flavor_ptr flavor_byid(int flavor_id);
extern flavor_ptr default_flavor(void);

/*
 * Given a result buffer of PATH_MAX size and a module or shared
 * library's basename (e.g. 'rtapi' with no directory or '.ko'), find
 * the full path to a module/shared library as follows:
 *
 * Complete the module file name by appending mod_ext from flavor data
 * to the basename (e.g. 'rtapi.ko').
 *
 * For RIP builds, prepend RTLIB_DIR from rtapi.ini, flavor name and
 * kernel release to the module file name
 * (e.g. '/home/me/linuxcnc-dev/rtlib/rtai-kernel/2.6.38-rtai/rtapi.ko');
 * if the file exists, copy into *result and return 0.
 *
 * For non-RIP builds, prepend
 * '/lib/modules/<kernel-release>/linuxcnc' to the module file name
 * (e.g. '/lib/modules/2.6.38-rtai/linuxcnc/rtapi.ko'); if the file
 * exists, copy into *result and return 0.
 *
 * Otherwise, prepend any (flavor-specific, currently RTAI only) RTDIR
 * from rtapi.ini and prepend to module file name
 * (e.g. '/usr/realtime/modules/rtai_hal.ko'); if the file exists,
 * copy into *result and return 0.
 *
 * Otherwise return non-0.
 */

extern int module_path(char *result, const char *basename);

/*
 * Look up a parameter value in rtapi.ini, checking first the
 * [flavor_<flavor>] section, then the [global] section.  Returns 0 if
 * successful, 1 otherwise.  Maximum n-1 bytes of the value and a
 * trailing \0 is copied into *result.
 *
 * Beware:  this function calls exit(-1) if rtapi.ini cannot be
 * successfully opened!
 */

extern int get_rtapi_config(char *result, const char *param, int n);


/***********************************************************************
*                      MESSAGING FUNCTIONS                             *
************************************************************************/
/* implemented in rtapi_support.c */

#include <stdarg.h>		/* va_start and va_end macros */

/** 'rtapi_snprintf()' works like 'snprintf()' from the normal
    C library, except that it may not handle long longs.
    It is provided here because some RTOS kernels don't provide
    a realtime safe version of the function, and those that do don't provide
    support for printing doubles.  On systems with a
    good kernel snprintf(), or in user space, this function
    simply calls the normal snprintf().  May be called from user,
    init/cleanup, and realtime code.
*/
extern int rtapi_snprintf(char *buf, unsigned long int size,
			   const char *fmt, ...)
    __attribute__((format(printf,3,4)));

/** 'rtapi_vsnprintf()' works like 'vsnprintf()' from the normal
    C library, except that it doesn't handle long longs.
    It is provided here because some RTOS kernels don't provide
    a realtime safe version of the function, and those that do don't provide
    support for printing doubles.  On systems with a
    good kernel vsnprintf(), or in user space, this function
    simply calls the normal vsnrintf().  May be called from user,
    init/cleanup, and realtime code.
*/
extern int rtapi_vsnprintf(char *buf, unsigned long size,
			    const char *fmt, va_list ap);

/** 'rtapi_print()' prints a printf style message.  Depending on the
    RTOS and whether the program is being compiled for user space
    or realtime, the message may be printed to stdout, stderr, or
    to a kernel message log, etc.  The calling syntax and format
    string is similar to printf except that floating point and
    longlongs are NOT supported in realtime and may not be supported
    in user space.  For some RTOS's, a 80 byte buffer is used, so the
    format line and arguments should not produce a line more than
    80 bytes long.  (The buffer is protected against overflow.)
    Does not block, but  can take a fairly long time, depending on
    the format string and OS.  May be called from user, init/cleanup,
    and realtime code.
*/
extern void rtapi_print(const char *fmt, ...)
    __attribute__((format(printf,1,2)));

/** 'rtapi_print_msg()' prints a printf-style message when the level
    is less than or equal to the current message level set by
    rtapi_set_msg_level().  May be called from user, init/cleanup,
    and realtime code.
*/
    typedef enum {
	RTAPI_MSG_NONE = 0,
	RTAPI_MSG_ERR,
	RTAPI_MSG_WARN,
	RTAPI_MSG_INFO,
	RTAPI_MSG_DBG,
	RTAPI_MSG_ALL
    } msg_level_t;

extern void rtapi_print_msg(int level, const char *fmt, ...)
    __attribute__((format(printf,2,3)));

/** Set the maximum level of message to print.  In userspace code,
    each component has its own independent message level.  In realtime
    code, all components share a single message level.  Returns 0 for
    success or -EINVAL if the level is out of range. */
extern int rtapi_set_msg_level(int level);

/** Retrieve the message level set by the last call to rtapi_set_msg_level */
extern int rtapi_get_msg_level(void);

/** 'rtapi_get_msg_handler' and 'rtapi_set_msg_handler' access the function
    pointer used by rtapi_print and rtapi_print_msg.  By default, messages
    appear in the kernel log, but by replacing the handler a user of the rtapi
    library can send the messages to another destination.  Calling
    rtapi_set_msg_handler with NULL restores the default handler. Call from
    real-time init/cleanup code only.  When called from rtapi_print(),
    'level' is RTAPI_MSG_ALL, a level which should not normally be used
    with rtapi_print_msg().
*/
typedef void(*rtapi_msg_handler_t)(msg_level_t level, const char *fmt,
				   va_list ap);

// message handler which writes to ringbuffer if global is available
extern void vs_ring_write(msg_level_t level, const char *format, va_list ap);

#ifdef RTAPI
typedef void (*rtapi_set_msg_handler_t)(rtapi_msg_handler_t);

extern void rtapi_set_msg_handler(rtapi_msg_handler_t handler);

typedef rtapi_msg_handler_t (*rtapi_get_msg_handler_t)(void);

extern rtapi_msg_handler_t rtapi_get_msg_handler(void);
#endif // RTAPI

extern int rtapi_set_logtag(const char *fmt, ...);

typedef enum {
	MSG_KERNEL = 0,
	MSG_RTUSER = 1,
	MSG_ULAPI = 2,
} msg_origin_t;

typedef enum {
    MSG_ASCII    = 0,  // printf conversion already applied
    MSG_STASHF   = 1,  // Jeff's stashf.c argument encoding
    MSG_PROTOBUF = 2,  // encoded as protobuf RTAPI_Message
} msg_encoding_t;

#define TAGSIZE 16

typedef struct {
    msg_origin_t   origin;   // where is this coming from
    int pid;                 // if User RT or ULAPI; 0 for kernel
    int level;               // as passed in to rtapi_print_msg()
    char tag[TAGSIZE];       // eg program or module name
    msg_encoding_t encoding; // how to interpret buf
    char buf[0];             // actual message
} rtapi_msgheader_t;

#define rtapi2syslog(level) (level+2)


/***********************************************************************
*                  LIGHTWEIGHT MUTEX FUNCTIONS                         *
************************************************************************/
#ifdef MODULE  /* kernel code */
#include <linux/sched.h>	/* for blocking when needed */
#else  /* userland code */
#include <sched.h>		/* for blocking when needed */
#endif
#include "rtapi_bitops.h"	/* atomic bit ops for lightweight mutex */

/** These three functions provide a very simple way to do mutual
    exclusion around shared resources.  They do _not_ replace
    semaphores, and can result in significant slowdowns if contention
    is severe.  However, unlike semaphores they can be used from both
    user and kernel space.  The 'try' and 'give' functions are non-
    blocking, and can be used anywhere.  The 'get' function blocks if
    the mutex is already taken, and can only be used in user space or
    the init code of a realtime module, _not_ in realtime code.
*/

/** 'rtapi_mutex_give()' releases the mutex pointed to by 'mutex'.
    The release is unconditional, even if the caller doesn't have
    the mutex, it will be released.
*/
    static __inline__ void rtapi_mutex_give(unsigned long *mutex) {
	rtapi_test_and_clear_bit(0, mutex);
    }
/** 'rtapi_mutex_try()' makes a non-blocking attempt to get the
    mutex pointed to by 'mutex'.  If the mutex was available, it
    returns 0 and the mutex is no longer available, since the
    caller now has it.  If the mutex is not available, it returns
    a non-zero value to indicate that someone else has the mutex.
    The programer is responsible for "doing the right thing" when
    it returns non-zero.  "Doing the right thing" almost certainly
    means doing something that will yield the CPU, so that whatever
    other process has the mutex gets a chance to release it.
*/ static __inline__ int rtapi_mutex_try(unsigned long *mutex) {
	return rtapi_test_and_set_bit(0, mutex);
    }

/** 'rtapi_mutex_get()' gets the mutex pointed to by 'mutex',
    blocking if the mutex is not available.  Because of this,
    calling it from a realtime task is a "very bad" thing to
    do.
*/
    static __inline__ void rtapi_mutex_get(unsigned long *mutex) {
	while (rtapi_test_and_set_bit(0, mutex)) {
#ifdef MODULE  /* kernel code */
	    schedule();
#else  /* userland code */
	    sched_yield();
#endif
	}
    }

/***********************************************************************
*                      TIME RELATED FUNCTIONS                          *
************************************************************************/
/* implemented in rtapi_time.c */

/** NOTE: These timing related functions are only available in
    realtime modules.  User processes may not call them!
*/
#ifdef RTAPI

/** 'rtapi_clock_set_period() sets the basic time interval for realtime
    tasks.  All periodic tasks will run at an integer multiple of this
    period.  The first call to 'rtapi_clock_set_period() with 'nsecs'
    greater than zero will start the clock, using 'nsecs' as the clock
    period in nano-seconds.  Due to hardware and RTOS limitations, the
    actual period may not be exactly what was requested.  On success,
    the function will return the actual clock period if it is available,
    otherwise it returns the requested period.  If the requested period
    is outside the limits imposed by the hardware or RTOS, it returns
    -EINVAL and does not start the clock.  Once the clock is started,
    subsequent calls with non-zero 'nsecs' return -EINVAL and have
    no effect.  Calling 'rtapi_clock_set_period() with 'nsecs' set to
    zero queries the clock, returning the current clock period, or zero
    if the clock has not yet been started.  Call only from within
    init/cleanup code, not from realtime tasks.  This function is not
    available from user (non-realtime) code.
*/
typedef long int (*rtapi_clock_set_period_t)(long int);
#define rtapi_clock_set_period(nsecs)		\
    rtapi_switch->rtapi_clock_set_period(nsecs)
extern long int _rtapi_clock_set_period(long int nsecs);

/** rtapi_delay() is a simple delay.  It is intended only for short
    delays, since it simply loops, wasting CPU cycles.  'nsec' is the
    desired delay, in nano-seconds.  'rtapi_delay_max() returns the
    max delay permitted (usually approximately 1/4 of the clock period).
    Any call to 'rtapi_delay()' requesting a delay longer than the max
    will delay for the max time only.  'rtapi_delay_max()' should be
    called befure using 'rtapi_delay()' to make sure the required delays
    can be achieved.  The actual resolution of the delay may be as good
    as one nano-second, or as bad as a several microseconds.  May be
    called from init/cleanup code, and from within realtime tasks.
*/
typedef void (*rtapi_delay_t)(long int);
#define rtapi_delay(nsec)			\
    rtapi_switch->rtapi_delay(nsec)
extern void _rtapi_delay(long int nsec);

typedef long int (*rtapi_delay_max_t)(void);
#define rtapi_delay_max()			\
    rtapi_switch->rtapi_delay_max()
extern long int _rtapi_delay_max(void);

#endif /* RTAPI */

/** rtapi_get_time returns the current time in nanoseconds.  Depending
    on the RTOS, this may be time since boot, or time since the clock
    period was set, or some other time.  Its absolute value means
    nothing, but it is monotonically increasing and can be used to
    schedule future events, or to time the duration of some activity.
    Returns a 64 bit value.  The resolution of the returned value may
    be as good as one nano-second, or as poor as several microseconds.
    May be called from init/cleanup code, and from within realtime tasks.
    
    Experience has shown that the implementation of this function in
    some RTOS/Kernel combinations is horrible.  It can take up to 
    several microseconds, which is at least 100 times longer than it
    should, and perhaps a thousand times longer.  Use it only if you
    MUST have results in seconds instead of clocks, and use it sparingly.
    See rtapi_get_clocks() instead.

    Note that longlong math may be poorly supported on some platforms,
    especially in kernel space. Also note that rtapi_print() will NOT
    print longlongs.  Most time measurements are relative, and should
    be done like this:  deltat = (long int)(end_time - start_time);
    where end_time and start_time are longlong values returned from
    rtapi_get_time, and deltat is an ordinary long int (32 bits).
    This will work for times up to about 2 seconds.
*/
typedef long long int (*rtapi_get_time_t)(void);
#define rtapi_get_time()			\
    rtapi_switch->rtapi_get_time()
extern long long int _rtapi_get_time(void);

/** rtapi_get_clocks returns the current time in CPU clocks.  It is 
    fast, since it just reads the TSC in the CPU instead of calling a
    kernel or RTOS function.  Of course, times measured in CPU clocks
    are not as convenient, but for relative measurements this works
    fine.  Its absolute value means nothing, but it is monotonically
    increasing* and can be used to schedule future events, or to time
    the duration of some activity.  (* on SMP machines, the two TSC's
    may get out of sync, so if a task reads the TSC, gets swapped to
    the other CPU, and reads again, the value may decrease.  RTAPI
    tries to force all RT tasks to run on one CPU.)
    Returns a 64 bit value.  The resolution of the returned value is
    one CPU clock, which is usually a few nanoseconds to a fraction of
    a nanosecond.
    May be called from init/cleanup code, and from within realtime tasks.
    
    Note that longlong math may be poorly supported on some platforms,
    especially in kernel space. Also note that rtapi_print() will NOT
    print longlongs.  Most time measurements are relative, and should
    be done like this:  deltat = (long int)(end_time - start_time);
    where end_time and start_time are longlong values returned from
    rtapi_get_time, and deltat is an ordinary long int (32 bits).
    This will work for times up to a second or so, depending on the
    CPU clock frequency.  It is best used for millisecond and 
    microsecond scale measurements though.
*/
typedef long long int (*rtapi_get_clocks_t)(void);
#define rtapi_get_clocks()			\
    rtapi_switch->rtapi_get_clocks()
extern long long int _rtapi_get_clocks(void);


/***********************************************************************
*                     TASK RELATED FUNCTIONS                           *
************************************************************************/
/* implemented in rtapi_task.c */

/** NOTE: These realtime task related functions are only available in
    realtime modules.  User processes may not call them!
*/

/** NOTE: The RTAPI is designed to be a _simple_ API.  As such, it uses
    a very simple strategy to deal with SMP systems.  It ignores them!
    All tasks are scheduled on the first CPU.  That doesn't mean that
    additional CPUs are wasted, they will be used for non-realtime code.
*/

/** The 'rtapi_prio_xxxx()' functions provide a portable way to set
    task priority.  The mapping of actual priority to priority number
    depends on the RTOS.  Priorities range from 'rtapi_prio_lowest()'
    to 'rtapi_prio_highest()', inclusive. To use this API, use one of
    two methods:

    1) Set your lowest priority task to 'rtapi_prio_lowest()', and for
       each task of the next lowest priority, set their priorities to
       'rtapi_prio_next_higher(previous)'.

    2) Set your highest priority task to 'rtapi_prio_highest()', and
       for each task of the next highest priority, set their priorities
       to 'rtapi_prio_next_lower(previous)'.

    A high priority task will preempt a lower priority task.  The linux kernel
    and userspace are always a lower priority than all rtapi tasks.

    Call these functions only from within init/cleanup code, not from
    realtime tasks.
*/
typedef int (*rtapi_prio_highest_lowest_t)(void);
#define rtapi_prio_highest()			\
    rtapi_switch->rtapi_prio_highest()
extern int _rtapi_prio_highest(void);
#define rtapi_prio_lowest()			\
    rtapi_switch->rtapi_prio_lowest()
extern int _rtapi_prio_lowest(void);

typedef int (*rtapi_prio_next_higher_lower_t)(int);
#define rtapi_prio_next_higher(prio)		\
    rtapi_switch->rtapi_prio_next_higher(prio)
extern int _rtapi_prio_next_higher(int prio);
#define rtapi_prio_next_lower(prio)		\
    rtapi_switch->rtapi_prio_next_lower(prio)
extern int _rtapi_prio_next_lower(int prio);

#ifdef RTAPI

/** 'rtapi_task_new()' creates but does not start a realtime task.
    The task is created in the "paused" state.  To start it, call
    either rtapi_task_start() for periodic tasks, or rtapi_task_resume()
    for free-running tasks.
    On success, returns a positive integer task ID.  This ID is used
    for all subsequent calls that need to act on the task.  On failure,
    returns a negative error code as listed above.  'taskcode' is the
    name of a function taking one int and returning void, which contains
    the task code.  'arg' will be passed to 'taskcode' as an abitrary
    void pointer when the task is started, and can be used to pass
    any amount of data to the task (by pointing to a struct, or other
    such tricks).
    'prio' is the  priority, as determined by one of the priority
    functions above.  'owner' is the module ID of the module that
    is making the call (see rtapi_init).  'stacksize' is the amount
    of stack to be used for the task - be generous, hardware
    interrupts may use the same stack.  'uses_fp' is a flag that
    tells the OS whether the task uses floating point so it can
    save the FPU registers on a task switch.  Failing to save
    registers when needed causes the dreaded "NAN bug", so most
    tasks should set 'uses_fp' to RTAPI_USES_FP.  If a task
    definitely does not use floating point, setting 'uses_fp' to
    RTAPI_NO_FP saves a few microseconds per task switch.  Call
    only from within init/cleanup code, not from realtime tasks.
*/
#define RTAPI_NO_FP   0
#define RTAPI_USES_FP 1

typedef int (*rtapi_task_new_t)(void (*) (void *), void *,
				int, int, unsigned long int,
				int, char *, int);
#define rtapi_task_new(taskcode, arg, prio, owner, stacksize, uses_fp, \
		       name, cpu_id)				       \
    rtapi_switch->rtapi_task_new(taskcode, arg, prio, owner, stacksize, \
				uses_fp, name, cpu_id)
extern int _rtapi_task_new(void (*taskcode) (void *), void *arg,
			      int prio, int owner, unsigned long int stacksize, 
			      int uses_fp, char *name, int cpu_id);

/** 'rtapi_task_delete()' deletes a task.  'task_id' is a task ID
    from a previous call to rtapi_task_new().  It frees memory
    associated with 'task', and does any other cleanup needed.  If
    the task has been started, you should pause it before deleting
    it.  Returns a status code.  Call only from within init/cleanup
    code, not from realtime tasks.
*/
typedef int (*rtapi_task_delete_t)(int);
#define rtapi_task_delete(task_id)		\
    rtapi_switch->rtapi_task_delete(task_id)
extern int _rtapi_task_delete(int task_id);

/** 'rtapi_task_start()' starts a task in periodic mode.  'task_id' is
    a task ID from a call to rtapi_task_new().  The task must be in
    the "paused" state, or it will return -EINVAL.
    'period_nsec' is the task period in nanoseconds, which will be
    rounded to the nearest multiple of the global clock period.  A
    task period less than the clock period (including zero) will be
    set equal to the clock period.
    Call only from within init/cleanup code, not from realtime tasks.
*/
typedef int (*rtapi_task_start_t)(int, unsigned long int);
#define rtapi_task_start(task_id, period_nsec)	\
    rtapi_switch->rtapi_task_start(task_id, period_nsec)
extern int _rtapi_task_start(int task_id, unsigned long int period_nsec);

/** 'rtapi_wait()' suspends execution of the current task until the
    next period.  The task must be periodic, if not, the result is
    undefined.  The function will return at the beginning of the
    next period.  Call only from within a realtime task.
*/
typedef void (*rtapi_wait_t)(void);
#define rtapi_wait()				\
    rtapi_switch->rtapi_wait()
extern void _rtapi_wait(void);

/** 'rtapi_task_resume() starts a task in free-running mode. 'task_id'
    is a task ID from a call to rtapi_task_new().  The task must be in
    the "paused" state, or it will return -EINVAL.
    A free running task runs continuously until either:
    1) It is prempted by a higher priority task.  It will resume as
       soon as the higher priority task releases the CPU.
    2) It calls a blocking function, like rtapi_sem_take().  It will
       resume when the function unblocks.
    3) it is returned to the "paused" state by rtapi_task_pause().
    May be called from init/cleanup code, and from within realtime tasks.
*/
typedef int (*rtapi_task_resume_t)(int);
#define rtapi_task_resume(task_id)		\
    rtapi_switch->rtapi_task_resume(task_id)
extern int _rtapi_task_resume(int task_id);

/** 'rtapi_task_pause() causes 'task_id' to stop execution and change
    to the "paused" state.  'task_id' can be free-running or periodic.
    Note that rtapi_task_pause() may called from any task, or from init
    or cleanup code, not just from the task that is to be paused.
    The task will resume execution when either rtapi_task_resume() or
    rtapi_task_start() is called.  May be called from init/cleanup code,
    and from within realtime tasks.
*/
typedef int (*rtapi_task_pause_t)(int);
#define rtapi_task_pause(task_id)		\
    rtapi_switch->rtapi_task_pause(task_id)
extern int _rtapi_task_pause(int task_id);

/** 'rtapi_task_self()' returns the task ID of the current task.
    Call only from a realtime task.
*/
typedef int (*rtapi_task_self_t)(void);
#define rtapi_task_self()			\
    rtapi_switch->rtapi_task_self()
extern int _rtapi_task_self(void);

#endif /* RTAPI */

/** 'rtapi_backtrace()' writes a stack trace to the log.
 available in all flavors and ULAPI/RTAPI
*/
typedef void (*rtapi_backtrace_t)(int);
#define rtapi_backtrace(msglevel)			\
    rtapi_switch->rtapi_backtrace(msglevel)
extern void _rtapi_backtrace(int msglevel);

/***********************************************************************
*                  SHARED MEMORY RELATED FUNCTIONS                     *
************************************************************************/
/* implemented in rtapi_shmem.c */

/** 'rtapi_shmem_new()' allocates a block of shared memory.  'key'
    identifies the memory block, and must be non-zero.  All modules
    wishing to access the same memory must use the same key.
    'module_id' is the ID of the module that is making the call (see
    rtapi_init).  The block will be at least 'size' bytes, and may
    be rounded up.  Allocating many small blocks may be very wasteful.
    When a particular block is allocated for the first time, the first
    4 bytes are zeroed.  Subsequent allocations of the same block
    by other modules or processes will not touch the contents of the
    block.  Applications can use those bytes to see if they need to
    initialize the block, or if another module already did so.
    On success, it returns a positive integer ID, which is used for
    all subsequent calls dealing with the block.  On failure it
    returns a negative error code.  Call only from within user or
    init/cleanup code, not from realtime tasks.
*/
typedef int (*rtapi_shmem_new_t)(int, int, unsigned long int);
#define rtapi_shmem_new(key, module_id, size)	\
    rtapi_switch->rtapi_shmem_new(key, module_id, size)
extern int _rtapi_shmem_new(int key, int module_id,
			    unsigned long int size);

/** 'rtapi_shmem_new_inst()' does the same for a particular instance.
 **/

typedef int (*rtapi_shmem_new_inst_t)(int, int, int, unsigned long int);
#define rtapi_shmem_new_inst(key, instance, module_id, size)	\
    rtapi_switch->rtapi_shmem_new_inst(key, instance, module_id, size)
extern int _rtapi_shmem_new_inst(int key, int instance, int module_id,
			    unsigned long int size);

/** 'rtapi_shmem_delete()' frees the shared memory block associated
    with 'shmem_id'.  'module_id' is the ID of the calling module.
    Returns a status code.  Call only from within user or init/cleanup
    code, not from realtime tasks.
*/
typedef int (*rtapi_shmem_delete_t)(int, int);
#define rtapi_shmem_delete(shmem_id, module_id)		\
    rtapi_switch->rtapi_shmem_delete(shmem_id, module_id)
extern int _rtapi_shmem_delete(int shmem_id, int module_id);

typedef int (*rtapi_shmem_delete_inst_t)(int, int, int);
#define rtapi_shmem_delete_inst(shmem_id, instance, module_id)	\
    rtapi_switch->rtapi_shmem_delete_inst(shmem_id, instance, module_id)
extern int _rtapi_shmem_delete_inst(int shmem_id, int instance, int module_id);

/** 'rtapi_shmem_getptr()' sets '*ptr' to point to shared memory block
    associated with 'shmem_id'.  Returns a status code.  May be called
    from user code, init/cleanup code, or realtime tasks.
*/

typedef int (*rtapi_shmem_getptr_t)(int, void **);
#define rtapi_shmem_getptr(shmem_id, ptr)		\
    rtapi_switch->rtapi_shmem_getptr(shmem_id, ptr)
extern int _rtapi_shmem_getptr(int shmem_id, void **ptr);

typedef int (*rtapi_shmem_getptr_inst_t)(int, int, void **);
#define rtapi_shmem_getptr_inst(shmem_id, instance, ptr)	\
    rtapi_switch->rtapi_shmem_getptr_inst(shmem_id, instance, ptr)
extern int _rtapi_shmem_getptr_inst(int shmem_id, int instance, void **ptr);

/***********************************************************************
*                        Ringbuffer related functions                  *
************************************************************************/

typedef int (*rtapi_ring_new_t) (size_t, size_t, int, int);
#define rtapi_ring_new(size, sp_size, module_id, flags) \
    rtapi_switch->rtapi_ring_new(size, sp_size, module_id, flags)
extern int _rtapi_ring_new(size_t size, size_t sp_size, int module_id, int flags);

typedef int (*rtapi_ring_attach_t) (int,ringbuffer_t *, int);
#define rtapi_ring_attach(handle, ptr, module_id)			\
    rtapi_switch->rtapi_ring_attach(handle, ptr, module_id)
extern int _rtapi_ring_attach(int handle, ringbuffer_t *ptr, int module_id);

typedef int (*rtapi_ring_detach_t) (int,int);
#define rtapi_ring_detach(handle, module_id) \
    rtapi_switch->rtapi_ring_detach(handle, module_id)
extern int _rtapi_ring_detach(int handle, int module_id);

// rtapi_ring_refcount(ringheader_t *ptr) defined in rtapi_ring.h


/***********************************************************************
*                        Callback on RT scheduling violation           *
* rtapi detects when a scheduling release point has been missed, and   *
* several other fault situations, most of which are depend on the      *
* thread system used.                                                  *
*                                                                      *
* A use case would be a hal module which exports an rt estop pin       *
* this pin would be raised by the callback, eg rtmon.comp              *
************************************************************************/

// rtapi_exception_handler_t is defined in rtapi_exception.h
typedef rtapi_exception_handler_t (*rtapi_set_exception_t) (rtapi_exception_handler_t);
#define rtapi_set_exception(handler)	\
    rtapi_switch->rtapi_set_exception(handler)
extern rtapi_exception_handler_t  _rtapi_set_exception(rtapi_exception_handler_t h);


/***********************************************************************
*                        I/O RELATED FUNCTIONS                         *
************************************************************************/
// the rtapi_inb()/rtapi_outb()/rtapi_inw()/rtapi_outw() functions have
// moved to src/rtapi/rtapi_io.h, including documentation.

#include "rtapi_io.h"

#if (defined(RTAPI) && defined(BUILD_DRIVERS)) 
/** 'rtapi_request_region() reserves I/O memory starting at 'base',
    going for 'size' bytes, for component 'name'.

    Note that on kernels before 2.4.0, this function always succeeds.

    If the allocation fails, this function returns NULL.  Otherwise, it returns
    a non-NULL value.
*/
#  include <linux/version.h>
#  if !defined(BUILD_SYS_USER_DSO)
#    include <linux/module.h>
#    include <linux/ioport.h>
#  endif // BUILD_SYS_USER_DSO

    static __inline__ void *rtapi_request_region(unsigned long base,
            unsigned long size, const char *name) {
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0) && !defined(BUILD_SYS_USER_DSO)
        return (void*)request_region(base, size, name);
#  else
        return (void*)-1;
#  endif
    }

/** 'rtapi_release_region() releases I/O memory reserved by 
    'rtapi_request_region', starting at 'base' and going for 'size' bytes.
    'base' and 'size' must exactly match an earlier successful call to
    rtapi_request_region or the result is undefined.
*/
    static __inline__ void rtapi_release_region(unsigned long base,
            unsigned long int size) {
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0) &&  !defined(BUILD_SYS_USER_DSO)
        release_region(base, size);
#  endif
    }
#endif // RTAPI && BUILD_DRIVERS

/***********************************************************************
*                            RTAPI SWITCH                              *
************************************************************************/
/** rtapi_switch contains pointers to the _rtapi_* functions declared
    above.  The struct is initialized in rtapi_common.c.

    Each thread system needs a member in the thread_flavor_id_t enum,
    and should set the macro THREAD_FLAVOR_ID to that enumerator.
*/

// prototype for dummy rtapi placeholder function
typedef int (*rtapi_dummy_t)(void);

typedef struct {
    const char *git_version;
    const char *thread_flavor_name; // for messsages
    int  thread_flavor_id;
    // init & exit functions
    rtapi_init_t rtapi_init;
    rtapi_exit_t rtapi_exit;
    rtapi_next_module_id_t rtapi_next_module_id;
    // time functions
#ifdef RTAPI
    rtapi_clock_set_period_t rtapi_clock_set_period;
    rtapi_delay_t rtapi_delay;
    rtapi_delay_max_t rtapi_delay_max;
#else
    rtapi_dummy_t rtapi_clock_set_period;
    rtapi_dummy_t rtapi_delay;
    rtapi_dummy_t rtapi_delay_max;
#endif
    rtapi_get_time_t rtapi_get_time;
    rtapi_get_clocks_t rtapi_get_clocks;
    // task functions
    rtapi_prio_highest_lowest_t rtapi_prio_highest;
    rtapi_prio_highest_lowest_t rtapi_prio_lowest;
    rtapi_prio_next_higher_lower_t rtapi_prio_next_higher;
    rtapi_prio_next_higher_lower_t rtapi_prio_next_lower;
#ifdef RTAPI
    rtapi_task_new_t rtapi_task_new;
    rtapi_task_delete_t rtapi_task_delete;
    rtapi_task_start_t rtapi_task_start;
    rtapi_wait_t rtapi_wait;
    rtapi_task_resume_t rtapi_task_resume;
    rtapi_task_pause_t rtapi_task_pause;
    rtapi_task_self_t rtapi_task_self;

#else
    rtapi_dummy_t rtapi_task_new;
    rtapi_dummy_t rtapi_task_delete;
    rtapi_dummy_t rtapi_task_start;
    rtapi_dummy_t rtapi_wait;
    rtapi_dummy_t rtapi_task_resume;
    rtapi_dummy_t rtapi_task_pause;
    rtapi_dummy_t rtapi_task_self;
#endif
    // shared memory functions
    rtapi_shmem_new_t rtapi_shmem_new;
    rtapi_shmem_new_inst_t rtapi_shmem_new_inst;
    rtapi_shmem_delete_t rtapi_shmem_delete;
    rtapi_shmem_delete_inst_t rtapi_shmem_delete_inst;
    rtapi_shmem_getptr_t rtapi_shmem_getptr;
    rtapi_shmem_getptr_inst_t rtapi_shmem_getptr_inst;

    // ringbuffer functions
    // these will be removed once the new hal_lib.c ring
    // support code is in place, since rings are just
    // glorified HAL shared memory segments and do not
    // warrant an own rtapi data object per se
    rtapi_ring_new_t rtapi_ring_new;
    rtapi_ring_attach_t rtapi_ring_attach;
    rtapi_ring_detach_t rtapi_ring_detach;
#ifdef RTAPI
    rtapi_set_exception_t rtapi_set_exception;
#else
    rtapi_dummy_t rtapi_set_exception;
#endif
    rtapi_backtrace_t rtapi_backtrace;

} rtapi_switch_t;

// using code is responsible to define this:
// this extern is not used within RTAPI
extern rtapi_switch_t *rtapi_switch;

/** 'rtapi_get_handle()' returns a pointer to the rtapi_switch 
    structure, such that using code may refernce rtapi
    methods.
 */
typedef rtapi_switch_t *(*rtapi_get_handle_t)(void);
extern rtapi_switch_t *rtapi_get_handle(void);

// autorelease the rtapi mutex on scope exit
// declare a variable like so in the scope to be protected:
//
// foo_type foo __attribute__((cleanup(rtapi_autorelease_mutex)));
//
// make sure rtapi_mutex_get(&(rtapi_data->mutex));
// is unconditionally called first thing on scope entry
extern void rtapi_autorelease_mutex(void *variable);

// exported by instance.c (kstyles) and rtapi_main.c (userlandRT)
// configurable at rtapi.so module load time _only_
extern int rtapi_instance;

#ifdef ULAPI
// the ulapi constructor and destructor
// these attach/detach the rtapi shm segments to/from ULAPI
typedef int  (*ulapi_main_t)(int, int, global_data_t *);
typedef int (*ulapi_exit_t)(int);
extern int ulapi_main(int instance, int flavor, global_data_t *global);
extern int ulapi_exit(int instance);

// Check that a ulapi module is compatible with the running kernel
// from rtapi_compat.c
extern void ulapi_kernel_compat_check(rtapi_switch_t *rtapi_switch,
				      char *ulapi_lib);
#endif

/***********************************************************************
*                      MODULE PARAMETER MACROS                         *
************************************************************************/

#ifdef RTAPI

/* The API for module parameters has changed as the kernel evolved,
   and will probably change again.  We define our own macro for
   declaring parameters, so the code that uses RTAPI can ignore
   the issue.
*/

/** RTAPI_MP_INT() declares a single integer module parameter.
    RTAPI_MP_LONG() declares a single long module parameter.
    RTAPI_MP_STRING() declares a single string module parameter.
    RTAPI_MP_ARRAY_INT() declares an array of integer module parameters.
    RTAPI_MP_ARRAY_LONG() declares an array of long module parameters.
    RTAPI_MP_ARRAY_STRING() declares a single string module parameters.
    'var' is the name of the variable used for the parameter, which
    should be initialized with the default value(s) when it is declared.
    'descr' is a short description of the parameter.
    'num' is the number of elements in an array.
*/

#include <rtapi_export.h>

#if !defined(BUILD_SYS_USER_DSO)
#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#endif
#endif
#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif

#ifndef LINUX_VERSION_CODE
#define LINUX_VERSION_CODE 0
#endif

#if defined(BUILD_SYS_USER_DSO) || (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
#define RTAPI_STRINGIFY(x)    #x


#define RTAPI_MP_INT(var,descr)    \
  MODULE_PARM(var,"i");            \
  MODULE_PARM_DESC(var,descr);

#define RTAPI_MP_LONG(var,descr)   \
  MODULE_PARM(var,"l");            \
  MODULE_PARM_DESC(var,descr);

#define RTAPI_MP_STRING(var,descr) \
  MODULE_PARM(var,"s");            \
  MODULE_PARM_DESC(var,descr);

#define RTAPI_MP_ARRAY_INT(var,num,descr)          \
  MODULE_PARM(var,"1-" RTAPI_STRINGIFY(num) "i");  \
  MODULE_PARM_DESC(var,descr);

#define RTAPI_MP_ARRAY_LONG(var,num,descr)         \
  MODULE_PARM(var,"1-" RTAPI_STRINGIFY(num) "l");  \
  MODULE_PARM_DESC(var,descr);

#define RTAPI_MP_ARRAY_STRING(var,num,descr)       \
  MODULE_PARM(var,"1-" RTAPI_STRINGIFY(num) "s");  \
  MODULE_PARM_DESC(var,descr);

#else /* version 2.6 or later */

#include <linux/module.h>

#define RTAPI_MP_INT(var,descr)    \
  module_param(var, int, 0);       \
  MODULE_PARM_DESC(var,descr);

#define RTAPI_MP_LONG(var,descr)   \
  module_param(var, long, 0);      \
  MODULE_PARM_DESC(var,descr);

#define RTAPI_MP_STRING(var,descr) \
  module_param(var, charp, 0);     \
  MODULE_PARM_DESC(var,descr);

#define RTAPI_MP_ARRAY_INT(var,num,descr)                \
  int __dummy_##var;                                     \
  module_param_array(var, int, &(__dummy_##var), 0);     \
  MODULE_PARM_DESC(var,descr);

#define RTAPI_MP_ARRAY_LONG(var,num,descr)               \
  int __dummy_##var;                                     \
  module_param_array(var, long, &(__dummy_##var), 0);    \
  MODULE_PARM_DESC(var,descr);

#define RTAPI_MP_ARRAY_STRING(var,num,descr)             \
  int __dummy_##var;                                     \
  module_param_array(var, charp, &(__dummy_##var), 0);  \
  MODULE_PARM_DESC(var,descr);

#endif /* version < 2.6 */

#if !defined(BUILD_SYS_USER_DSO)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#define MODULE_LICENSE(license)         \
static const char __module_license[] __attribute__((section(".modinfo"))) =   \
"license=" license
#endif
#endif

#endif /* RTAPI */


RTAPI_END_DECLS

#endif /* RTAPI_H */
