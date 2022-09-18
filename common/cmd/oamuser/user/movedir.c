/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/movedir.c	1.4.11.2"
#ident  "$Header: movedir.c 2.0 91/07/13 $"

#include <sys/types.h>
#include <stdio.h>
#include <userdefs.h>
#include "messages.h"

extern	int	access(),
		rename();

extern	void	errmsg();

/*
	Move directory contents from one place to another
*/
int
move_dir(from, to)
	char *from;			/* directory to move files from */
	char *to;			/* dirctory to move files to */
{
	register rc = EX_SUCCESS;
	
	if (access(from, 0) == 0) {	/* home dir exists */
		if (access(to, 0) == 0) {	/* "new" directory exists */
			errmsg(M_NOSPACE, from, to);
			return EX_NOSPACE;
		}
		/*
		 * rename the "from" directory to the "to" directory
		*/
		(void) rename(from, to);
	}
	return rc;
}
