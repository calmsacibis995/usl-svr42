/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PW_H
#define _PW_H

#ident	"@(#)sgs-head:pw.h	1.3"

#if defined(__STDC__)

extern char *logname(void);
extern char *regcmp(const char *, ... );
extern char *regex(const char *, const char *, ...);
#else
extern char *logname();
extern char *regcmp();
extern char *regex();

#endif

#endif /* _PW_H */
