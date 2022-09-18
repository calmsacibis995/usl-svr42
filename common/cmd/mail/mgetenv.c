/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mgetenv.c	1.5.2.2"
#ident "@(#)mgetenv.c	1.5 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	mgetenv, Mgetenv - manage the /etc/mail/mailcnfg environment space

    SYNOPSIS
	char *mgetenv(char *name);
	char *Mgetenv(char *name);

    DESCRIPTION
	mgetenv() returns the environment value from the
	/etc/mail/mailcnfg environment.

	Return values:	(char*)0 - no value for that variable
			pointer  - the value

	Mgetenv() returns the environment value from the
	/etc/mail/mailcnfg environment.

	Return values:	"" - no value for that variable
			pointer  - the value

	All work is passed on to xgetenv() and Xgetenv(),
	with a check for xsetenv(MAILCNFG).
*/

static int xset = 0;

static void msetenv()
{
    struct stat statb;
    static char mailcnfg[] = MAILCNFG;
    if (xsetenv(mailcnfg) != 1)
	if (stat(mailcnfg, &statb) == 0)
	    {
	    /* file DOES exist! */
	    lfmt(stderr, MM_ERROR, ":119:Cannot access %s: %s\n",
		mailcnfg, strerror(errno));
	    exit(1);
	    /* NOTREACHED */
	    }

    xset = 1;
}

#ifdef __STDC__
char *mgetenv(const char *env)
#else
char *mgetenv(env)
char *env;
#endif
{
    if (xset == 0)
	msetenv();
    return xgetenv(env);
}

#ifdef __STDC__
char *Mgetenv(const char *env)
#else
char *Mgetenv(env)
char *env;
#endif
{
    if (xset == 0)
	msetenv();
    return Xgetenv(env);
}
