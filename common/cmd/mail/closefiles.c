/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/closefiles.c	1.3.2.3"
#ident "@(#)closefiles.c	1.4 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	closeallfiles - close all open files

    SYNOPSIS
	void closeallfiles(int firstfd)

    DESCRIPTION
	closeallfiles() closes all open file descriptors
	starting with "firstfd".
*/

/* _NFILE used to be defined in stdio.h */
#ifdef SVR4
# undef _NFILE
# define _NFILE	ulimit(UL_GDESLIM)
#else
# ifdef USE_GETDTABLESIZE
#  undef _NFILE
#  define _NFILE getdtablesize()
# endif
#endif

void closeallfiles(firstfd)
int firstfd;
{
    register int i;
    register int nfile = _NFILE;

    for (i = firstfd; i < nfile; i++)
	close(i);
}
