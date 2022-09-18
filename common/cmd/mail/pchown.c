/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/pchown.c	1.5"
#ident "@(#)pchown.c	1.6 'attmail mail(1) command'"
/*
    NAME
	pchown - workaround for POSIX systems which don't allow chown(3)

    SYNOPSIS
	/usr/lib/mail/posixchown -m username
	/usr/lib/mail/posixchown -m :dirname

		chown username mail /var/mail/username
		chown root mail /var/mail/:*

	/usr/lib/mail/posixchown -s username

		chown username mail /var/mail/:saved/username

	/usr/lib/mail/posixchown -T

		chown bin mail /etc/mail/Tmailsurr

	/usr/lib/mail/posixchown -S [dirname]

		chown smtp mail /var/spool/smtpq
		chown smtp mail /var/spool/smtpq/dirname

	/usr/lib/mail/posixchown -a alias.t

		chown bin mail /etc/mail/alias.t

    DESCRIPTION
	This program runs setuid root. Its purpose is to get around the
	stupid restriction on chown() found on some POSIX systems. The mail
	programs execute this program in a restricted manner, passing
	in one of the arguments as shown above.
*/

#include "mail.h"

static struct passwd *cgetpwnam ARGS((const char *arg));
static struct group *cgetgrnam ARGS((const char *arg));
static void cchown ARGS((const char *buf, uid_t uid, gid_t gid));

const char *progname = "";

void usage()
{
    pfmt(stderr, MM_ACTION, ":499:Usage: %s -m [username]\n", progname);
    pfmt(stderr, MM_NOSTD, ":500:\t%s -T\n", progname);
    pfmt(stderr, MM_NOSTD, ":501:\t%s -S [dirname]\n", progname);
    exit(1);				/* NOTREACHED */
}

main(argc, argv)
int argc;
char **argv;
{
    register int c;
    int mflag = 0, Tflag = 0, Sflag = 0, aflag = 0, sflag = 0;
    char *arg = 0;
    char buf[FILENAME_MAX];
    int arglen;

    progname = argv[0];

    while ((c = getopt(argc, argv, "amTSs?")) != -1)
	switch (c)
	    {
	    case 'a': aflag = 1; break;
	    case 'm': mflag = 1; break;
	    case 'T': Tflag = 1; break;
	    case 'S': Sflag = 1; break;
	    case 's': sflag = 1; break;
	    default: usage(); /* NOTREACHED */
	    }

    if (argc != optind)
	arg = argv[optind++];

    if (argc != optind)
	usage();

    if (aflag + mflag + Tflag + Sflag + sflag > 1)
	usage();

    /* an alias file under /etc/mail */
    if (aflag)
	{
	if (!arg)
	    usage();

	if (strchr(arg, '/'))
	    usage();

	arglen = strlen(arg);
	if (arglen < 3)
	    usage();

	if (strcmp(arg + arglen - 3, ".t") == 0)
	    {
	    struct group *gr = cgetgrnam("mail");
	    (void) sprintf(buf, "/etc/mail/%s", arg);
	    cchown(buf, (uid_t)2, gr->gr_gid);
	    }
	}

    /* either /var/mail/user or /var/mail/:xyz */
    else if (mflag)
	{
	if (!arg)
	    usage();

	if (strchr(arg, '/'))
	    usage();

	if (arg[0] == ':')
	    {
	    struct group *gr = cgetgrnam("mail");
	    (void) sprintf(buf, "%s/%s", MAILDIR, arg);
	    cchown(buf, (uid_t)0, gr->gr_gid);
	    }

	else
	    {
	    struct passwd *pw = cgetpwnam(arg);
	    struct group *gr = cgetgrnam("mail");
	    (void) sprintf(buf, "%s/%s", MAILDIR, arg);
	    cchown(buf, pw->pw_uid, gr->gr_gid);
	    }
	}

    /* either /var/spool/smtpq or /var/spool/smtpq/dir */
    else if (Sflag)
	{
	struct passwd *pw = cgetpwnam("smtp");
	struct group *gr = cgetgrnam("mail");

	if (arg)
	    {
	    (void) sprintf(buf, "%s/%s", spoolsmtpq, arg);
	    cchown(buf, pw->pw_uid, gr->gr_gid);
	    }

	else
	    cchown(spoolsmtpq, pw->pw_uid, gr->gr_gid);
	}


    /* /var/mail/:saved/user */
    else if (sflag)
	{
	if (arg)
	    {
	    struct passwd *pw = cgetpwnam(arg);
	    struct group *gr = cgetgrnam("mail");
	    if (strchr(arg, '/'))
		usage();
	    (void) sprintf(buf, "%s/:saved/%s", MAILDIR, arg);
	    cchown(buf, pw->pw_uid, gr->gr_gid);
	    }
	else
	    usage();
	}

    /* /etc/mail/Tmailsurr */
    else if (Tflag)
	{
	struct group *gr = cgetgrnam("mail");
	cchown(TMAILSURR, (uid_t)2, gr->gr_gid);
	}

    return 0;
}

/*
    NAME
	cgetpwnam - checking version of getpwnam
*/
static struct passwd *cgetpwnam(arg)
const char *arg;
{
    struct passwd *pw = getpwnam(arg);
    if (!pw)
	{
	pfmt(stderr, MM_ERROR, ":502:%s: unknown user %s\n", progname, arg);
	exit(1);
	}
    return pw;
}

/*
    NAME
	cgetgrnam - checking version of getgrnam
*/
static struct group *cgetgrnam(arg)
const char *arg;
{
    struct group *gr = getgrnam(arg);
    if (!gr)
	{
	pfmt(stderr, MM_ERROR, ":502:%s: unknown user %s\n", progname, arg);
	exit(1);
	}
    return gr;
}

/*
    NAME
	cchown - checking version of chown
*/
static void cchown(buf, uid, gid)
const char *buf;
uid_t uid;
gid_t gid;
{
    if (chown(buf, uid, gid) == -1)
	{
	pfmt(stderr, MM_ERROR, ":503:%s: cannot chown(%s, %d, %d)\n", progname, buf, uid, gid);
	exit(1);
	}
}
