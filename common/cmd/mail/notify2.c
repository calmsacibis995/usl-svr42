/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/notify2.c	1.5.2.4"
#ident "@(#)notify2.c	2.18 'attmail mail(1) command'"
#include "libmail.h"

/*
 * notify2 - Do user-notification of new incoming mail message.
 *           Typically this program is used as the command in the
 *	     "Forward to >|command" line. See also notify(1) which sets up
 *	     the user's mailfile to utilize this command.....
 */

extern char	*optarg;	/* for getopt */
extern char	*getenv();
extern void	notify();

static int dobackup();
static void securebuf();
static void notifyall();

main(argc, argv)
int	argc;
char	*argv[];
{
	char		*originator = (char *)NULL;
	char		*subject = (char *)NULL;
	register char	*logname;
	char		buf[2048];
	register int	c = 0;
	int		errflg = 0;

	(void) setlocale(LC_ALL, "");
	(void) setlabel("UX:notify2");
	(void) setcat("uxemail");

	while ((c = getopt(argc, argv, "o:s:?")) != EOF) {
		switch(c) {
		case 'o':
			/* who sent this one */
			originator = optarg;
			break;
		case 's':
			/* value of Subject: line if present */
			subject = optarg;
			break;
		default:
			errflg++;
			break;
		}
	}

	if ((originator == (char *)NULL) ||
	    (subject == (char *)NULL)) {
		errflg++;
	}

	if (errflg) {
		pfmt(stderr, MM_ACTION,
			":408:Usage: %s -o originator -s subject\n",
			argv[0]);
		exit(1);
	}

	/*
	 * Who am I? Note that $LOGNAME is set by
	 * mail itself and is guaranteed to be valid.
	 */
	if ((logname = getenv("LOGNAME")) == (char *)NULL) {
		exit (0);
	}

	sprintf(buf, "\7\7\7New mail from '%s'\r\n\7\7Subject: %s\r\n\n",
		originator, subject);
	securebuf(buf);
	notifyall(logname, buf);
	exit (0);
	/* NOTREACHED */
}

/*
    Make certain that non-printable characters
    within the mail message become printable.
*/
static void securebuf(buf)
    register char *buf;
{
    for ( ; *buf; buf++)
	if (!isprint(*buf) && !isspace(*buf) && (*buf != '\07'))
		*buf = '!';
}

/*
    Look up the list of systems in /etc/mail/notify.sys.
    The 1st column contains a system name and the 2nd
    column contains the path to the root for that system.
*/
static void notifyall(user, msg)
    char *user;
    char *msg;
{
    FILE *sysfp = fopen("/etc/mail/notify.sys", "r");

    /*
     * Look for the root paths. Skip the
     * line for the current system.
     */
    if (sysfp) {
	char buf[BUFSIZ];
	struct utsname utsbuf;
	uname(&utsbuf);
	while (fgets(buf, sizeof(buf), sysfp)) {
	    char *sys = strtok(buf, " \t\n");
	    char *dir = strtok((char*)0, " \t\n");
	    if (strcmp(sys, utsbuf.sysname) != 0)
		notify(user, msg, 1, dir);
	}
	fclose(sysfp);
    }
    notify(user, msg, 1, "");
}
