/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _STROPTS_H
#define _STROPTS_H

#ident	"@(#)sgs-head:stropts.h	1.8"
/*
 * Streams user options definitions.
 */

#ifndef _SYS_STROPTS_H
#include <sys/stropts.h>
#endif

#if defined(__STDC__)

extern int getmsg(int, struct strbuf *, struct strbuf *, int *);
extern int putmsg(int, const struct strbuf *, const struct strbuf *, int);

extern int getpmsg(int, struct strbuf *, struct strbuf *, int *, int *);
extern int putpmsg(int, const struct strbuf *, const struct strbuf *, int, int);

#else

extern int getmsg();
extern int putmsg();

extern int getpmsg();
extern int putpmsg();

#endif

#endif /* _STROPTS_H */
