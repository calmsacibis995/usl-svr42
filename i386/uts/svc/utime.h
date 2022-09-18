/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_UTIME_H	/* wrapper symbol for kernel use */
#define _SVC_UTIME_H	/* subject to change without notice */

#ident	"@(#)uts-x86:svc/utime.h	1.4"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#else

#include <sys/types.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/* utimbuf is used by utime(2) */

struct utimbuf {
	time_t actime;		/* access time */
	time_t modtime;		/* modification time */
};

#endif	/* _SVC_UTIME_H */
