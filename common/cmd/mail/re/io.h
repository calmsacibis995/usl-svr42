/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/re/io.h	1.1.2.2"
#ident "@(#)io.h	1.2 'attmail mail(1) command'"
#ifndef	EPR

#ifdef	USE_STDIO
#define		PR	printf(
#define		EPR	fprintf(stderr,
#define		SPR	sprintf(
#define		WR(b,n)	fwrite(b, 1, n, stdout)
#define		FLUSH	fflush(stdout)
#else
#include <fio.h>
#if defined(__STDC__) || defined(c_plusplus) || defined(__cplusplus)
extern int fprint(int, char*, ...);
extern int sprint(char*, char*, ...);
#else
extern int fprint();
extern int sprint();
#endif

#define		PR	fprint(1,
#define		EPR	fprint(2,
#define		SPR	sprint(
#define		WR(b,n)	write(1, b, (long)(n))
#define		FLUSH	Fflush(1)
#endif

#endif
