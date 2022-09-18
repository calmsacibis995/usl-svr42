/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/rpc/rpc_subr.c	1.7.2.3"
#ident	"$Header: $"
/*
 * Miscellaneous support routines for kernel implementation of RPC.
 *
 */

#include	<util/param.h>
#include	<util/types.h>
#include	<net/rpc/types.h>
#include	<fs/vnode.h>
#include	<io/stream.h>
#include	<io/stropts.h>
#include	<io/strsubr.h>
#include	<proc/cred.h>
#include	<proc/proc.h>
#include	<proc/user.h>

#ifdef DEBUG
/*
 * Kernel level debugging aid. The global variable "rpclog" is a bit
 * mask which allows various types of debugging messages to be printed
 * out.
 * 
 *	rpclog & 1 	will cause actual failures to be printed.
 *	rpclog & 2	will cause informational messages to be
 *			printed on the client side of rpc.
 *	rpclog & 4	will cause informational messages to be
 *			printed on the server side of rpc.
 */

int rpclog = 0;

int
rpc_log(level, str, a1)
	ulong		level;
	register char	*str;
	register int	a1;

{
	if (level & rpclog)
		printf(str, a1);
	return(0);
}

#endif /* DEBUG */

/* pop TIMOD off the stream */
void
poptimod(vp)
struct vnode *vp;
{
	int error, isfound, ret;

	error = strioctl(vp, I_FIND, "timod", 0, K_TO_K, u.u_cred, &isfound);
	if (error) {
		RPCLOG((ulong)1, "poptimod: I_FIND strioctl error %d\n", error);
		return;
	}
	if (isfound != 0) {
		error = strioctl(vp, I_POP, "timod", 0, K_TO_K, u.u_cred, &ret);
		if (error)
			RPCLOG((ulong)1, "poptimod: I_POP strioctl error %d\n", error);
	}
}
