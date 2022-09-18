/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/notifyu.c	1.6.2.2"
#ident	"@(#)notifyu.c	1.11 'attmail mail(1) command'"
/* notify(u,m): notifies user "u" with message "m"
 * J. A. Kutsch  ho 43233  x-3059
 * January 1981
 *
 * Converted to C++
 * Tony Hansen
 * January 1989
 *
 * Added a timeout around the open and write to the tty device.
 * This prevents hanging the process if a person logs off at
 * just the wrong moment (datakit seems to have problems here).
 * Tony Hansen
 * March 1989
 *
 * If user is logged in more than once, notification is made to all terminals.
 * notification is given without "Message from ..." preface.
 * If messages are being denied, notify ignores that terminal
 *
 * Converted to common K&R C, ANSI C and C++
 * Tony Hansen
 * April 1989
 *
 */

#include "libmail.h"
#include <utmp.h>
#if !defined(__cplusplus) && !defined(c_plusplus)
# ifdef SIGPOLL
#  define SIGRET void
# else
#  define SIGRET int
# endif
typedef SIGRET (*SIG_PF) ();
#endif

static volatile sig_atomic_t sigcaught;

/* Do nothing, but allow the write() to break. */
static SIGRET catcher()
{
    sigcaught = 1;
}

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
void notify(char *user, char *msg, int check_mesg_y, char *etcdir)
#else
void notify(user, msg, check_mesg_y, etcdir)
    char *user;
    char *msg;
    int check_mesg_y;
    char *etcdir;
#endif
{
    /* search the utmp file for this user */
    SIG_PF old;
    unsigned int oldalarm;
    register FILE *utfp;
    char buf[FILENAME_MAX];

    /* break out if fopen() of /etc/utmp hangs */
    old = (SIG_PF)signal(SIGALRM, (SIG_PF)catcher);
    oldalarm = alarm(60);

    /* open /etc/utmp */
    sprintf(buf, "%s/etc/utmp", etcdir);
    utfp = fopen(buf, "r");

    /* clean up our alarm */
    alarm(0);
    signal(SIGALRM, old);
    alarm(oldalarm);

    if (utfp != 0)
	{
	struct utmp utmp;
	while (fread((char*)&utmp, sizeof(utmp), 1, utfp))
	    /* grab the tty name */
	    if (strncmp(user, utmp.ut_name, 8) == 0)
		{
		char tty[9];
		char dev[FILENAME_MAX];
		FILE *port;
		register int i;
		pid_t pid;

		for (i = 0; i < sizeof(utmp.ut_line); i++)
		    tty[i] = utmp.ut_line[i];
		tty[i] = '\0';

		/* stick /dev/ in front */
		sprintf(dev, "%s/dev/%s", etcdir, tty);

		/* stat dir to make certain 'mesg y' is set */
		if (check_mesg_y)
		    {/*EMPTY*/
		    /* Don't do this test for now. It may be an enhancement in the future. */
		    }

		/* Sometimes a write to a tty will hang. Sometimes it's */
		/* so bad that an alarm won't break out of it. So we'll */
		/* do a fork, let the child do the writes. The parent will */
		/* set an alarm; if it goes off, the child will be killed */
		/* and processing will continue. */
		switch (pid = fork())
		    {
		    case -1:	/* fork failed, skip this one */
			break;

		    case 0:	/* child */
			/* write to the tty */
			port = fopen(dev, "w");
			if (port != 0) {
			    (void) fprintf(port,"\r\n%s\r\n",msg);
			    (void) fclose (port);
			}
			_exit(0);
			/* NOTREACHED */

		    default:	/* parent */
			/* break out if write() to the tty hangs */
			old = (SIG_PF)signal(SIGALRM, (SIG_PF)catcher);
			sigcaught = 0;
			oldalarm = alarm(60);
			/* Wait for the child to exit or the alarm to go off. */
			/* There's no need to be rigorous in the use of wait() here. */
			(void) wait((int *)0);
			if (sigcaught)
			    (void) kill(pid, SIGKILL);

			/* clean up our alarm */
			alarm(0);
			signal(SIGALRM, old);
			alarm(oldalarm);
		    }
		}

	(void) fclose (utfp);
	}
}
