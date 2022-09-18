/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_CLOCK_H	/* wrapper symbol for kernel use */
#define _SVC_CLOCK_H	/* subject to change without notice */

#ident	"@(#)uts-x86:svc/clock.h	1.2"
#ident	"$Header: $"
 
#define SECHR	(60*60)		/* seconds/hr */
#define SECDAY	(24*SECHR)	/* seconds/day */
#define SECYR	(365*SECDAY)	/* seconds/common year */

#endif	/* _SVC_CLOCK_H */
