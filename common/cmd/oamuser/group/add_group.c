/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:group/add_group.c	1.3.15.2"
#ident  "$Header: add_group.c 2.0 91/07/13 $"

#include	<sys/types.h>
#include	<stdio.h>
#include	<unistd.h>
#include	<userdefs.h>
#include	<priv.h>
#include	<pfmt.h>

/*
 * Procedure:	add_group
 *
 * Restrictions:
 *		fopen:		none
 *		lckpwdf:	none
 *		ulckpwdf:	none
 *		fclose:		none
*/

extern	int	lckpwdf(),
		ulckpwdf();

int
add_group(group, gid)
	char	*group;	/* name of group to add */
	gid_t	gid;		/* gid of group to add */
{
	FILE	*etcgrp;		/* /etc/group file */

	/*
	 * lockout anyone trying to write the group file.  This
	 * is done by calling ``lckpwdf()'' which sets a lock
	 * on the file ``/etc/security/ia/.pwd.lock''.
	*/
	if (lckpwdf() != 0) {
		pfmt(stderr, MM_ERROR, ":0:Group file busy.  Try again later\n");
		return EX_UPDATE;
	}
	if ((etcgrp = fopen(GROUP, "a")) == NULL) {
		(void) ulckpwdf();
		return EX_UPDATE;
	}

	(void) fprintf(etcgrp, "%s::%ld:\n", group, gid);

	(void) fclose(etcgrp);

	(void) ulckpwdf();

	return EX_SUCCESS;
}
