/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:common/cmd/oampkg/pkgmk/quit.c	1.1.7.4"
#ident  "$Header: $"
#include <stdio.h>
#include <signal.h>
#include <pkgdev.h>
#include <limits.h>

extern struct pkgdev pkgdev;
extern char	pkgloc[],
		*prog,
		*t_pkgmap,
		*t_pkginfo;
extern int	started;

extern int	unlink(),
		rrmdir(),
		pkgumount();
extern void	exit();

#define MSG_COMPLETE	"## Packaging complete.\n"
#define MSG_TERM	"## Packaging terminated at user request.\n"
#define MSG_ERROR	"## Packaging was not successful.\n"

void
quit(retcode) 
int retcode;
{
	extern char cdir[PATH_MAX];
	extern char cfile[PATH_MAX];
	extern char cuniq[PATH_MAX];
	extern char fname[PATH_MAX];

	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);


	if(retcode && started)
		(void) rrmdir(pkgloc); /* clean up output directory */

	if(pkgdev.mount)
		(void) pkgumount(&pkgdev);

	if(t_pkgmap)
		(void) unlink(t_pkgmap);
	if(t_pkginfo)
		(void) unlink(t_pkginfo);
	/*
	 * Cleanup for compression.
	 */
	if(strcmp(cdir, ""))
		(void) rrmdir(cdir);
	(void) unlink(cfile);
	(void) unlink(cuniq);
	(void) unlink(fname);

	if(retcode == 3)
		(void) fprintf(stderr, MSG_TERM);
	else if(retcode)
		(void) fprintf(stderr, MSG_ERROR);
	else 
		(void) fprintf(stdout, MSG_COMPLETE);

	exit(retcode);
}
