/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_NFS_EXPORT_H	/* wrapper symbol for kernel use */
#define _FS_NFS_EXPORT_H	/* subject to change without notice */

#ident	"@(#)uts-comm:fs/nfs/export.h	1.5.2.3"
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

#ifdef _KERNEL_HEADERS

#ifndef _NET_TRANSPORT_TIUSER_H
#include <net/transport/tiuser.h>	/* REQUIRED */
#endif

#ifndef _FS_VFS_H
#include <fs/vfs.h>			/* REQUIRED */
#endif

#ifndef _PROC_CRED_H
#include <proc/cred.h>
#endif

#elif defined(_KERNEL)

#include <sys/tiuser.h>			/* REQUIRED */
#include <sys/vfs.h>			/* REQUIRED */
#include <sys/cred.h>

#endif /* _KERNEL_HEADERS */

/*
 * exported vfs flags.
 */

#define	EX_RDONLY	0x001	/* exported read only */
#define	EX_RDMOSTLY	0x002	/* exported read mostly */
#define	EX_RDWR		0x004	/* exported read-write */
#define	EX_EXCEPTIONS	0x008	/* exported with ``exceptions'' lists */
#define	EX_ALL		(EX_RDONLY | EX_RDMOSTLY | EX_RDWR | EX_EXCEPTIONS)

#define	EXMAXADDRS	256	/* max number in address list */
struct exaddrlist {
	unsigned naddrs;		/* number of addresses */
	struct netbuf *addrvec;		/* pointer to array of addresses */
	struct netbuf *addrmask;	/* mask of comparable bits of addrvec */
};

/*
 * Associated with AUTH_UNIX is an array of internet addresses
 * to check root permission.
 */
#define	EXMAXROOTADDRS	256		/* should be config option */
struct unixexport {
	struct exaddrlist rootaddrs;
};

/*
 * Associated with AUTH_DES is a list of network names to check
 * root permission, plus a time window to check for expired
 * credentials.
 */
#define	EXMAXROOTNAMES	256		/* should be config option */
struct desexport {
	unsigned nnames;
	char **rootnames;
	int window;
};

/*
 * Associated with AUTH_ESV is an array of machine addresses
 * to check root permission.
 */
#define	EXMAXESVROOTADDRS	256	/* should be config option */
struct esvexport {
	struct exaddrlist		 esvrootaddrs;	/* root clients */
};

/*
 * The structure to load the per-host security information from
 * /etc/lid_and_priv
 */
struct nfslpbuf {
	_VOID		*dummy;
	struct netbuf	*addr;
	struct netbuf	*mask;
	lid_t		lid;
	lid_t		esvlid;
	pvec_t		priv;
};

/*
 * The export information passed to exportfs()
 */
struct export {
	int		ex_flags;	/* flags */
	unsigned	ex_anon;	/* uid for unauthenticated requests */
	int		ex_auth;	/* switch */
	union {
		struct unixexport	exunix;		/* case AUTH_UNIX */
		struct desexport	exdes;		/* case AUTH_DES */
		struct esvexport	exesv;		/* case AUTH_ESV */
	} ex_u;
	struct exaddrlist ex_roaddrs;
	struct exaddrlist ex_rwaddrs;
};
#define	ex_des	ex_u.exdes
#define	ex_unix	ex_u.exunix
#define ex_esv	ex_u.exesv

#ifdef	_KERNEL

/*
 * A node associated with an export entry on the list of exported
 * filesystems.
 */
struct exportinfo {
	struct export		exi_export;
	fsid_t			exi_fsid;
	struct fid		*exi_fid;
	struct exportinfo	*exi_next;
};

/* external routines associated with exports */
#ifdef __STDC__
struct exportinfo *findexport(fsid_t *, struct fid *);
int	setnfslp(struct nfslpbuf *, u_int, lid_t, pvec_t);
void	applynfslp(struct netbuf *, struct cred *, u_int);
#else
struct exportinfo *findexport();
int	setnfslp();
void	applynfslp();
#endif

#endif	/* _KERNEL */

#endif	/* _FS_NFS_EXPORT_H */
