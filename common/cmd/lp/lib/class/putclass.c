/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/class/putclass.c	1.8.2.3"
#ident	"$Header: $"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"
#include "errno.h"
#include "sys/types.h"
#include "string.h"

#include "lp.h"
#include "class.h"
#include "printers.h"

/**
 ** putclass() - WRITE CLASS OUT TO DISK
 **/

int
#if	defined(__STDC__)
putclass (
	char *			name,
	CLASS *			clsbufp
)
#else
putclass (name, clsbufp)
	char			*name;
	CLASS			*clsbufp;
#endif
{
	char			*file;

	FILE			*fp;

	level_t			lid;

	int			n;

	if (!name || !*name) {
		errno = EINVAL;
		return -1;
	}

	if (STREQU(NAME_ALL, name)) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * Open the class file and write out the class members.
	 */

	if (!(file = getclassfile(name)))
		return -1;

	if (!(fp = open_lpfile(file, "w", MODE_READ))) {
		Free (file);
		return -1;
	}

	printlist (fp, clsbufp->members);

	if (ferror(fp)) {
		close_lpfile (fp);
		lid = PR_SYS_PUBLIC;
		while ((n=lvlfile (file, MAC_SET, &lid)) < 0 && errno == EINTR)
		    continue;
		Free (file);
		return -1;
	}
	close_lpfile (fp);
	lid = PR_SYS_PUBLIC;
	while ((n=lvlfile (file, MAC_SET, &lid)) < 0 && errno == EINTR)
	    continue;

	Free (file);
	if (n < 0 && errno != ENOSYS)
	    return -1;

	return 0;
}
