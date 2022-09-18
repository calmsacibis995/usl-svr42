/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/_mail_pipe.c	1.13.2.6"
#ident "@(#)_mail_pipe.c	2.36 'attmail mail(1) command'"
/*
    NAME
	mail_pipe - run mailbox piping program

    SYNOPSIS
	mail_pipe [-xdebug_level] -rrecipient -RReturn_path -ccontent_type -Ssubject

    Inheritable Privileges: -
    Fixed Privileges: P_SETUID

    DESCRIPTION
	mail_pipe runs the program specified in /var/mail/mailuser
	as mailuser.
*/

#include "libmail.h"
#ifdef SVR4_1
# include <priv.h>
#endif /* SVR4_1 */

#define SAME	0
#define	frwrd	"Forward to"

extern		char	**environ;
extern		char	*optarg;	/* for getopt */

static		void	error_msg ARGS((int rc, char *fmt, ...));
static		char	*add_quotes ARGS((char *p, char *str));

static		char	dbgfname[20];
static		FILE	*dbgfp;
static		int	debug;
extern	int	ma_id();
static		char	mailforward[FILENAME_MAX] = FWRDDIR;

	/* environment stuff */
#ifdef SVR4
static		char	PATH[] = "PATH=/usr/bin:/usr/lbin:";
static		char	SHELL[] = "SHELL=/usr/bin/sh";
#else
static		char	PATH[] = "PATH=/bin:/usr/bin:/usr/lbin:";
static		char	SHELL[] = "SHELL=/bin/sh";
#endif
static		char	LOGNAME[50] = "LOGNAME=";
static		char	HOME[128] = "HOME=";
#define		HOMELOC 3
#define		TZLOC 4
static		char	*newenviron[] = { PATH, SHELL, LOGNAME, HOME, 0, 0 };
const char	*progname = "";

static		char	MAmnotpiped[] = ":1:Mail for %s not being piped\n";

/*
 * Procedure:     main
 *
 * Restrictions:
 *		setuid(2): P_SETUID
 *		setgid(2): P_SETUID
 *		execvp(2):
 */

