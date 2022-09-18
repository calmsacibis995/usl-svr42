/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/createmf.c	1.8.2.3"
#ident "@(#)createmf.c	2.9 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	createmf - create the mail file

    SYNOPSIS
	void createmf(uid_t uid, char *file)

    DESCRIPTION  
	If the mail file does not exist, create it with the
	correct uid and gid.
*/

void createmf(uid, file)
uid_t uid;
char *file;
{
	void (*istat)(), (*qstat)(), (*hstat)();

	if (access(file, F_OK) == CERROR) {
		mode_t omask;
		istat = signal(SIGINT, SIG_IGN);
		qstat = signal(SIGQUIT, SIG_IGN);
		hstat = signal(SIGHUP, SIG_IGN);
		omask = umask(0);
		(void) close(creat(file, MFMODE));
		(void) umask(omask);
		if (chown(file, uid, my_egid) == -1)
		    (void) posix_chown(file);
		(void) signal(SIGINT, istat);
		(void) signal(SIGQUIT, qstat);
		(void) signal(SIGHUP, hstat);
	}
}
