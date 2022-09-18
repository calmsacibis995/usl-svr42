/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/doopen.c	1.8.2.2"
#ident "@(#)doopen.c	2.10 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	doopen - Generic open routine.

    SYNOPSIS
	FILE *doopen(char *file, char *type, int errnum)

    DESCRIPTION
	doopen() opens the given file and
	returns the file pointer on success.
	On an error, it exits with the passed error value.

	Note: This should be used only for critical files
	as it will terminate mail(1) on failure.
*/

FILE *doopen(file, type, errnum)
char	*type;
char	*file;
int	errnum;
{
	static const char pn[] = "doopen";
	FILE *fptr;

	if ((fptr = fopen(file, type)) == NULL) {
		pfmt(stderr, MM_ERROR,
			":47:Cannot open %s, type %s: %s\n", 
			file, type, strerror(errno));
		error = errnum;
		Dout(pn, 0, "can't open '%s' type: %s\n",file,type);
		Dout(pn, 0, "error set to %d\n", error);
		done(0);
	}
	return(fptr);
}
