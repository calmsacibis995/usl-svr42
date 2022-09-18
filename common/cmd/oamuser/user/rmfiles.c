/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/rmfiles.c	1.6.11.2"
#ident  "$Header: rmfiles.c 2.0 91/07/13 $"

/*
 * Procedure:	rm_files
 *
 * Restrictions:
 *		execl():	P_MACREAD
 *
 * Notes:	This routine forks and execs the "/usr/bin/rm" command
 *		with the "-rf" option to remove ALL files in the named
 *		directory.
*/

#include <sys/types.h>
#include <stdio.h>
#include <userdefs.h>
#include <errno.h>
#include <priv.h>
#include "messages.h"

#define 	SBUFSZ	256

extern void errmsg();
extern int rmdir();
extern	long	wait();

int
rm_files(homedir)
	char *homedir;			/* home directory to remove */
{
	char	*cmd = "/usr/bin/rm",
		*options = "-rf";

	int	status;
	pid_t	pid;

	/* delete all files belonging to owner */

	if ((pid = fork()) == 0) {
		/*
		 * in the child
		*/
		(void) procprivl(CLRPRV, MACREAD_W, 0);
		(void) execl(cmd, cmd, options, homedir, (char *)NULL);
		(void) procprivl(SETPRV, MACREAD_W, 0);
		exit(1);
	}

	/*
	 * the parent sits quietly and waits for the child to terminate.
	*/
	(void) wait(&status);

	if (((status >> 8) & 0377) != 0) {
		errmsg(M_RMFILES);
		return EX_HOMEDIR;
	}
	return EX_SUCCESS;
}
