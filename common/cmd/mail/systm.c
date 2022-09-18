/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/systm.c	1.1.2.2"
#ident "@(#)systm.c	1.3 'attmail mail(1) command'"
#include "mail.h"
#ifdef SVR4_1
# include <priv.h>
# include <mac.h>
# include <sys/secsys.h>
#endif
/*
    NAME
	systm - version of system(3) which resets uid,gid

    SYNOPSIS
	int systm(char *command)

    DESCRIPTION
	Invoke the shell to execute the command, and wait for
	the command to terminate.

    RETURNS
	status-> command exit status
*/

int systm(s)
char *s;
{
    pid_t	pid;

    /*
	    Spawn the shell to execute command, however, since the
	    mail command runs setgid mode reset the effective group
	    id to the real group id so that the command does not
	    acquire any special privileges
    */
    if ((pid = fork()) == CHILD) {
	    setuid(my_uid);
	    setgid(my_gid);
#ifdef SVR4_1
	    /* Don't permit any of our privileges (if any) */
	    /* to be passed on to child processes. */
	    (void) procprivl(CLRPRV, pm_max(P_ALLPRIVS), (priv_t)0);
#endif /* SVR4_1 */
#ifdef SVR3
	    execl("/bin/sh", "sh", "-c", s, (char*)NULL);
#else
	    execl("/usr/bin/sh", "sh", "-c", s, (char*)NULL);
#endif
	    exit(127);
    }
    return (dowait(pid));
}
