/*
 * Author: Xun Luo, xluo@cs.uic.edu
 * $Date: 2008/06/12 16:12:35 $
 * $Revision: 1.1 $
 * 
 * This work is open source under Internet2 license.
 * Read COPYING file for copyright information.
 */

/* 
 * This header file contain all the API definitions of tsc-i2 library.
 */

#ifndef _TSC_I2_H
#define _TSC_I2_H

/* this part must be put before all other headers */
#if ( defined(TSCI2_OS_WIN32) )
#include <winsock.h>  /* This does: #include <windows.h> */
#else
#include "tsci2_config.h"
#endif  /* TSCI2_OS_WIN32 */

#include "int64.h"

#include <fcntl.h>
#include <sys/types.h>

#if ( defined(TSCI2_OS_WIN32) )
#include <process.h>
#else
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#endif /* TSCI2_OS_WIN32 */

#if ( defined(TSCI2_OS_WIN32) )
/*
 * simulation functions for unistd
 */
#define EPOCHFILETIME (116444736000000000)
int gettimeofday(struct timeval *tv, struct timezone *tz);
unsigned int sleep (unsigned seconds);
int usleep (unsigned long microseconds);
#endif /* TSCI2_OS_WIN32 */


/****************************************************************
 * CONSTANTS
 ***************************************************************/

/* methods to get time */
#define TSCI2_ERROR -1    /* failure */
#define TSCI2_DAEMON 1    /* client get served by daemon */
#define TSCI2_CLIENT 2    /* client do the calibration by self */
#define TSCI2_FALLBACK 4  /* use system default gettimeofday() */

/* maximum offset: microseconds in 50 years. */
#define TSCI2_OFFSET_MAX (int64_t) 15768e11;

/****************************************************************
 * STRUCTURES
 ***************************************************************/

#ifdef TSCI2_DEBUG
/* daemon statistics */
typedef struct {
    unsigned int poll_factor;
    double jitter;
    double stability;
    int64_t daemon_offset;
    double daemon_delta_freq;
} daemon_stats;
#endif  /* TSCI2_DEBUG */

typedef struct _tsci2_context_t *tsci2_context;

/****************************************************************
 * FUNCTIONS
 ***************************************************************/
/*
 * Initialize the library.  Connect to the daemon or fall back.  The
 * return value indicates the mode of operation: with a daemon, with
 * gettimeofday(), or with a conversion table trained only within the
 * process.
 *   @param methods The flag of user prefered methods.
 */
int
tsci2_init(int methods);

/*
 * Shutdown the library. If the library is running in DAEMON mode, unlink
 * the shared memory.
 */
int
tsci2_shutdown();

/* 
 * Create a new tsci2 context. The name of this function, together with
 * the following tsci2_free_context(), are borrowed from malloc(3) and 
 * free(3). Note tsci2_context is a pointer, so the usage of these functions
 * are similar to malloc and free.
 * Returns NULL when allocation fails, otherwise the pointer is returned.
 * tsci2_free_context has no return value.
 */
tsci2_context
tsci2_alloc_context();

void 
tsci2_free_context(tsci2_context context);

/*
 * Retreive the current internal context (the conversion table) of the
 * library.  Memory pointed to by context must be preallocated.  Most
 * applications never call this function, and instead rely on the
 * implicit default current context.
 */
int
tsci2_get_default_context(tsci2_context context);

/*
 *  time reporting utility
 *   @param tp Pointer to reported time.
 *   @param context The context used for conversion.
 */
int
tsci2_gettimeofday(struct timeval *tp, tsci2_context context);

/*
 * direct access to the tics reading of TSC
 */
uint64_t
tsci2_getticks();

/* converstion utilities */
int
tsci2_ticks2timeval(uint64_t ticks,
                    struct timeval *tv,
                    tsci2_context context);

#ifdef TSCI2_DEBUG
/*
 * 'backdoor' function to peek at daemon statistics
 */
int
tsci2_peek_daemon_stats(daemon_stats *d_stat, tsci2_context context);
#endif  /* TSCI2_DEBUG */

#endif /* _TSC_I2_H */
