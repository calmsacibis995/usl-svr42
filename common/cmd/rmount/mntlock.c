/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
	lock   -- create a file to hold a lock for rmnttab operations.
	unlock -- release the lock; the file that held the lock remains

	Note:	Despite the name, ``mntlock,'' we don't lock mnttab.
*/

#ident	"@(#)rmount:mntlock.c	1.1.10.2" /* really should be rmntlock.c */
#ident  "$Header: mntlock.c 1.2 91/06/27 $"
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "rmount.h"

extern int	errno;
extern char	*cmd;

static int	lfd = -1;

void
lock()
{
	static char *msg_c = "%s: warning: cannot create semaphore file %s\n";
	static char *msg_l = "%s: warning: cannot lock   semaphore file %s\n";

	while ((lfd = creat(RSEM_FILE, (S_IRUSR|S_IWUSR))) < 0
	&&	errno == EAGAIN)
		(void)sleep(1);
	if (lfd < 0)
		Fprintf(stderr, msg_c, cmd, RSEM_FILE);
	if (lockf(lfd, F_LOCK, 0L) < 0)
		Fprintf(stderr, msg_l, cmd, RSEM_FILE);
}

void
unlock()
{
	if (lfd >= 0)
		Close(lfd);
}
