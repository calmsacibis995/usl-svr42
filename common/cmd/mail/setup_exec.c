/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/setup_exec.c	1.9.2.2"
#ident "@(#)setup_exec.c	2.11 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	setup_exec - set up an argument vector

    SYNOPSIS
	char **setup_exec(char *command)

    DESCRIPTION
	Parse the command into a list of arguments appropriate
	for use by exec().
*/

#define MAXARGS 512
#define CHUNKSIZE 128
static char	*_argvec[MAXARGS]; /* enough to begin with */
static int	argcnt = MAXARGS-1;
static char	**argvec = &_argvec[0];

char **setup_exec(s)
char *s;
{
    register int i = parse_execarg(s, 0, &argcnt, &argvec, CHUNKSIZE, _argvec);
    argvec[i] = (char *)NULL;
    return (i == 0) ? (char**)NULL : argvec;
}
