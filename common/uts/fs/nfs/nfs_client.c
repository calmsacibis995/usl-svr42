/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/nfs/nfs_client.c	1.8.2.2"
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

#include <util/param.h>
#include <util/types.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <fs/vnode.h>
#include <fs/vfs.h>
#include <acc/dac/acl.h>
#include <svc/errno.h>
#include <fs/buf.h>
#include <fs/stat.h>
#include <proc/cred.h>
#include <mem/kmem.h>
#include <mem/pvn.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/transport/tiuser.h>
#include <util/sysmacros.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/nfs_clnt.h>
#include <fs/nfs/rnode.h>

/*
 * Attributes caching:
 *
 * Attributes are cached in the rnode in struct vattr form.
 * There is a time associated with the cached attributes (r_attrtime)
 * which tells whether the attributes are valid. The time is initialized
 * to the difference between current time and the modify time of the vnode
 * when new attributes are cached. This allows the attributes for
 * files that have changed recently to be timed out sooner than for files
 * that have not changed for a long time. There are minimum and maximum
 * timeout values that can be set per mount point.
 */

/*
 * Validate caches by checking cached attributes. If they have timed out
 * get the attributes from the server and compare mtimes. If mtimes are
 * different purge all caches for this vnode.
 */
int
nfs_validate_caches(vp, cred)
	struct vnode *vp;
	struct cred *cred;
{
	struct vattr va;

	return (nfsgetattr(vp, &va, cred));
}

void
nfs_purge_caches(vp)
	struct vnode *vp;
{
	struct rnode *rp;

	/* LINTED pointer alignment */
	rp = vtor(vp);
	NFSLOG(8, "nfs_purge_caches: rnode %x\n", rp, 0);
	/* LINTED pointer alignment */
	INVAL_ATTRCACHE(vp);
	dnlc_purge_vp(vp);
	(void) VOP_PUTPAGE(vp, 0, 0, B_INVAL, rp->r_cred);
}

/*
 * Flush the attribute and data caches if the attribute cache is not valid.
 * NOTE: "not valid" now includes mi_noac being set
 */
void
nfs_cache_check(vp, mtime)
	struct vnode *vp;
	timestruc_t mtime;
{
	/* LINTED pointer alignment */
	if (!CACHE_VALID(vtor(vp), mtime) || vtomi(vp)->mi_noac) {
		nfs_purge_caches(vp);
	}
}

/*
 * Set attributes cache for given vnode using nfsfattr.
 */
void
nfs_attrcache(vp, na)
	struct vnode *vp;
	struct nfsfattr *na;
{

#ifdef	VNOCACHE
	/* LINTED pointer alignment */
	if ((vp->v_flag & VNOCACHE) || vtomi(vp)->mi_noac)
#else
	/* LINTED pointer alignment */
	if (vtomi(vp)->mi_noac)
#endif
		return;
	/* LINTED pointer alignment */
	nattr_to_vattr(vp, na, &vtor(vp)->r_attr);
	set_attrcache_time(vp);
}

/*
 * Set attributes cache for given vnode using nfsesvfattr.
 */
void
nfs_esvattrcache(vp, na)
	struct vnode *vp;
	struct nfsesvfattr *na;
{
	lid_t tmplid;
	/* LINTED pointer alignment */
	struct rnode *rp = vtor(vp);

#ifdef	VNOCACHE
	/* LINTED pointer alignment */
	if ((vp->v_flag & VNOCACHE) || vtomi(vp)->mi_noac)
#else
	/* LINTED pointer alignment */
	if (vtomi(vp)->mi_noac)
#endif
		return;
	rp->r_aclcnt = acl_getmax();
	esvnattr_to_vattr(vp, na, &rp->r_attr, &tmplid, rp->r_acl, &rp->r_aclcnt);
	if (tmplid != vp->v_lid) {
		NFSLOG(1, "nfs_esvattrcache: new LID %d != old LID %d\n",
		       tmplid, vp->v_lid);
		vp->v_lid = tmplid;
	}
	set_attrcache_time(vp);
}

