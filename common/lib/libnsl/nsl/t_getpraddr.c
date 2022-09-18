/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * This is the implementation of the proposed XPG4 routine.
 */

#ident "$Header: t_getpraddr.c 2.1 90/11/16 $"
#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_getpraddr.c	1.2"
#include "sys/errno.h"
#include "sys/types.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/tihdr.h"
#include "sys/tiuser.h"
#include "sys/timod.h"
#include "sys/signal.h"
#include "_import.h"


extern int t_errno;
extern int errno;
extern void (*sigset())();
extern int ioctl();

t_getprotaddr(fd, boundaddr, peeraddr)
	int fd;
	struct t_bind *boundaddr;
	struct t_bind *peeraddr;
{
	void (*sigsave)();
	int error = 0;

	if (_t_checkfd(fd) == 0)
		return(-1);

	sigsave = sigset(SIGPOLL, SIG_HOLD);

	if (boundaddr) {
		if (ioctl(fd, TI_GETMYNAME, &boundaddr->addr) < 0) {
			boundaddr->addr.len = 0;
			error = -1;
		}
	}
	if (peeraddr) {
		if (ioctl(fd, TI_GETPEERNAME, &peeraddr->addr) < 0) {
			peeraddr->addr.len = 0;
			error = -1;
		}
	}

out:
	sigset(SIGPOLL, sigsave);
	return(error);
}
