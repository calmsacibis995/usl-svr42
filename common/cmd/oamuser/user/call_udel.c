/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/call_udel.c	1.1.7.2"
#ident  "$Header: call_udel.c 2.0 91/07/13 $"

#include	<sys/types.h>
#include	<stdio.h>
#include	<userdefs.h>

extern	pid_t	fork(),
		wait();

extern	int	execvp();

extern	void	exit();

/*
 * Procedure:	call_udelete
 *
 * Restrictions:
 *		freopen:	None
 *		execl:		None
*/

int
call_udelete(nargv)
	char	*nargv;
{
	int	ret;
	char	*cmd = "/usr/sbin/userdel";

	switch (fork()) {
	case 0:
		/* CHILD */
		if (freopen("/dev/null", "w+", stdout) == NULL
			|| freopen("/dev/null", "w+", stderr) == NULL
			|| execl(cmd, cmd, nargv, (char *) NULL) == -1)
			exit(EX_FAILURE);
		break;
	case -1:
		/* ERROR */
		return EX_FAILURE;
	default:
		/* PARENT */	
		if (wait(&ret) == -1)
			return EX_FAILURE;
		ret = (ret >> 8) & 0xff;
	}
	return ret;
		
}
