/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:svc/svc.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>
#include <sys/callo.h>
#include <sys/resource.h>

struct callo	callout[NCALL];

struct rlimit	rlimits[] = {
		SCPULIM,
		HCPULIM,
		SFSZLIM,
		HFSZLIM,
		SDATLIM,
		HDATLIM,
		SSTKLIM,
		HSTKLIM,
		SCORLIM,
		HCORLIM,
		SFNOLIM,
		HFNOLIM,
		SVMMLIM,
		HVMMLIM
};

/* Time variables for XENIX-style ftime() system call. */
int	Timezone = TIMEZONE;	/* tuneable time zone for XENIX ftime() */
int	Dstflag = DSTFLAG;	/* tuneable daylight time flag for XENIX */
