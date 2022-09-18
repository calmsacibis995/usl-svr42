/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/parse_ex.c	1.2.2.2"
#ident "@(#)parse_ex.c	1.2 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	parse_execarg - parse string into argument vector

    SYNOPSIS
	int parse_execarg(char *p, int i)

    DESCRIPTION
	Parse up string into an argument vector. Assume that
	whitespace delimits arguments. Any non-escaped double
	quotes will be used to group multiple whitespace-delimited
	tokens into a single argument.
*/

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

int parse_execarg(p, i, pargcnt, pargvec, chunksize, _argvec)
register char	*p;
register int	i;
int		*pargcnt;
char		***pargvec;
int		chunksize;
char		**_argvec;
{
	register char	*q;
	register int	stop;
	register int	ignorespace = FALSE;

	p = (char*)skipspace(p);
	while (*p) {
		q = p;
		stop = FALSE;
		while (*q && (stop == FALSE)) {
		    again:
			switch (*q) {
			case '\\':
				/* Slide command string 1 char to left */
				strmove (q, q+1);
				break;
			case '"':
				ignorespace = ((ignorespace == TRUE) ?
								FALSE : TRUE);
				/* Slide command string 1 char to left */
				strmove (q, q+1);
				goto again;
			default:
				if (isspace(*q) && (ignorespace == FALSE)) {
					stop = TRUE;
					continue;
				}
				break;
			}
			q++;
		}
		if (*q == '\0') {
			if ((i < *pargcnt) || expand_argvec(pargvec, chunksize, _argvec, pargcnt))
				(*pargvec)[i++] = p;
			break;
		}
		*q++ = '\0';
		if ((i < *pargcnt) || expand_argvec(pargvec, chunksize, _argvec, pargcnt))
			(*pargvec)[i++] = p;
		p = (char*)skipspace(q);
	}

	return i;
}