main(argc, argv)
int argc;
char *argv[];
{
	register	int c = 0;
	register char	ch = 0, ch1 = 0, ch2 = 0;
	register	char *p;
	register struct passwd *pwentry;
	char 		*mailuser = (char *)NULL;
	char 		*Rpath = (char *)NULL;
	char 		*con_type = (char *)NULL;
	char 		*subject = (char *)NULL;
	char		**argvec;
	char		buf[2048];	/* work area */
	char		buf2[2048];	/* work area */
	const char	*cmd;
	register unsigned int len;
	register FILE 	*mf;
	struct	stat	sbuf;
	register int	i;
	int		errflg = 0;
	int		keepdbgfile = 0;

	/* Initialize the privileges, locale and message database */
#ifdef SVR4_1
	procprivl(CLRPRV, pm_work(P_ALLPRIVS), (priv_t)0);
	setuid(getuid());
#endif /* SVR4_1 */
	progname = argv[0];
	(void) ma_id();
	(void) setlocale(LC_ALL, "");
	(void) setcat("uxemail");
	(void) setlabel("UX:mail_pipe");

	while ((c = getopt(argc, argv, "x:r:R:c:S:?")) != EOF) {
		switch (c) {
		case 'x':
			/* Set debugging level */
			debug = atoi(optarg);
			if (debug < 0) {
				/* Keep trace file even if successful */
				keepdbgfile = -1;
				debug = -debug;
			}
			break;
		case 'r':
			/* Recipient name */
			mailuser = optarg;
			break;
		case 'R':
			/* Return path to originator */
			Rpath = optarg;
			break;
		case 'c':
			/* Content-Type: */
			con_type = optarg;
			break;
		case 'S':
			/* Subject: */
			subject = optarg;
			break;
		default:
			errflg++;
		}
	}

	if (debug == 0) {
		char *pdebug;
		/* If not set as an invocation option, check for system-wide */
		/* global flag */
		if ((pdebug = mgetenv("DEBUG")) != (char *)NULL) {
			debug = atoi(pdebug);
			if (debug < 0) {
				/* Keep trace file even if successful */
				keepdbgfile = -1;
				debug = -debug;
			}
		}
	}
	if (debug > 0) {
		strcpy (dbgfname, "/tmp/MLDBGXXXXXX");
		mktemp (dbgfname);
		if ((dbgfp = fopen(dbgfname,"w")) == (FILE *)NULL) {
			pfmt(stderr, MM_ERROR, ":2:Cannot open %s: %s\n",
				dbgfname, strerror(errno));
			exit (14);
		}
		setbuf (dbgfp, NULL);
		fprintf(dbgfp, "%s: debugging level == %d\n", progname, debug);
		fprintf(dbgfp,
			"=====\n%s started. Invocation args are:\n", progname);
		for (i=0; i < argc; i++) {
			fprintf(dbgfp,"\targ[%d] = '%s'\n", i, argv[i]);
		}
	}

	if ((mailuser == (char *)NULL) || (Rpath == (char *)NULL) ||
	    (con_type == (char *)NULL) || (subject == (char *)NULL) ||
	    errflg) {
		error_msg (1, ":4:Usage: %s [-xdebug_level] -rrecipient -RRpath -ccon_type -Ssubject\n", progname);
	}

	if (strlen(mailuser) > 14) {
		error_msg (2, ":341:user name '%s' too long\n", mailuser);
	}

	/*
	 * Check mailbox for piping. This will also fail if someone else
	 * is running the program who does not have permission to read
	 * the mailbox.
	 */
	(void) strcat(mailforward, mailuser);
	if ((stat(mailforward, &sbuf) != 0) || ((sbuf.st_mode & S_ISUID) != S_ISUID)) {
	     /* File doesn't exist or setuid/setgid bit not set */
	     error_msg (3, MAmnotpiped, mailuser);
	}
	if ((mf = fopen(mailforward, "r")) == NULL) {
		error_msg (4, MAmnotpiped, mailuser);
	}
	fread(buf, (unsigned)(strlen(frwrd)), 1, mf);
	if (strncmp(buf, frwrd, strlen(frwrd)) != SAME) {
		error_msg (5, MAmnotpiped, mailuser);
	}
	buf[0] = '\0';
	while ((c = fgetc(mf)) != EOF) {
		ch2 = ch1; /* save previous values of ch */
		ch1 = ch;
		ch = (char)c;
		/*
		 * Pipe symbol ('|') must be first char of word. If not,
		 * keep looking.....
		 */
		if ((ch == '|') && (((ch1 == ' ') || (ch1 == '\t') || (ch1 == ',')) ||
				    ((ch1 == '>') && ((ch2 == ' ') || (ch2 == '\t') || (ch2 == ','))))) {
			break;
		}
	}
	if (ch != '|') {
		error_msg (6, MAmnotpiped, mailuser);
	}

	/* get/set user info */
	if (!(pwentry = getpwnam(mailuser))) {
		error_msg (7, ":6:Unknown user '%s'\n", mailuser);
	}

	if (pwentry->pw_uid != sbuf.st_uid) {
		error_msg (3, MAmnotpiped, mailuser);
	}

#ifdef SVR4_1
	procprivl(SETPRV, pm_work(P_SETUID), (priv_t)0);
#endif /* SVR4_1 */

	if (setgid(pwentry->pw_gid) == (gid_t)-1) {
#ifdef SVR4_1
		procprivl(CLRPRV, pm_work(P_ALLPRIVS), (priv_t)0);
#endif /* SVR4_1 */
		error_msg (8, ":7:setgid() failed: %s\n", strerror(errno));
	}

	if (setuid(pwentry->pw_uid) == (uid_t)-1) {
#ifdef SVR4_1
		procprivl(CLRPRV, pm_work(P_ALLPRIVS), (priv_t)0);
#endif /* SVR4_1 */
		error_msg (9, ":8:setuid() failed: %s\n", strerror(errno));
	}

#ifdef SVR4_1
	procprivl(CLRPRV, pm_work(P_ALLPRIVS), (priv_t)0);
#endif /* SVR4_1 */

	if (chdir(pwentry->pw_dir) == -1) {
		error_msg (10, ":9:Cannot change directory to \"%s\": %s\n",
			pwentry->pw_dir, strerror(errno));
	}

	/*
	 * Change the environment so it only has the variables
	 * PATH, SHELL, LOGNAME, HOME and TZ.
	 */
	environ = newenviron;
	(void) strcat(LOGNAME, mailuser);
	if ((len = strlen(pwentry->pw_dir) + 5) > sizeof(HOME)) {
		if (!(newenviron[HOMELOC] = malloc(len+1))) {
			error_msg (11, ":10:Out of memory: %s\n", strerror(errno));
		}
		(void) strcpy(newenviron[HOMELOC], HOME);
	}
	(void) strcat(HOME, pwentry->pw_dir);
	newenviron[TZLOC] = (p = getenv("TZ")) ? p-3 : 0;

	(void) umask(027);

	/* read the rest of the line */
	fgets(buf2, sizeof(buf2), mf);
	buf2[sizeof(buf2)-1] = '\0';

	/* skip past any exit-code (S=;C=;F=;) definitions */
	cmd = skipspace(buf2);
	if (isalpha(cmd[0]) && cmd[1] == '=') {
		cmd = skiptospace(cmd);
		cmd = skipspace(cmd);
	}

	/*
	 * Scan for any %keywords and replace with appropriate
	 * values
	 */
	p = buf;
	while ((ch = *cmd++) != '\0') {
		switch (ch) {
		case '\\':
			/* take next char regardless */
			ch = *cmd++;
			if (ch == '\0') {
				break;
			}
			/* FALLTHROUGH */

		default:
			*p++ = ch;
			break;

		case '\n':
			break;

		case '%':
			ch = *cmd++;
			if (ch == '\0') {
				*p++ = '%';
				break;
			}
			switch (ch) {
			default:
				*p++ = '%';
				*p++ = ch;
				continue;
			case '\n':
				*p++ = '%';
				break;
			case 'R': /* Return path to originator */
				p = add_quotes(p, Rpath);
				continue;
			case 'c': /* Content-Type */
				p = add_quotes(p, con_type);
				continue;
			case 'S': /* Subject: */
				p = add_quotes(p, subject);
				continue;
			}
			break;
		}
	}
	*p = '\0';

	/* parse out the command */
	if ((argvec = setup_exec (buf)) == (char **)NULL) {
		error_msg (12, ":11:No command specified after pipe symbol\n");
	}
	if (debug) {
		fprintf(dbgfp,"%s: arg vec to exec =\n", progname);
		for (i= 0; argvec[i] != (char *)NULL; i++) {
			fprintf(dbgfp,"\targvec[%d] = '%s'\n", i, argvec[i]);
		}
		fclose (dbgfp);
	}
	if (keepdbgfile == 0) {
		unlink (dbgfname);
	}
	execvp (*argvec, argvec);
	debug = 0;
	error_msg (13, ":12:Cannot execute %s: %s\n", *argvec, strerror(errno));
	/* NOTREACHED */
}

