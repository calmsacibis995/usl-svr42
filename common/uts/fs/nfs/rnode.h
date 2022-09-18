/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_NFS_RNODE_H	/* wrapper symbol for kernel use */
#define _FS_NFS_RNODE_H	/* subject to change without notice */

#ident	"@(#)uts-comm:fs/nfs/rnode.h	1.6.2.2"
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

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _SVC_TIME_H
#include <svc/time.h>		/* REQUIRED */
#endif

#ifndef _FS_VNODE_H
#include <fs/vnode.h>		/* REQUIRED */
#endif

#ifndef _FS_NFS_NFS_H
#include <fs/nfs/nfs.h>		/* REQUIRED */
#endif

#ifndef _ACC_DAC_ACL_H
#include <acc/dac/acl.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/time.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <nfs/nfs.h>		/* REQUIRED */
#include <sys/acl.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Remote file information structure.
 * The rnode is the "inode" for remote files.  It contains all the
 * information necessary to handle remote file on the client side.
 */
struct rnode {
	struct rnode	*r_freef;	/* free list forward pointer */
	struct rnode	*r_freeb;	/* free list back pointer */
	struct rnode	*r_hash;	/* rnode hash chain */
	struct vnode	r_vnode;	/* vnode for remote file */
	fhandle_t	r_fh;		/* file handle */
	u_short		r_flags;	/* flags, see below */
	short		r_error;	/* async write error */
	union {
		daddr_t	R_nextr;	/* next byte read offset (read-ahead) */
		int	R_lastcookie;	/* last readdir cookie */
	} r_r;
#define		r_nextr	r_r.R_nextr
#define		r_lastcookie	r_r.R_lastcookie
	long		r_owner;	/* proc index for locker of rnode */
	long		r_count;	/* number of rnode locks for r_owner */
	struct cred	*r_cred;	/* current credentials */
	struct cred	*r_unlcred;	/* unlinked credentials */
	char		*r_unlname;	/* unlinked file name */
	struct vnode	*r_unldvp;	/* parent dir of unlinked file */
	struct vattr	r_attr;		/* cached vnode attributes */
	struct timeval	r_attrtime;	/* time attributes become invalid */
	struct acl	*r_acl;		/* file's ACL, if any */
	u_int		r_aclcnt;	/* number of entries in r_acl */
};

#define r_size	r_attr.va_size		/* file size in bytes */

/*
 * Flags
 */
#define	RLOCKED		0x01		/* rnode is in use */
#define	RWANT		0x02		/* someone wants a wakeup */
#define	RATTRVALID	0x04		/* Attributes in the rnode are valid */
#define	REOF		0x08		/* EOF encountered on read */
#define	RDIRTY		0x10		/* dirty pages from write operation */
#define RMLD		0x20		/* parent MLD */
#define REFFMLD		0x40		/* Child (effective) MLD */
#define RINACTIVE	0x80		/* rnode is becoming inactive */

/*
 * Convert between vnode and rnode
 */
#define	rtov(rp)	(&(rp)->r_vnode)
#define	vtor(vp)	((struct rnode *)((vp)->v_data))
#define	vtofh(vp)	(&(vtor(vp)->r_fh))
#define	rtofh(rp)	(&(rp)->r_fh)

#ifdef	SYSV
#define	rlock(rp)	rlockk(rp, __FILE__, __LINE__);
#define	runlock(rp)	runlockk(rp, __FILE__, __LINE__);

/*
 * Lock and unlock rnodes.
 */
