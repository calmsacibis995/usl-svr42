/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/meth.d/bk_system.c	1.5.5.2"
#ident  "$Header: bk_system.c 1.2 91/06/21 $"

#include	<sys/types.h>
#include	<signal.h>
#include	<backup.h>

extern pid_t fork();
extern int	execl();
extern pid_t wait();
extern void _exit();

extern int	bklevels;

static char	bin_shell[] = "/sbin/sh";
static char	shell[] = "sh";
static char	shflg[]= "-c";

int
bk_system(s)
char	*s;
{
	int	status;
	pid_t	pid;
	int	w;

	if ((pid = fork()) == 0) {
		(void) execl(bin_shell, shell, shflg, s, (char *)0);
		_exit(127);
	}
	BEGIN_CRITICAL_REGION;

	while ((w = wait(&status)) != pid && w != -1)
		;

	BEGIN_CRITICAL_REGION;

	return((w == -1)? w: status);
} /* bk_system() */
