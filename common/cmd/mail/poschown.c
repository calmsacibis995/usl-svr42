/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/poschown.c	1.4"
#ident "@(#)poschown.c	1.5 'attmail mail(1) command'"
/*
    NAME
	posix_chown - workaround for POSIX systems which don't allow chown(3)

    SYNOPSIS
	int posix_chown(const char *arg)

    DESCRIPTION
	This function attempts to get around the stupid restriction
	on chown() found on some POSIX systems. The mail programs
	use this function if chown(3) fails, which in turn
	executes the program /usr/lib/mail/pchown in a restricted
	manner.
*/

#include "mail.h"

static int runchown(arg1, arg2)
const char *arg1, *arg2;
{
    const char *arglist[4];
    arglist[0] = "/usr/lib/mail/pchown";
    arglist[1] = arg1;
    arglist[2] = arg2;
    arglist[3] = 0;
    return systemvp(arglist[0], arglist, 0);
}

#define ERR ((errno = EACCES), -1)
#define RET ((ret != 0) ? ((errno = EACCES), -1) : 0)

/* ARGSUSED */
int posix_chown(arg)
const char *arg;
{
    char buf[FILENAME_MAX];
    int ret;

    if (strcmp(arg, ".") == 0)
	{
	arg = getcwd(buf, FILENAME_MAX);
	if (!arg)
	    return ERR;
	}

    if (strncmp(arg, MAILDIR, 10) == 0)
	{
	const char *p = arg + 10;
	if (p[0] == ':')
	    {
	    /* a file under /var/mail/:saved */
	    if (strncmp(p, ":saved/", 7) == 0)
		{
		p += 7;
		if (strchr(p, '/'))
		    return ERR;
		ret = runchown("-s", p);
		return RET;
		}

	    /* one of the /var/mail/:* dirs */
	    if (strchr(p, '/'))
		return ERR;

	    ret = runchown("-m", p);
	    return RET;
	    }

	else
	    {
	    /* a file under /var/mail */
	    if (strchr(p, '/'))
		return ERR;
	    ret = runchown("-m", p);
	    return RET;
	    }
	}

    else if (strncmp(arg, spoolsmtpq, strlen(spoolsmtpq)) == 0)
	{
	const char *p = arg + strlen(spoolsmtpq);
	/* directory under /var/spool/smtpq */
	if (p[0] == '/')
	    {
	    p++;
	    if (strchr(p, '/'))
		return ERR;
	    ret = runchown("-S", p);
	    return RET;
	    }

	else if (p[0] != '\0')
	    {
	    return ERR;
	    }

	/* the /var/spool/smtpq directory itself */
	ret = runchown("-S", (char*)0);
	return RET;
	}

    else if (strncmp(arg, "/etc/mail/", 10) == 0)
	{
	const char *p = arg + 10;
	int len;
	if (strchr(p, '/'))
	    return ERR;

	/* /etc/mail/Tmailsurr */
	if (strcmp(p, "Tmailsurr") == 0)
	    {
	    ret = runchown("-T", (char*)0);
	    return RET;
	    }

	len = strlen(p);
	if (len < 3)
	    return ERR;

	/* an alias file */
	if (strcmp(p + len - 3, ".t") == 0)
	    {
	    ret = runchown("-a", p);
	    return RET;
	    }

	return ERR;
	}

    else
	return ERR;
}