/*
#define RLOCK(rp)	RLOCKK(rp, __FILE__, __LINE__)

#define	RLOCKK(rp, file, line) { \
printf("RLOCK: called from %s, lineno %d\n", file, line); \
	while (((rp)->r_flags & RLOCKED) && \
	    (rp)->r_owner != ((long) u.u_procp->p_pid)) { \
		(rp)->r_flags |= RWANT; \
printf("RLOCK: r_owner %x, masterprocp %x: sleeping\n", (rp)->r_owner, ((long) u.u_procp->p_pid)); \
		(void) sleep((caddr_t)(rp), PINOD); \
	} \
	(rp)->r_owner = ((long) u.u_procp->p_pid); \
	(rp)->r_count++; \
	(rp)->r_flags |= RLOCKED; \
	if (((rp)->r_vnode.v_flag & VISSWAP) != 0) \
		u.u_procp->p_swlocks++; \
}
*/
#define	RLOCK(rp) { \
	while (((rp)->r_flags & RLOCKED) && \
	    (rp)->r_owner != ((long) u.u_procp->p_pid)) { \
		(rp)->r_flags |= RWANT; \
		(void) sleep((caddr_t)(rp), PINOD); \
	} \
	(rp)->r_owner = ((long) u.u_procp->p_pid); \
	(rp)->r_count++; \
	(rp)->r_flags |= RLOCKED; \
	if (((rp)->r_vnode.v_flag & VISSWAP) != 0) \
		u.u_procp->p_swlocks++; \
}

/*
#define RUNLOCK(rp)	RUNLOCKK(rp, __FILE__, __LINE__)
#define	RUNLOCKK(rp, file, line) { \
printf("RUNLOCK: called from %s, lineno %d\n", file, line); \
	if (--(rp)->r_count < 0) \
		panic("RUNLOCK"); \
	if (((rp)->r_vnode.v_flag & VISSWAP) != 0) \
		u.u_procp->p_swlocks--; \
	if ((rp)->r_count == 0) { \
		(rp)->r_flags &= ~RLOCKED; \
		if ((rp)->r_flags & RWANT) { \
			(rp)->r_flags &= ~RWANT; \
			wakeprocs((caddr_t)(rp), PRMPT); \
		} \
	} \
}
*/
#define	RUNLOCK(rp) { \
	if (--(rp)->r_count < 0) \
		panic("RUNLOCK"); \
	if (((rp)->r_vnode.v_flag & VISSWAP) != 0) \
		u.u_procp->p_swlocks--; \
	if ((rp)->r_count == 0) { \
		(rp)->r_flags &= ~RLOCKED; \
		if ((rp)->r_flags & RWANT) { \
			(rp)->r_flags &= ~RWANT; \
			wakeprocs((caddr_t)(rp), PRMPT); \
		} \
	} \
}
#else
/*
 * Lock and unlock rnodes.
 */
#define	RLOCK(rp) { \
	while (((rp)->r_flags & RLOCKED) && \
	    (rp)->r_owner != uniqpid()) { \
		(rp)->r_flags |= RWANT; \
		(void) sleep((caddr_t)(rp), PINOD); \
	} \
	(rp)->r_owner = uniqpid(); \
	(rp)->r_count++; \
	(rp)->r_flags |= RLOCKED; \
	if (((rp)->r_vnode.v_flag & VISSWAP) != 0) \
		u.u_procp->p_swlocks++; \
}

#define	RUNLOCK(rp) { \
	if (--(rp)->r_count < 0) \
		panic("RUNLOCK"); \
	if (((rp)->r_vnode.v_flag & VISSWAP) != 0) \
		u.u_procp->p_swlocks--; \
	if ((rp)->r_count == 0) { \
		(rp)->r_flags &= ~RLOCKED; \
		if ((rp)->r_flags & RWANT) { \
			(rp)->r_flags &= ~RWANT; \
			wakeprocs((caddr_t)(rp), PRMPT); \
		} \
	} \
}
#endif
#ifdef _KERNEL

#ifdef __STDC__
void	rp_rmhash(struct rnode *);
void	rinactive(struct rnode *);
void	rfree(struct rnode *);
void	rinval(struct vfs *);
void	rflush(struct vfs *);
void	rlockk(struct rnode *, char *, int);
void	runlockk(struct rnode *, char *, int);
#else	/* __STDC__ */
void	rp_rmhash();
void	rinactive();
void	rfree();
void	rinval();
void	rflush();
void	rlockk();
void	runlockk();
#endif	/* __STDC__ */

#endif /* _KERNEL */

#endif	/* _FS_NFS_RNODE_H */
