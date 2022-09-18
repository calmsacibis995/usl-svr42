/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
	rd_rmnttab	-	not ``read,'' rather
				get ``ready'' to use the rmnttab table

	return:
		0	-	stat and open success
		1	-	rmnttab does not exist
		2	-	stat or open error
*/

#ident	"@(#)rmount:rd_rmnttab.c	1.1.9.2"
#ident  "$Header: rd_rmnttab.c 1.2 91/06/27 $"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include "rmount.h"
#include "rmnttab.h"

extern char *cmd;

int
rd_rmnttab(rpp, stbufp)
	FILE		**rpp;		/* file pointer for reading rmntab */
	struct stat	*stbufp;	/* status of rmnttab */
{

	if (stat(RMNTTAB, stbufp) < 0) {
		if (errno == ENOENT)	 	/* rmnttab does not exist */
			return 1;		/* pretend it's empty */
		else {
			Fprintf(stderr, "%s: cannot stat %s\n", cmd, RMNTTAB);
			return 2;
		}
	}
	else if ((*rpp = fopen(RMNTTAB, "r")) == NULL) {
		Fprintf(stderr, "%s: cannot open %s\n", cmd, RMNTTAB);
		return 2;
	}
	return 0;
}
