/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/nfs/nfssys.c	1.3.2.2"
#ident	"$Header: $"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
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

#include <util/types.h>
#include <net/rpc/types.h>
#include <svc/systm.h>
#include <fs/vfs.h>
#include <svc/errno.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/export.h>
#include <fs/nfs/nfssys.h>

extern int async_daemon_ready, async_daemon_count, nfs_server_count;

/*ARGSUSED*/
int
nfssys(register struct nfssysa *uap, rval_t *rvp)
{
	switch ((int) uap->opcode) {

		/*
		 * NFS server daemon entry
		 */
		case NFS_SVC:
			{
				struct nfs_svc_args    nsa;

				if (copyin((caddr_t) uap->nfssysarg_svc,
					   (caddr_t) &nsa, sizeof(nsa)))
					return(EFAULT);
				else
					return(nfs_svc(&nsa));
			}

		/*
		 * NFS client async daemon entry
		 */
		case ASYNC_DAEMON:
			return(async_daemon());

		/*
		 * export a file system
		 */
		case EXPORTFS:
			{
				struct exportfs_args    ea;

				if (copyin((caddr_t) uap->nfssysarg_exportfs,
					   (caddr_t) &ea, sizeof(ea)))
					return(EFAULT);
				else
					return(exportfs(&ea));
			}

		/*
		 * get a file handle
		 */
		case NFS_GETFH:
			{
				struct nfs_getfh_args    nga;

				if (copyin((caddr_t) uap->nfssysarg_getfh,
					   (caddr_t) &nga, sizeof(nga)))
					return(EFAULT);
				else
					return(nfs_getfh(&nga));
			}

		/*
		 * open a file referred to by a
		 * file handle
		 */
		case NFS_CNVT:
			return (nfs_cnvt(uap->nfssysarg_cnvt, rvp));

		/*
		 * load the contents of file containing lid
		 * and priv info
		 */
		case NFS_LOADLP:
			{
				struct nfs_loadlp_args	llpa;

				if (copyin((caddr_t) uap->nfssysarg_loadlp,
					   (caddr_t) &llpa, sizeof(llpa)))
					return(EFAULT);
				else
					return(setnfslp(llpa.buf, llpa.size,
						llpa.deflid, llpa.defpriv));
			}

		/*
		 * do any async_daemons (biods) exist ?
		 */
		case NFS_ISBIODRUN:
			if (async_daemon_count)
				return (0);
			else
				return(ENOENT);

		/*
		 * do any nfs daemons (nfsds) exist ?
		 */
		case NFS_ISNFSDRUN:
			if (nfs_server_count)
				return (0);
			else
				return(ENOENT);

		default:
			return(EINVAL);
	}
}
