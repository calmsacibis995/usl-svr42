/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
	wr_mnttab - write the mount table
		write an struct rmnttab entry to "rfp"
	return:
		1 on success, 0 on failure of fopen
*/

#ident	"@(#)rmount:wr_rmnttab.c	1.1.9.3"
#ident  "$Header: wr_rmnttab.c 1.2 91/06/27 $"
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include "rmount.h"
#include "rmnttab.h"

extern char *cmd;

#ifdef __STDC__
#define		SPC4(X)		sizeof(#X)
#define		SPC4_ULONG_MAX	SPC4(ULONG_MAX)
#else
#define		SPC4_ULONG_MAX	10
#endif

int
wr_rmnttab(res, dir, opt, fs, lvl)
	char	*res;
	char	*dir;
	char	*opt;
	char	*fs;
	char	*lvl;
{
	int 	ret	= 1;
	FILE	*rfp;
	struct	rmnttab	rmnt;
	char	time_str[SPC4_ULONG_MAX + 1];

	(void)sprintf(time_str,"%ld", time((long *)0));

	rmnt.rmnt_special	= res;
	rmnt.rmnt_mountp	= dir;
	rmnt.rmnt_fstype	= fs;
	rmnt.rmnt_mntopts	= opt;
	rmnt.rmnt_time		= time_str;
	rmnt.rmnt_lvl		= lvl;

	Signal(SIGHUP,  SIG_IGN);
	Signal(SIGQUIT, SIG_IGN);
	Signal(SIGINT,  SIG_IGN);
	
	if ((rfp=fopen (RMNTTAB, "a")) == NULL) {
		perror(cmd);
		Fprintf(stderr, "%s: cannot open %s\n", cmd, RMNTTAB);
		return 0;
	}
	putrmntent(rfp, &rmnt);
	Fclose(rfp);

	Signal(SIGHUP,  SIG_DFL);
	Signal(SIGQUIT, SIG_DFL);
	Signal(SIGINT,  SIG_DFL);
	
	return ret;
}
