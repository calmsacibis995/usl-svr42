/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/smtploop.c	1.4.2.4"
#ident "@(#)smtploop.c	1.9 'attmail mail(1) command'"
/*
    NAME
	smtploop - loop through MLD /var/spool/smtpq invoking smtpsched

    SYNOPSIS
	smtploop [smtpsched arguments]

    DESCRIPTION
	Smtploop will loop through the MLD directory /var/spool/smtpq,
	invoking smtpsched at each MLD level. All additional arguments are
	passed on to smtpsched.

	If /var/spool/smtpq is not an MLD, then smtpsched is invoked without
	changing levels.

    SECURITY LEVEL
	Smtploop requires P_SETPLEVEL privileges.
*/

#include "libmail.h"
#ifdef SVR4_1
# include <priv.h>
# include <mac.h>
#endif

#include "lpriv.h"

static char smtpdir[] = "/var/spool/smtpq";
static char smtpsched[] = "/usr/lib/mail/surrcmd/smtpsched";

const char *progname = "";

void doexec(seclevel, argv)
char *seclevel;
char **argv;
{
#ifdef SVR4_1
    level_t iseclevel = strtoul(seclevel, (char**)0, 16);
#endif
    pid_t pid;
    unsigned int sleeptime = 1;
    while ((pid = fork()) == -1)
	(void) sleep(sleeptime++);

    if (pid == 0)	/* child */
	{
#ifdef SVR4_1
	procprivl(SETPRV, pm_work(P_SETPLEVEL), (priv_t)0);
	if (lvlproc(MAC_SET, &iseclevel) < 0)
	    errexit(1, errno, ":60:Cannot set level to %s\n", seclevel);
	procprivl(CLRPRV, pm_work(P_SETPLEVEL), (priv_t)0);
#endif
	argv[0] = smtpsched;
	(void) execvp(smtpsched, argv);
	_exit(1);
	/* NOTREACHED */
	}

    else		/* parent */
	waitpid(pid, (int*)0, 0);
}

#ifdef SVR3
waitpid(pid, null1, null2)
pid_t pid;
int *null1;
int null2;
{
	register int ret;
	do {
		ret = wait((int *)0);
	} while((ret >= 0) && (ret != pid));
}
#endif

/* ARGSUSED */
main(argc, argv)
int argc;
char **argv;
{
    int mldrc;
#ifdef SVR4_1
    procprivl(CLRPRV, pm_work(P_ALLPRIVS), (priv_t)0);
#endif
    setuid(getuid());
    progname = argv[0];
	signal(SIGCLD, SIG_DFL); /* Allow us to wait on children */

    if ((mldrc = check4mld(smtpdir)) == 0)
	{
	DIR *sdirp;
	struct dirent *sdp;

	/* open /var/spool/smtpq in real mode */
	sdirp = realmode_opendir(smtpdir);

	/* while read security-level */
	while ((sdp = readdir(sdirp)) != 0)
	    {
	    /* look at /var/spool/smtpq/{security-level} */
	    char *seclevel = sdp->d_name;
	    if ((strcmp(seclevel, ".") != 0) && (strcmp(seclevel, "..") != 0))
		doexec(seclevel, argv);
	    }

	(void) closedir(sdirp);
	}

    else if (mldrc == -1)
#ifdef SVR4_1
	errexit(2, errno, ":61:Cannot test for MLD\n");
#else
	errexit(2, errno, "Cannot test for MLD\n");
#endif

    else
	{
	argv[0] = smtpsched;
	(void) execvp(smtpsched, argv);
	_exit(1);
	/* NOTREACHED */
	}

    return 0;
}
