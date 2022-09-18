/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)siserver:os/setitimer.c	1.1"
#endif

#include <sys/time.h>

#ifndef i386	/* funNotUsedByATT, setitimer */

setitimer(flag,time,otime)
int flag;
struct itimerval time,otime;
{
    /* pretty bogus, huh? */
    alarm(time.it_interval.tv_sec);
}

#endif	/* i386, funNotUsedByATT */
