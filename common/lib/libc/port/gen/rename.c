/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/rename.c	1.12"
/*LINTLIBRARY*/
#include "synonyms.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int
remove(filename)
const char *filename;
{
	struct stat	statb;

	/* If filename is not a directory, call unlink(filename)
	 * Otherwise, call rmdir(filename) */

	if (lstat(filename, &statb) != 0)
		return(-1);
	if ((statb.st_mode & S_IFMT) != S_IFDIR)
		return(unlink(filename));
	return(rmdir(filename));
}

int
rename(old, new)
const char *old;
const char *new;
{
	return(_rename(old, new));
}
