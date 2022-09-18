/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_CALLO_H	/* wrapper symbol for kernel use */
#define _SVC_CALLO_H	/* subject to change without notice */

#ident	"@(#)uts-x86:svc/callo.h	1.3"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *	The callout structure is for a routine arranging
 *	to be called by the clock interrupt
 *	(clock.c) with a specified argument,
 *	in a specified amount of time.
 *	Used, for example, to time tab delays on typewriters.
 */

struct	callo
{
	int	c_time;		/* incremental time */
	int	c_id;		/* timeout id */
	caddr_t	c_arg;		/* argument to routine */
	void	(*c_func)();	/* routine */
};
extern  struct  callo   callout[];

#endif	/* _SVC_CALLO_H */
