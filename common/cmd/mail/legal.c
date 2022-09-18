/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/legal.c	1.6.2.2"
#ident "@(#)legal.c	2.6 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	legal - check existence of file

    SYNOPSIS
	int legal(char *file)

    DESCRIPTION
	legal() checks to see if "file" is a writable file name.

	Returns:
		0	-> file or directory exists, but is unwriteable
		1	-> file exists writeable
		2	-> file does not exist, but can be created
*/

int legal(file)
register char *file;
{
	register char *sp;
	char dfile[MAXFILENAME];

	/*
		If file does not exist then try "." if file name has
		no "/". For file names that have a "/", try a check
		for existence of the parent directory.
	*/
	if (access(file, F_OK) == CSUCCESS) {
		if (access(file, W_OK) == CSUCCESS) return(1);
		else return(0);
	} else {
		if ((sp=strrchr(file, '/')) == NULL) {
			strcpy(dfile, ".");
		} else if (sp == file) {
			strcpy(dfile, "/");
		} else {
			strncpy(dfile, file, sp - file);
			dfile[sp - file] = '\0';
		}
		if (access(dfile, W_OK) == CERROR)
			return(0);
		return(2);
	}
}
