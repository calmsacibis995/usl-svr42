/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mailcheck.c	1.7.2.6"
#ident "@(#)mailcheck.c	1.16 'attmail mail(1) command'"
/*
    NAME
	mailcheck - check for mail at all security levels

    SYNOPSIS
	mailcheck [-Z]

    DESCRIPTION
	Mailcheck loops through the dominated security levels looking for
	mail. Whenever it finds some, it prints a message on stdout:

		You have mail at level: Top Secret

	If there is no mail, it prints on stderr

		No mail

	If the -Z option is given, the message will show the full security level
	name instead of the security level alias name.

    SECURITY LEVEL
	Mailcheck takes no privileges.

    EXIT CODES
	0 - mail exists at some level
	1 - no mail at any checked level
	2 - some error occurred
*/
#include "libmail.h"
#ifdef SVR4_1
# include <priv.h>
# include <mac.h>
#endif /* SVR4_1 */
#include "lpriv.h"

const char *progname = "";

static int Zflag = 0;
static int counter = 0;

extern int optind;

/*
    NAME
	usage - print usage message

    SYNOPSIS
	void usage()

    DESCRIPTION
	prints usage message and exits
*/
static void usage()
{
    pfmt(stderr, MM_ACTION, ":364:Usage: %s [-Z]\n", progname);
    exit(2);
    /* NOTREACHED */
}

/*
    NAME
	rm_mr_stat - stat a file in real mode and P_MACREAD

    SYNOPSIS
	int rm_mr_stat(char *file, struct stat *pstatbuf)

    DESCRIPTION
	Set real mode and P_MACREAD.
	Stat(2) the file.
	Unset the modes.

    RETURN
	Returns the results from the stat().

    SIDE EFFECTS
	Calls errexit() if the real mode settings fail.
*/
static int rm_mr_stat(file, pstatbuf)
char *file;
struct stat *pstatbuf;
{
#ifdef SVR4_1
    int ret, sverrno;
    if (mldmode(MLD_REAL) == -1)
	errexit(2, errno, ":356:Cannot set real mode!\n");
    ret = stat(file, pstatbuf);
    sverrno = errno;
    if (mldmode(MLD_VIRT) == -1)
	errexit(2, errno, ":348:Cannot set virtual mode!\n");
    errno = sverrno;
    return ret;
#else /* SVR4_1 */
    return stat(file, pstatbuf);
#endif /* SVR4_1 */
}

/*
    NAME
	checkformail - check to see if the mail box has mail in it

    SYNOPSIS
	int checkformail(char *seclevel, char *name)

    DESCRIPTION
	Checkformail looks in /var/mail/{seclevel}/{name} to see if it
	contains mail.

	If {seclevel} is (char*)0, then /var/mail/{name} will be used.

    RETURNS
	1 if mail exists
	0 if not

    SIDE EFFECTS
	increments the global variable "counter" if mail is found
*/
static int checkformail(seclevel, name)
char *seclevel;
char *name;
{
    int ret;
    struct stat statbuf;
    char mailbox[FILENAME_MAX];

    if (seclevel)
	{
	(void) sprintf(mailbox, "%s/%s/%s", maildir, seclevel, name);
	ret = rm_mr_stat(mailbox, &statbuf);
	}

    else
	{
	(void) sprintf(mailbox, "%s/%s", maildir, name);
	ret = stat(mailbox, &statbuf);
	}

    /* No mailbox means no mail. */
    if (ret == -1)
	return 0;

    /* An empty mailbox means no mail. */
    if (statbuf.st_size == 0)
	return 0;

    /* Something must be there. */
    /* In SVr4.1, there's no need to check for Forwarding */
    /* since that's kept in another directory. */
    counter++;
    return 1;
}

/*
    NAME
	prmailmsg - print the message about having mail at a given level

    SYNOPSIS
	void prmailmsg(char *seclevel)

    DESCRIPTION
	prmailmsg looks up the given security level and prints out a message
	about there being mail at that level.

    SIDE EFFECTS
	Calls errexit() if the lvlout() calls fail.
*/
static void prmailmsg(seclevel)
char *seclevel;
{
#ifdef SVR4_1
    level_t iseclevel = strtoul(seclevel, (char**)0, 16);
    int i = lvlout(&iseclevel, (char*)0, 0, Zflag ? LVL_FULL : LVL_ALIAS);
    char *bufp;

    if (i < 0)
	errexit(2, errno, ":360:Invalid level found? (%s)\n", seclevel);
    bufp = malloc(i);
    if (!bufp)
	errexit(2, 0, ":10:Out of memory: %s\n", strerror(errno));
    i = lvlout(&iseclevel, bufp, i, Zflag ? LVL_FULL : LVL_ALIAS);
    if (i != 0)
	errexit(2, errno, ":360:Invalid level found? (%s)\n", seclevel);
    pfmt(stdout, MM_NOSTD, ":399:You have mail at level: %s\n", bufp);
    free(bufp);
#else /* SVR4_1 */
    pfmt(stdout, MM_NOSTD, ":400:You have mail\n");
#endif /* SVR4_1 */
}

main(argc, argv)
int argc;
char **argv;
{
    int c, mldrc;
    struct passwd *pw;
    char *name;

    progname = argv[0];
    /* reset our privileges */
#ifdef SVR4_1
    (void) mldmode(MLD_REAL);
#endif /* SVR4_1 */
    (void) setcat("uxemail");
    (void) setlabel("UX:mailcheck");
    (void) setlocale(LC_ALL, "");

    /* Find the options */
    while ((c = getopt(argc, argv, "Z?")) != -1)
	switch (c)
	    {
	    case 'Z': Zflag = 1; break;
	    default: usage(); /* NOTREACHED */
	    }

    /* no more arguments! */
    if (argc - optind != 0)
	usage(); /* NOTREACHED */

    /* get user name from /etc/passwd */
    pw = getpwuid(getuid());
    if (!pw)
	errexit(2, 0, ":365:Cannot find user for uid=%d!\n", getuid());
    name = pw->pw_name;

    /* check for mail */
    if ((mldrc = check4mld(maildir)) == 0)
	{
	/* We're in a MLD /var/mail */
	DIR *mdirp;
	struct dirent *mdp;

	/* open /var/mail in real mode */
	mdirp = realmode_opendir(maildir);

	/* while read security-level */
	while ((mdp = readdir(mdirp)) != 0)
	    {
	    char *seclevel = mdp->d_name;
	    if ((strcmp(seclevel, ".") != 0) && (strcmp(seclevel, "..") != 0))
		/* check for mail in /var/mail/{security-level}/user */
		if (checkformail(seclevel, name))
		    prmailmsg(seclevel);
	    }

	(void) closedir(mdirp);
	}

    else if (mldrc == -1)
	errexit(2, errno, ":491:Cannot test for Multi-Level Directory\n");

    else
	{
	/* We're in a regular /var/mail */
	if (checkformail((char*)0, name))
	    pfmt(stdout, MM_NOSTD, ":400:You have mail\n");
	}

    if (counter == 0)
	pfmt(stderr, MM_NOSTD, ":141:No mail.\n");
    return (counter == 0);
}
