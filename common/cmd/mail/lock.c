/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/lock.c	1.11.2.2"
#ident "@(#)lock.c	2.17 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	lock - lock the mail box with error messages

    SYNOPSIS
	int lock(char *user, int sending);
	int dlock(char *filename, int sending);

    DESCRIPTION
	lock()/dlock() will use maillock()/maildlock() to lock
	the mail box and produce an error message if it can't.

	If sending is true, an indication of success
	will be returned. Otherwise, done() is called.
*/

static int testlock ARGS((char *user, int reason, int sending));

/* lock the file using maillock() */
int lock(user, sending)
char	*user;
int	sending;
{
	static const char pn[] = "lock";
	Dout(pn, 5, "(%s,%d)\n", user, sending);
	return testlock(user, maillock(user,10), sending);
}

/* lock the file using maildlock() */
int dlock(filename, sending)
char	*filename;
int	sending;
{
	static const char pn[] = "dlock";
	char dir[MAXFILENAME];
	char file[MAXFILENAME];
	char *p = strrchr(filename, '/');

	Dout(pn, 5, "(%s,%d)\n", filename, sending);
	/* split the name into the path and file */
	if (p)
	    {
	    int len = p - filename;
	    strncpy(dir, filename, len);
	    dir[len] = '\0';
	    strcpy(file, p+1);
	    }

	/* the file is in the current directory */
	else
	    {
	    strcpy(dir, ".");
	    strcpy(file, filename);
	    }

	Dout(pn, 5, "\tusing dir=%s, file=%s\n", dir, file);
	return testlock(file, maildlock(file, 10, dir, 1), sending);
}

static int testlock(user, reason, sending)
char *user;
int reason;
int sending;
{
	static const char pn[] = "testlock";

	Dout(pn, 6, "(%s, %d, %d)\n", user, reason, sending);

	switch (reason) {
	case L_SUCCESS:
	    return 1;
	case L_NAMELEN:
	    errmsg(E_LOCK,
		":109:Cannot create lock file. Username '%s' is > 13 chars\n",
		user);
	    break;
	case L_TMPLOCK:
	    errmsg(E_LOCK, ":110:Cannot create temp lock file\n");
	    break;
	case L_TMPWRITE:
	    errmsg(E_LOCK, ":111:Error writing pid to lock file\n");
	    break;
	case L_MAXTRYS:
	    errmsg(E_LOCK, 
	    	":388:Creation of lockfile failed after 10 tries\n");
	    break;
	case L_ERROR:
	    errmsg(E_LOCK, 
	    	":113:Cannot link temp lockfile to lockfile\n");
	    break;
	case L_MANLOCK:
	    errmsg(E_LOCK,
	    	":114:Cannot set mandatory file lock on temp lockfile\n");
	    break;
	}
	if (sending)
		return 0;
	else
		done(0); /* NOTREACHED */
}

void unlock()
{
	mailunlock();
}
