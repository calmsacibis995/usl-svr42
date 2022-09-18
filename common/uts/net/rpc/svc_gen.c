/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/rpc/svc_gen.c	1.5.2.3"
#ident	"$Header: $"
/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

/*
 * svc_generic.c,
 * Server side for RPC in the kernel.
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <net/rpc/types.h>
#include <net/tcpip/in.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/transport/tiuser.h>
#include <net/ktli/t_kuser.h>
#include <net/rpc/svc.h>
#include <fs/file.h>
#include <proc/user.h>
#include <io/stream.h>
#include <net/transport/tihdr.h>
#include <fs/fcntl.h>
#include <svc/errno.h>
 
int
svc_tli_kcreate(fp, sendsz, nxprt)
	register struct file	*fp;		/* connection end point */
	u_int			sendsz;		/* max sendsize */
	SVCXPRT			**nxprt;
{
	SVCXPRT			*xprt = NULL;		/* service handle */
	TIUSER			*tiptr = NULL;
	int			error;

	error = 0;

	if (fp == NULL || nxprt == NULL)
		return EINVAL;

	if ((error = t_kopen(fp, -1, FREAD|FWRITE|FNDELAY, &tiptr)) != 0) {
                RPCLOG(1, "svc_tli_kcreate: t_kopen: %d\n", error);
         	return error;
	}

	/*
	 * call transport specific function.
	 */
	switch(tiptr->tp_info.servtype) {
		case T_CLTS:
			error = svc_clts_kcreate(tiptr, sendsz, &xprt);
			break;
		default:
                	RPCLOG(1, "svc_tli_kcreate: Bad service type %d\n", 
				tiptr->tp_info.servtype);
			error = EINVAL;
			break;
        }
	if (error != 0)
		goto freedata;

	(void)poptimod(tiptr->fp->f_vnode);

	xprt->xp_port = (u_short)-1;	/* To show that it is tli based. Switch */

	*nxprt = xprt;
	return 0;

freedata:
	if (xprt)
		SVC_DESTROY(xprt);
	return error;
}



