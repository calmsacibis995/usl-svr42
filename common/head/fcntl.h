/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FCNTL_H
#define _FCNTL_H

#ident	"@(#)sgs-head:fcntl.h	1.6.5.1"

#include <sys/types.h>
#include <sys/fcntl.h>

#if defined(__STDC__)

extern int fcntl(int, int, ...);
extern int open(const char *, int, ...);
extern int creat(const char *, mode_t);

#else

extern int fcntl();
extern int open();
extern int creat();

#endif

#endif /* _FCNTL_H */
