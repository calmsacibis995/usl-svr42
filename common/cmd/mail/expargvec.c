/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/expargvec.c	1.1.2.2"
#ident "@(#)expargvec.c	1.1 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	expand_argvec - expand an argument vector

    SYNOPSIS
	int expand_argvec(char ***pargvec, int chunksize, char **_argvec, int *pargcnt)

    DESCRIPTION
	expand_argvec() is called when a static argument vector
	is about to be overrun. It (re)allocates space with
	malloc() and realloc() to replace the static buffer, bumping up
	the size of the array by chunksize.
*/

int expand_argvec(pargvec, chunksize, _argvec, pargcnt)
char ***pargvec;
int chunksize;
char **_argvec;
int *pargcnt;
{
    char **svargvec = *pargvec;
    char **mem;
    *pargcnt += chunksize;
    if (*pargvec == _argvec)
	{
	if ((mem = (char**) malloc(*pargcnt * sizeof(char**))) != (char**)NULL)
	    memcpy((char*)mem, (char*)svargvec, (*pargcnt - chunksize) * (int)sizeof(char**));
	}

    else
	mem = (char**) realloc((char*)*pargvec, *pargcnt * sizeof(char**));

    if (mem)
        {
	*pargvec = mem;
	return 1;
	}

    else
	{
	*pargcnt -= chunksize;
	return 0;
	}
}
