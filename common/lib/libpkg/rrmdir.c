/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/rrmdir.c	1.4.5.3"
#ident "$Header: $"
#include <limits.h>

extern int	sprintf(),
		system();

int
rrmdir(path)
char *path;
{
	char cmd[PATH_MAX+13];

	(void) sprintf(cmd, "/usr/bin/rm -rf %s", path);
	return(system(cmd) ? 1 : 0);
}