/*
    NAME
	error_msg - print error message and exit

    SYNOPSIS
	void error_msg(int rc, char *fmt, args...)

    Restrictions:

    DESCRIPTION
	error_msg() prints out an error message and
	then exits with the given exit code "rc".
*/

/* PRINTFLIKE2 */
static void
#ifdef __STDC__
error_msg(int rc, char *fmt, ...)
#else
# ifdef lint
error_msg(Xrc, Xfmt, va_alist)
int Xrc;
char *Xfmt;
va_dcl
# else
error_msg(va_alist)
va_dcl
# endif
#endif
{
#ifndef __STDC__
	int	rc;
	char 	*fmt;
#endif
	va_list	args;

#ifndef __STDC__
# ifdef lint
	rc = Xrc;
	fmt = Xfmt;
# endif
#endif

#ifdef __STDC__
	va_start(args, fmt);
#else
	va_start(args);
	rc = va_arg(args, int);
	fmt = va_arg(args, char *);
#endif

	vpfmt(stderr, MM_ERROR, fmt, args);

	if (debug > 0) {
		vfprintf(dbgfp, fmt, args);
	}
	va_end(args);
	exit (rc);
	/* NOTREACHED */
}

/*
    NAME
	add_quotes - surround string with double-quotes

    SYNOPSIS
	char *add_quotes(char *p, char *str)

    Restrictions:

    DESCRIPTION
	Must prepend and append double-quotes around the string
	so that setup_exec() will keep the string as a single argument.
*/

static char *add_quotes(p, str)
register char *p;
register char *str;
{
    *p++ = '"';
    for ( ; *str; str++) {
	if (*str == '"') {
	    /* Must escape any embedded */
	    /* quotes to make transparent */
	    /* to setup_exec(). */
	    *p++ = '\\';
	}
	*p++ = *str;
    }
    *p++ = '"';
    return p;
}
