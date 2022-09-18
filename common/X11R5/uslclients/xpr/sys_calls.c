#ident	"@(#)xpr:sys_calls.c	1.4"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"

#include "xpr.h"

#ifndef MEMUTIL
extern char		*malloc(),
			*calloc();
#endif /* MEMUTIL */

extern int		read();

/**
 ** Malloc()
 **/
char			*Malloc (size)
	unsigned int		size;
{
	register char		*ret = malloc(size);


	if (!ret) {
		fprintf (
			stderr,
			"xpr: Could not allocate %d bytes.\n",
			size
		);
		exit (1);
	}
	return (ret);
}

/**
 ** Calloc()
 **/

char			*Calloc (nelem, elsize)
	unsigned int		nelem,
				elsize;
{
	register char		*ret = calloc(nelem, elsize);


	if (!ret) {
		fprintf (
			stderr,
			"xpr: Could not allocate %d bytes.\n",
			nelem * elsize
		);
		exit (1);
	}
	return (ret);
}

/**
 ** Read()
 **/

void			Read (fd, buf, nbytes)
	int			fd;
	char			*buf;
	unsigned int		nbytes;
{
	int			n;
	unsigned int		nread	= 0;


	while (nread != nbytes) {
		switch ((n = read(fd, buf + nread, nbytes - nread))) {

		case -1:
			fprintf (
				stderr,
				"xpr: Error reading input (%s)\n.",
				PERROR(errno)
			);
			exit (1);
			/*NOTREACHED*/

		case 0:
			fprintf (
				stderr,
				"xpr: Premature EOF in input.\n"
			);
			exit (1);
			/*NOTREACHED*/

		default:
			nread += n;
			break;
		}
	}
	return;
}
