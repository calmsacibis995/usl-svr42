/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_NFS_NFSSYS_H	/* wrapper symbol for kernel use */
#define _FS_NFS_NFSSYS_H	/* subject to change without notice */

#ident	"@(#)uts-comm:fs/nfs/nfssys.h	1.4.2.5"
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

/*
 * Definitions for nfssys system call.
 * Note: <sys/nfs/export.h> and <sys/nfs/nfs.h> must be included before
 * this file.
 */

#ifdef _KERNEL_HEADERS

#ifndef _FS_NFS_NFS_H
#include <fs/nfs/nfs.h>		/* REQUIRED */
#endif

#ifndef _FS_NFS_EXPORT_H
#include <fs/nfs/export.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <nfs/nfs.h>		/* REQUIRED */
#include <nfs/export.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

enum nfssys_op	{ NFS_SVC, ASYNC_DAEMON, EXPORTFS, NFS_GETFH,
		NFS_CNVT, NFS_LOADLP, NFS_ISBIODRUN, NFS_ISNFSDRUN };

struct nfs_svc_args {
	int	fd;
};

struct exportfs_args {
	char		*dname;
	struct export	*uex;
};

struct nfs_getfh_args {
	char		*fname;
	fhandle_t	*fhp;
};

struct nfs_cnvt_args {
	fhandle_t	*fh;
	int		filemode;
	int		*fd;
};

struct nfs_loadlp_args {
	int		size;
	struct nfslpbuf	*buf;
	lid_t		deflid;
	lid_t		esvdeflid;
	pvec_t		defpriv;
};

#ifdef _KERNEL

/*
 * there are no asynch_daemon_args, isbiodrun_args and
 * isnfsdrun_args.
 */
union nfssysargs {
	struct exportfs_args		*exportfs_args_u;	/* exportfs args */
	struct nfs_getfh_args		*nfs_getfh_args_u;	/* nfs_getfh args */
	struct nfs_svc_args		*nfs_svc_args_u;	/* nfs_svc args */
	struct nfs_cnvt_args		*nfs_cnvt_args_u;	/* nfs_cnvt args */
	struct nfs_loadlp_args		*nfs_loadlp_args_u;	/* nfs_loadlp args */
};

struct nfssysa {
	enum nfssys_op		opcode;	/* operation discriminator */
	union nfssysargs	arg;	/* syscall-specific arg pointer */
#define	nfssysarg_exportfs	arg.exportfs_args_u
#define	nfssysarg_getfh		arg.nfs_getfh_args_u
#define	nfssysarg_svc		arg.nfs_svc_args_u
#define nfssysarg_cnvt		arg.nfs_cnvt_args_u
#define nfssysarg_loadlp	arg.nfs_loadlp_args_u
};

#endif

#endif	/* _FS_NFS_NFSSYS_H */
