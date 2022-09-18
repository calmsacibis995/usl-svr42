/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:stat.c	1.6"
#endif

#define STAT_C

#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


time_t
StatFile(filename, inode, size)
char *		filename;
o_ino_t	*	inode;
off_t *		size;
{
	struct stat status;
	
	if (size != (off_t *)0) {
		*size = (off_t)0;
	}
	if (inode != (o_ino_t *)0) {
		*inode = -1;
	}
	if (stat (filename, &status) == -1) {
		return (time_t)0;
	}
	if (inode != (o_ino_t *)0) {
		*inode = status.st_ino;
		if (size != (off_t *)0) {
			*size = status.st_size;
		}
	}
	return status.st_mtime;
}

mode_t
GetUmask ()
{
	mode_t	mask;

	mask = umask (0);
	(void)umask (mask);
	mask = ~mask & (S_IRWXU | S_IRWXG | S_IRWXO);

	return mask;
}
