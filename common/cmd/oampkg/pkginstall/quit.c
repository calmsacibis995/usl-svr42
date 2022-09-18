/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:common/cmd/oampkg/pkginstall/quit.c	1.11.8.13"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/utsname.h>
#include <limits.h>
#include <pkgdev.h>
#include <pkglocs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wait.h>
#include <errno.h>
#include "install.h"

extern struct admin
		adm;
struct pkgdev	pkgdev;
extern int	dparts,
		started,
		update,
		iflag,
		failflag,
		warnflag,
		reboot, ireboot;

extern char	tmpdir[],
		pkgloc[],
		uncompdir[PATH_MAX],
		*prog,
		*msgtext,
		*pkginst,
		*pkgname,
		*log,
		*quiet;

extern void	*calloc(),
		nqtext(),
		ptext(),
		logerr(),
		ds_skiptoend(),
		exit();
extern int	chdir(),
		rrmdir();

static char	*reason();

void		trap(), quit();
static void	mailmsg(), quitmsg();

void
trap(signo)
int signo;
{
	if((signo == SIGINT) || (signo == SIGHUP))
		quit(3);
	else
		quit(1);
}

extern	pid_t	dd_pid;		/* PID of dd process */
siginfo_t	infop;

void
quit(retcode)
int	retcode;
{
	struct	stat status;

	char t_contents[PATH_MAX];
	(void) signal(SIGINT, SIG_IGN);


	if(retcode != 99) {
		if((retcode % 10) == 0) {
			if(failflag)
				retcode += 1;
			else if(warnflag)
				retcode += 2;
		}

		if(ireboot)
			retcode = (retcode % 10) + 20;
		if(reboot)
			 retcode = (retcode % 10) + 10;
	}

	if(tmpdir[0])
		(void) rrmdir(tmpdir);

	/*
	 * If we just installed a compressed package,
	 * instvol created a directory into which it
	 * uncompresses files.  Let's remove it if
	 * it exists.
	 */
	if(stat(uncompdir, &status) == 0) 
		(void) rrmdir(uncompdir);
	
	/* remove the temp contents file if it exists */
	(void) sprintf(t_contents, "%s/t.contents", PKGADM);
	if(access(t_contents, 0) == 0) 
		(void) unlink(t_contents);

	/* send mail to appropriate user list */
	mailmsg(retcode);

	/* display message about this installation */
	quitmsg(retcode);

	/* no need to umount device since calling process 
	 * was responsible for original mount
	 */

	if(!update && !started && pkgloc[0]) {
		(void) chdir("/");
		(void) rrmdir(pkgloc);
	}
	if(dparts > 0) 
		ds_skiptoend(pkgdev.cdevice);
	/*
	 * If the datastream media is diskette, the pkginstall process 
	 * hangs.  To prevent this, kill the dd process reading from 
	 * the datastream diskette device.
	 */

	if (dd_pid > 0 && pkgdev.bdevice) {
		(void) sigsend(P_PID, dd_pid, SIGKILL);
		if(waitid(P_PID, dd_pid, &infop, WSTOPPED||WEXITED) < 0) {
			/* If dd_pid is no longer around disregard */
			if (errno != ECHILD) {
				progerr("could not terminate dd process");
				exit(1); 
			}
		}
		dd_pid = -1;
	}
	/* 
	 * If quit() was called because a Set Installation Package's request
	 * request script returned NOSET (no packages from set were selected
	 * for installation), quit gracefully.
	 */
	if(retcode == NOSET)
		retcode = 0;
	(void)ds_close(1);
	exit(retcode);
}

static void
quitmsg(retcode)
int	retcode;
{
	char	*status;
	extern	char *category;

	status = reason(retcode);

	if(quiet && strcmp(quiet, "true")) {
		(void) putc('\n', stderr);
		if(iflag)
			ptext(stderr, "Processing of request script %s.", status);
		/*
		 * Do not display completion of set installation packages.
		 */
		else if(pkginst && strcmp(category, "set"))
			ptext(stderr, "Installation of %s (%s) %s.", 
				pkgname, pkginst, status);
	
		if(retcode == NOSET)
			ptext(stderr, "No packages were selected for installation from set <%s>\n", pkgname);
		if(retcode && !started)
			ptext(stderr, "No changes were made to the system.");
	}
}

static void
mailmsg(retcode)
int retcode;
{
	struct utsname utsbuf;
	FILE	*pp;
	char	*status, *cmd;
	extern	char	*package;
	extern	char	*pkgabrv;

	if(iflag || (adm.mail == NULL))
		return;

	cmd = calloc(strlen(adm.mail) + sizeof(MAILCMD) + 2, sizeof(char));
	if(cmd == NULL) {
		logerr("WARNING: unable to send e-mail notofication");
		return;
	}

	(void) sprintf(cmd, "%s %s", MAILCMD, adm.mail);
	if((pp = popen(cmd, "w")) == NULL) {
		logerr("WARNING: unable to send e-mail notofication");
		return;
	}

	if(msgtext)
		nqtext(pp, msgtext, pkgabrv);

	(void) strcpy(utsbuf.nodename, "(unknown)");
	(void) uname(&utsbuf);
	status = reason(retcode);
	nqtext(pp, "\nInstallation of %s on %s as package instance <%s> %s.\n",
			pkgname, utsbuf.nodename, (pkginst ? pkginst : package), status);
	switch(retcode) {
	   case  1:
	   case  2:
	   case  5:
	   case 11:
	   case 12:
	   case 15:
	   case 21:
	   case 22:
	   case 25:
	   case 99:
		nqtext(pp, "\nError messages are logged in /var/sadm/install/logs/%s.log.\n",
			(pkginst ? pkginst : package));
		break;
	}
	if(pclose(pp)) 
		logerr("WARNING: e-mail notification may have failed");
}

static char *
reason(retcode)
{
	char	*status;

	switch(retcode) {
	  case  0:
	  case 10:
	  case 20:
		status = "was successful";
		break;

	  case  1:
	  case 11:
	  case 21:
		status = "failed";
		break;

	  case  2:
	  case 12:
	  case 22:
		status = "partially failed";
		break;

	  case  3:
	  case 13:
	  case 23:
		status = "was terminated due to user request";
		break;

	  case  4:
	  case 14:
	  case 24:
		status = "was suspended (administration)";
		break;

	  case  5:
	  case 15:
	  case 25:
		status = "was suspended (interaction required)";
		break;

	  case NOSET:
		status = "was terminated - no set packages selected for installation";
		break;

	  case 99:
		if(started)
			status = "failed (internal error) - package partially installed";
		else
			status = "failed (internal error)";
		break;

	  default:
		status = "failed with an unrecognized error code.";
		break;
	}

	return(status);
}