/*
 * Set attributes cache for given vnode using vnode attributes.
 */
void
nfs_attrcache_va(vp, va)
	struct vnode *vp;
	struct vattr *va;
{

#ifdef	VNOCACHE
	/* LINTED pointer alignment */
	if ((vp->v_flag & VNOCACHE) || vtomi(vp)->mi_noac)
#else
	/* LINTED pointer alignment */
	if (vtomi(vp)->mi_noac)
#endif
		return;
	/* LINTED pointer alignment */
	vtor(vp)->r_attr = *va;
	vp->v_type = va->va_type;
	set_attrcache_time(vp);
}

void
set_attrcache_time(vp)
	struct vnode *vp;
{
	struct rnode *rp;
	int delta;

	/* LINTED pointer alignment */
	rp = vtor(vp);
	rp->r_attrtime.tv_sec = hrestime.tv_sec;
	rp->r_attrtime.tv_usec = 0;
	/*
	 * Delta is the number of seconds that we will cache
	 * attributes of the file.  It is based on the number of seconds
	 * since the last change (i.e. files that changed recently
	 * are likely to change soon), but there is a minimum and
	 * a maximum for regular files and for directories.
	 */
	delta = (hrestime.tv_sec - rp->r_attr.va_mtime.tv_sec) >> 4;
	if (vp->v_type == VDIR) {
		/* LINTED pointer alignment */
		if (delta < vtomi(vp)->mi_acdirmin) {
			/* LINTED pointer alignment */
			delta = vtomi(vp)->mi_acdirmin;
		/* LINTED pointer alignment */
		} else if (delta > vtomi(vp)->mi_acdirmax) {
			/* LINTED pointer alignment */
			delta = vtomi(vp)->mi_acdirmax;
		}
	} else {
		/* LINTED pointer alignment */
		if (delta < vtomi(vp)->mi_acregmin) {
			/* LINTED pointer alignment */
			delta = vtomi(vp)->mi_acregmin;
		/* LINTED pointer alignment */
		} else if (delta > vtomi(vp)->mi_acregmax) {
			/* LINTED pointer alignment */
			delta = vtomi(vp)->mi_acregmax;
		}
	}
	rp->r_attrtime.tv_sec += delta;
}

/*
 * Fill in attribute from the cache. If valid return 1 otherwise 0;
 */
int
nfs_getattr_cache(vp, vap)
	struct vnode *vp;
	struct vattr *vap;
{
	struct rnode *rp;

	/* LINTED pointer alignment */
	rp = vtor(vp);
	if (hrestime.tv_sec < rp->r_attrtime.tv_sec) {
		/*
		 * Cached attributes are valid
		 */
		*vap = rp->r_attr;
		return (1);
	}
	return (0);
}

/*
 * Get attributes over-the-wire.
 * Return 0 if successful, otherwise error.
 */
