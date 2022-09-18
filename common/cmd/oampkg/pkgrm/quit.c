/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgrm/quit.c	1.7.9.2"
#ident  "$Header: quit.c 1.2 91/06/27 $"

#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include <pkgdev.h>
#include <pkglocs.h>

extern struct pkgdev
		pkgdev;
extern int	intfchg,
		npkgs,
		failflag,
		admnflag,
		nullflag,
		intrflag,
		warnflag,
		reboot, ireboot;

extern void	ckreturn(),
		intf_reloc(),
		ptext(),
		echo(),
		exit();
extern int	chdir(),
		pkgumount();

#define MSG_REBOOT \
"\\n*** IMPORTANT NOTICE ***\\n\
\\tIf removal of all desired packages is complete,\\n\
\\tthe machine should be rebooted in order to\\n\
\\tensure sane operation. Execute the shutdown\\n\
\\tcommand with the appropriate options and wait for\\n\
\\tthe \"Console Login:\" prompt."

void		trap(), quit();

void
trap(signo)
int signo;
{
	if((signo == SIGINT) || (signo == SIGHUP))
		quit(3);
	else
		quit(1);
}

void
quit(retcode)
int retcode;
{
	char t_contents[PATH_MAX];
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);

	if(retcode != 99) {
		ckreturn(retcode);
		if(failflag)
			retcode = 1;
		else if(warnflag)
			retcode = 2;
		else if(intrflag)
			retcode = 3;
		else if(admnflag)
			retcode = 4;
		else if(nullflag)
			retcode = 5;
		else
			retcode = 0;
		if(ireboot)
			retcode += 20;
		if(reboot)
			retcode += 10;
	}

	/* remove the temp contents file if it exists */
	(void) sprintf(t_contents, "%s/t.contents", PKGADM);
	if(access(t_contents, 0) == 0) 
		(void) unlink(t_contents);

	if(reboot || ireboot)
		ptext(stderr, MSG_REBOOT);

	if(pkgdev.mount) {
		(void) chdir("/");
		(void) pkgumount(&pkgdev);
	}
	if(npkgs == 1)
		echo("\n1 package was not processed!\n");
	else if(npkgs)
		echo("\n%d packages were not processed!\n", npkgs);

	if(intfchg)
		intf_reloc();
	exit(retcode);
}
