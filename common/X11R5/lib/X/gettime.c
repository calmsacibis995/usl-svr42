/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:gettime.c	1.1"
#ifndef	NOIDENT
#ident	"@(#)t4Xlib:gettime.c	1.4"
#endif

#ifdef SHARELIB
#include <libXi.h>
#define time	(*_libX_time)
#define times	(*_libX_times)
#endif

/*
 gettimeofday.c (C source file)
	Acc: 575557265 Mon Mar 28 08:01:05 1988
	Mod: 575557356 Mon Mar 28 08:02:36 1988
	Sta: 575557356 Mon Mar 28 08:02:36 1988
	Owner: 2011
	Group: 1985
	Permissions: 664
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
#if defined(SYSV) && ! defined(SVR4_0)
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <Xos.h>
#include <sys/times.h>

extern long time();
extern long times();

/*
 * Timezone argument no longer accepted ... single arg in SVR4.0 libc
struct Timezone {
        int     tz_minuteswest;
        int     tz_dsttime;
};
 */

gettimeofday(tp)
	struct timeval * tp;
{
	static long basetime=0;
	static long basetime_hz;

	struct tms times_buf;
 	register long ticks = times(&times_buf);
	register unsigned long tmp;

	if (tp == NULL) {
   		errno = EFAULT;
   		return -1;
   	}

	if ((basetime == 0) || (ticks <= basetime_hz)) {
		/* either wrap around or first time */
		struct tms	times_buf;
	
		if (time((long *) &basetime) == -1) {
			basetime = 0;
      			errno = EFAULT;
      			return -1;
		}
		
		basetime_hz = ticks;
	}

	tmp = ticks - basetime_hz;
	tp->tv_sec  = basetime + tmp / HZ;
	tp->tv_usec = (tmp % HZ) * (1000000 / HZ);

	/*
      	if (tzp != NULL) {
 		tzp-> tz_minuteswest = timezone / 60;
 		tzp-> tz_dsttime = daylight;
 	}
	*/
	return 0;
} /* end of gettimeofday */
#endif /* SYSV && ! SVR4_0 */