int
nfs_getattr_otw(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct cred *cred;
{
	int error;
	struct nfsattrstat *ns;

	ns = (struct nfsattrstat *)kmem_zalloc(sizeof (*ns), KM_SLEEP);

	/* LINTED pointer alignment */
	error = rfscall(vtomi(vp), RFS_GETATTR, 0, xdr_fhandle,
	    /* LINTED pointer alignment */
	    (caddr_t)vtofh(vp), xdr_attrstat, (caddr_t)ns, cred);

	if (error == 0) {
		error = geterrno(ns->ns_status);
		if (error == 0) {
			nattr_to_vattr(vp, &ns->ns_attr, vap);
			nfs_cache_check(vp, vap->va_mtime);
			nfs_attrcache_va(vp, vap);
		} else {
			PURGE_STALE_FH(error, vp);
		}
	}
	kmem_free((caddr_t)ns, sizeof (*ns));

	return (error);
}

/*
 * Get extended attributes over-the-wire.
 * Return 0 if successful, otherwise error.
 */
int
nfs_getesvattr_otw(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct cred *cred;
{
	int error;
	struct nfsesvattrstat *ns;
	lid_t tmplid;

	ns = (struct nfsesvattrstat *)kmem_alloc(sizeof (*ns), KM_SLEEP);

	/* LINTED pointer alignment */
	error = rfscall(vtomi(vp), RFS_GETATTR, 0, xdr_fhandle,
	    /* LINTED pointer alignment */
	    (caddr_t)vtofh(vp), xdr_esvattrstat, (caddr_t)ns, cred);

	if (error == 0) {
		error = geterrno(ns->ns_status);
		if (error == 0) {
			/* LINTED pointer alignment */
			vtor(vp)->r_aclcnt = acl_getmax();
			esvnattr_to_vattr(vp, &ns->ns_attr, vap, &tmplid,
					/* LINTED pointer alignment */
					vtor(vp)->r_acl, &vtor(vp)->r_aclcnt);
			if (tmplid != vp->v_lid) {
				NFSLOG(1, "nfs_getesvattr_otw: new LID %d != old LID %d\n",
				       tmplid, vp->v_lid);
				vp->v_lid = tmplid;
			}
			nfs_cache_check(vp, vap->va_mtime);
			nfs_attrcache_va(vp, vap);
		} else {
			PURGE_STALE_FH(error, vp);
		}
	}
	kmem_free((caddr_t)ns, sizeof (*ns));

	return (error);
}

/*
 * Return either cached or remote attributes. If get remote attr
 * use them to check and invalidate caches, then cache the new attributes.
 */
int
nfsgetattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct cred *cred;
{
	int error;

	if (nfs_getattr_cache(vp, vap)) {
		/*
		 * got cached attributes, we're done.
		 */
		return (0);
	}

	/* LINTED pointer alignment */
	if (vtomi(vp)->mi_protocol == NFS_V2)
		error = nfs_getattr_otw(vp, vap, cred);
	else
		error = nfs_getesvattr_otw(vp, vap, cred);
	return (error);
}

void
nattr_to_vattr(vp, na, vap)
	register struct vnode *vp;
	register struct nfsfattr *na;
	register struct vattr *vap;
{
	struct rnode *rp;

	/* LINTED pointer alignment */
	rp = vtor(vp);
	vap->va_type = (enum vtype)na->na_type;
	vap->va_mode = na->na_mode;
	vap->va_uid = na->na_uid;
	vap->va_gid = na->na_gid;
	/* fsid must be negative, so OR with 0x80000000 */
	/* LINTED pointer alignment */
	vap->va_fsid = 0x80000000 | (long)makedevice(vfs_fixedmajor(vp->v_vfsp), vtomi(vp)->mi_mntno);
	vap->va_nodeid = na->na_nodeid;
	vap->va_nlink = na->na_nlink;
	if (rp->r_size < na->na_size || ((rp->r_flags & RDIRTY) == 0)){
		rp->r_size = vap->va_size = na->na_size;
	} else {
		vap->va_size = rp->r_size;
	}
	vap->va_atime.tv_sec  = na->na_atime.tv_sec;
	vap->va_atime.tv_nsec = na->na_atime.tv_usec*1000;
	vap->va_mtime.tv_sec  = na->na_mtime.tv_sec;
	vap->va_mtime.tv_nsec = na->na_mtime.tv_usec*1000;
	vap->va_ctime.tv_sec  = na->na_ctime.tv_sec;
	vap->va_ctime.tv_nsec = na->na_ctime.tv_usec*1000;
	vap->va_rdev = na->na_rdev;
	vap->va_nblocks = na->na_blocks;
	switch(na->na_type) {

	case NFBLK:
		vap->va_blksize = DEV_BSIZE;
		break;

	case NFCHR:
		vap->va_blksize = MAXBSIZE;
		break;

	default:
		vap->va_blksize = na->na_blocksize;
		break;
	}
	/*
	 * This bit of ugliness is a *TEMPORARY* hack to preserve the
	 * over-the-wire protocols for named-pipe vnodes.  It remaps the
	 * special over-the-wire type to the VFIFO type. (see note in nfs.h)
	 *
	 * BUYER BEWARE:
	 *  If you are porting the NFS to a non-SUN server, you probably
	 *  don't want to include the following block of code.  The
	 *  over-the-wire special file types will be changing with the
	 *  NFS Protocol Revision.
	 */
	if (NA_ISFIFO(na)) {
		vap->va_type = VFIFO;
		vap->va_mode = (vap->va_mode & ~S_IFMT) | S_IFIFO;
		vap->va_rdev = 0;
		vap->va_blksize = na->na_blocksize;
	}
}

void
esvnattr_to_vattr(vp, na, vap, lidp, aclp, nacl)
	register struct vnode *vp;
	register struct nfsesvfattr *na;
	register struct vattr *vap;
	register lid_t *lidp;
	register struct acl *aclp;	/* ptr to area to write ACL entries */
	register u_int *nacl;		/* Number of ACL entries that will fit.
					 * Will be overwritten with the # of ACL
					 * entries actually copied
					 */
{
	struct rnode *rp;

	/* LINTED pointer alignment */
	rp = vtor(vp);
	vap->va_type = (enum vtype)na->na_type;
	vap->va_mode = na->na_mode;
	vap->va_uid = na->na_uid;
	vap->va_gid = na->na_gid;
	/* fsid must be negative, so OR with 0x80000000 */
	/* LINTED pointer alignment */
	vap->va_fsid = 0x80000000 | (long)makedevice(vfs_fixedmajor(vp->v_vfsp), vtomi(vp)->mi_mntno);
	vap->va_nodeid = na->na_nodeid;
	vap->va_nlink = na->na_nlink;
	if (rp->r_size < na->na_size || ((rp->r_flags & RDIRTY) == 0)){
		rp->r_size = vap->va_size = na->na_size;
	} else {
		vap->va_size = rp->r_size;
	}
	vap->va_atime.tv_sec  = na->na_atime.tv_sec;
	vap->va_atime.tv_nsec = na->na_atime.tv_usec*1000;
	vap->va_mtime.tv_sec  = na->na_mtime.tv_sec;
	vap->va_mtime.tv_nsec = na->na_mtime.tv_usec*1000;
	vap->va_ctime.tv_sec  = na->na_ctime.tv_sec;
	vap->va_ctime.tv_nsec = na->na_ctime.tv_usec*1000;
	vap->va_rdev = na->na_rdev;		/* NA_TSTMLD will still work */
	vap->va_nblocks = na->na_blocks;
	switch(na->na_type) {

	case NFBLK:
		vap->va_blksize = DEV_BSIZE;
		break;

	case NFCHR:
		vap->va_blksize = MAXBSIZE;
		break;

	default:
		vap->va_blksize = na->na_blocksize;
		break;
	}
	/*
	 * This bit of ugliness is a *TEMPORARY* hack to preserve the
	 * over-the-wire protocols for named-pipe vnodes.  It remaps the
	 * special over-the-wire type to the VFIFO type. (see note in nfs.h)
	 *
	 * BUYER BEWARE:
	 *  If you are porting the NFS to a non-SUN server, you probably
	 *  don't want to include the following block of code.  The
	 *  over-the-wire special file types will be changing with the
	 *  NFS Protocol Revision.
	 */
	if (NA_ISFIFO(na)) {
		vap->va_type = VFIFO;
		vap->va_mode = (vap->va_mode & ~S_IFMT) | S_IFIFO;
		vap->va_rdev = 0;
		vap->va_blksize = na->na_blocksize;
	}

	if (map_local_token(na->na_sens, SENS_T, (caddr_t)lidp, sizeof(lid_t)) != sizeof(lid_t))
		vp->v_lid = (lid_t)0;
	*nacl = map_local_token(na->na_acl, ACL_T, (caddr_t)aclp, *nacl * sizeof (struct acl));
	*nacl /= sizeof(struct acl);
}
