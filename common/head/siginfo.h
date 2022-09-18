/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SIGINFO_H
#define _SIGINFO_H

#ident	"@(#)sgs-head:siginfo.h	1.4"

#include <sys/types.h>
#include <sys/siginfo.h>

struct siginfolist {
	int nsiginfo;
	char **vsiginfo;
};

extern char * _sys_illlist[];
extern char * _sys_fpelist[];
extern char * _sys_segvlist[];
extern char * _sys_buslist[];
extern char * _sys_traplist[];
extern char * _sys_cldlist[];
extern struct siginfolist _sys_siginfolist[];

#if defined(__STDC__)

extern void psiginfo(siginfo_t *, const char *);
extern void psignal(int, const char *);

#else

extern void psiginfo();
extern void psignal();

#endif

#endif /* _SIGINFO_H */
