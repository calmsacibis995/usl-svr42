/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/finduid.c	1.2.14.2"
#ident  "$Header: finduid.c 2.0 91/07/13 $"
/*
 * Command:	finduid
 *
 * Usage:	finduid
 *
 * Level:	SYS_PRIVATE
 *
 * Inheritable Privileges:	P_DACREAD
 *	 Fixed Privileges:	none
 *
 * Notes:	prints the next available uid
*/

/* LINTLIBRARY */
#include	<sys/types.h>
#include	<stdio.h>
#include	<userdefs.h>
#include	<mac.h>
#include	<pwd.h>
#include	<errno.h>
#include	<unistd.h>
#include	<locale.h>
#include	<pfmt.h>
#include	"uidage.h"

extern	int	errno;
extern	void	exit();
extern	uid_t	findnextuid();
extern	char	*strerror();

/* return the next available uid */
main()
{
	uid_t	uid;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxsysadm");
	(void) setlabel("UX:finduid");

	errno = 0;

	if (access(UIDAGEF, R_OK) != 0) {
		if (errno != ENOENT) {
			(void) pfmt(stderr, MM_ERROR | MM_NOGET, "%s\n",
				strerror(errno));
			exit(EX_FAILURE);
		}
	}

	uid = findnextuid();

	if (uid == -1)
		exit(EX_FAILURE);

	(void) fprintf(stdout, "%ld\n", uid);

	exit(EX_SUCCESS);

	/*NOTREACHED*/
}
