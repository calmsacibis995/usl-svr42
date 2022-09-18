/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/nfs/nfs_vnops.c	1.22.3.9"
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

#include <acc/dac/acl.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dirent.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/fs_subr.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/nfs_clnt.h>
#include <fs/nfs/rnode.h>
#include <fs/pathname.h>
#include <fs/specfs/snode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/rm.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_vn.h>
#include <mem/swap.h>
#include <mem/vmmeter.h>
#include <net/lockmgr/lockmgr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/transport/tiuser.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifdef __STDC__
struct vnode	*dnlc_lookup(vnode_t *, char *, cred_t *);
void		dnlc_enter(vnode_t *, char *, vnode_t *, cred_t *);
void		dnlc_purge_vp(vnode_t *);
void		dnlc_remove(vnode_t *, char *);
char		*newname(void);
int		setdirgid(struct vnode *);
u_int		setdirmode(struct vnode *, u_int);
STATIC	void	printfhandle(caddr_t);
STATIC	int	rwvp(struct vnode *, struct uio *, enum uio_rw, int, struct cred *);
STATIC	int	nfs_rdwr(struct vnode *, struct uio *, enum uio_rw, int, struct cred *);
STATIC	int	nfs_oldlookup(struct vnode *, char *, struct vnode **, struct cred *, struct pathname *, int);
STATIC	int	nfs_strategy(struct buf *);
STATIC	int	do_bio(struct buf *);
STATIC	int	nfs_lockrelease(struct vnode *, int, off_t, struct cred *);
#else
struct vnode	*dnlc_lookup();
void		dnlc_enter();
void		dnlc_purge_vp();
void		dnlc_remove();
char		*newname();
int		setdirgid();
u_int		setdirmode();
STATIC	void	printfhandle();
STATIC	int	rwvp();
STATIC	int	nfs_rdwr();
STATIC	int	nfs_oldlookup();
STATIC	int	nfs_strategy();
STATIC	int	do_bio();
STATIC	int	nfs_lockrelease();
#endif /* __STDC__ */

/*
 * Do close to open consistancy checking on all filesystems.
 * If this boolean is false, CTO checking can be selectively
 * turned off by setting actimeo to -1 at mount time.
 */
int nfs_cto = 1;

/*
 * Error flags used to pass information about certain special errors
 * back from do_bio() to nfs_getapage() (yuck).
 */
#define	NFS_CACHEINVALERR	-99
#define	NFS_EOF			-98

#define ISVDEV(t) ((t == VBLK) || (t == VCHR) || (t == VFIFO) || (t == VXNAM))

/*
 * These are the vnode ops routines which implement the vnode interface to
 * the networked file system.  These routines just take their parameters,
 * make them look networkish by putting the right info into interface structs,
 * and then calling the appropriate remote routine(s) to do the work.
 *
 * Note on directory name lookup cacheing:  we desire that all operations
 * on a given client machine come out the same with or without the cache.
 * To correctly do this, we serialize all operations on a given directory,
 * by using RLOCK and RUNLOCK around rfscalls to the server.  This way,
 * we cannot get into races with ourself that would cause invalid information
 * in the cache.  Other clients (or the server itself) can cause our
 * cached information to become invalid, the same as with data pages.
 * Also, if we do detect a stale fhandle, we purge the directory cache
 * relative to that vnode.  This way, the user won't get burned by the
 * cache repeatedly.
 */

STATIC	int nfs_open();
STATIC	int nfs_close();
STATIC	int nfs_read();
STATIC	int nfs_write();
STATIC	int nfs_ioctl();
STATIC	int nfs_getattr();
STATIC	int nfs_setattr();
STATIC	int nfs_access();
STATIC	int nfs_lookup();
STATIC	int nfs_create();
STATIC	int nfs_remove();
STATIC	int nfs_link();
STATIC	int nfs_rename();
STATIC	int nfs_mkdir();
STATIC	int nfs_rmdir();
STATIC	int nfs_readdir();
STATIC	int nfs_symlink();
STATIC	int nfs_readlink();
STATIC	int nfs_fsync();
STATIC	void nfs_inactive();
STATIC	int nfs_fid();
STATIC	void nfs_rwlock();
STATIC	void nfs_rwunlock();
STATIC	int nfs_seek();
extern  int fs_cmp();
STATIC  int nfs_frlock();
STATIC  int nfs_space();

extern	int fs_nosys();

STATIC  int nfs_realvp();
STATIC	int nfs_getpage();
STATIC	int nfs_putpage();
STATIC	int nfs_map();
STATIC	int nfs_addmap();
STATIC	int nfs_delmap();
STATIC	int nfs_allocstore();
extern	int fs_poll();
STATIC	int nfs_setlevel();
STATIC	int nfs_makemld();

struct vnodeops nfs_vnodeops = {
	nfs_open,
	nfs_close,
	nfs_read,
	nfs_write,
	nfs_ioctl,
	fs_setfl,
	nfs_getattr,
	nfs_setattr,
	nfs_access,
	nfs_lookup,
	nfs_create,
	nfs_remove,
	nfs_link,
	nfs_rename,
	nfs_mkdir,
	nfs_rmdir,
	nfs_readdir,
	nfs_symlink,
	nfs_readlink,
	nfs_fsync,
	nfs_inactive,
	nfs_fid,
	nfs_rwlock,
	nfs_rwunlock,
	nfs_seek,
	fs_cmp,
	nfs_frlock,
	nfs_space,
	nfs_realvp,
	nfs_getpage,
	nfs_putpage,
	nfs_map,
	nfs_addmap,
	nfs_delmap,
	fs_poll,
	fs_nosys,	/* not used */
	fs_pathconf,
	nfs_allocstore,
	fs_nosys,	/* getacl */
	fs_nosys,	/* getaclcnt */
	fs_nosys,	/* setacl */
	nfs_setlevel,
	fs_nosys,	/* getdvstat */
	fs_nosys,	/* setdvstat */
	nfs_makemld,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
};

/*ARGSUSED*/
STATIC int
nfs_open(vpp, flag, cred)
	register struct vnode **vpp;
	int flag;
	struct cred *cred;
{
	int error;
	struct vattr va;
	int mode = 0;

	/* LINTED pointer alignment */
	NFSLOG(2, "nfs_open %s %x ", vtomi(*vpp)->mi_hostname, *vpp);
	NFSLOG(2, "flag %d\n", flag, 0);
	error = 0;
	/*
	 * if close-to-open consistancy checking is turned off
	 * we can avoid the over the wire getattr.
	 * For SVR4ES close-to-open checking is always on
	 */
	/* LINTED pointer alignment */
	if (nfs_cto || !vtomi(*vpp)->mi_nocto || mac_installed) {
		/*
		 * Force a call to the server to get fresh attributes
		 * so we can check caches. This is required for close-to-open
		 * consistency.
		 */
		/* LINTED pointer alignment */
		if (vtomi(*vpp)->mi_protocol == NFS_V2)
			error = nfs_getattr_otw(*vpp, &va, cred);
		else {
			nfs_purge_caches(*vpp);
			if (flag & FWRITE)	mode |= VWRITE;
			if (flag & FREAD)	mode |= VREAD;
			error = nfs_access(*vpp, mode, 0, cred);
		}
	}
	NFSLOG(4, "nfs_open: returning %d\n", error, 0);
	return (error);
}

/* ARGSUSED */
STATIC int
nfs_close(vp, flag, count, offset, cred)
	struct vnode *vp;
	int flag;
	int count;
	off_t offset;	/* XXX */
	struct cred *cred;
{
	register struct rnode *rp;

	(void)nfs_lockrelease(vp, flag, offset, cred);

	if (count > 1)
		return (0);

	/* LINTED pointer alignment */
	NFSLOG(2, "nfs_close %s %x ", vtomi(vp)->mi_hostname, vp);
	NFSLOG(2, "flag %d\n", flag, 0);

	/* LINTED pointer alignment */
	rp = vtor(vp);

	if ( (rp->r_flags & REOF) && (vp->v_type == VDIR) )
		rp->r_flags &= ~REOF;

	/*
	 * If the file is an unlinked file, then flush the lookup
	 * cache so that nfs_inactive will be called if this is
	 * the last reference.  Otherwise, if close-to-open
	 * consistancy is turned on and the file was open
	 * for writing or we had an asynchronous write error, we
	 * force the "sync on close" semantic by calling nfs_putpage.
	 */
	if (rp->r_unldvp != NULL || rp->r_error) {
		(void) nfs_putpage(vp, 0, 0, B_INVAL, cred);
		dnlc_purge_vp(vp);
	/* LINTED pointer alignment */
	} else if ((nfs_cto || !vtomi(vp)->mi_nocto)
	    && ((flag & FWRITE) || rp->r_error)) {
		if (rp->r_error == 0) {
			rp->r_error = nfs_putpage(vp, 0, 0, 0, cred);
		}
		else	{
			(void) nfs_putpage(vp, 0, 0, 0, cred);
		}
	}
	NFSLOG(4, "nfs_close: returning %d\n",
		flag & FWRITE? rp->r_error : 0, 0);
	return (flag & FWRITE? rp->r_error : 0);
}

/*ARGSUSED*/
STATIC int
nfs_read(vp, uiop, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	int ioflag;
	struct cred *cred;
{
	return (nfs_rdwr(vp, uiop, UIO_READ, ioflag, cred));
}

/*ARGSUSED*/
STATIC int
nfs_write(vp, uiop, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	int ioflag;
	struct cred *cred;
{
	return (nfs_rdwr(vp, uiop, UIO_WRITE, ioflag, cred));
}

STATIC int
nfs_rdwr(vp, uiop, rw, ioflag, cred)
	register struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct cred *cred;
{
	int error = 0;
	/* LINTED pointer alignment */
	register struct rnode *rp = vtor(vp);

	/* LINTED pointer alignment */
	NFSLOG(2, "nfs_rdwr: %s %x ", vtomi(vp)->mi_hostname, vp);
	NFSLOG(2, "rw %s offset %x ",
		rw == UIO_READ ? "READ" : "WRITE", uiop->uio_offset);
	NFSLOG(2, "len %d cred 0x%x\n", uiop->uio_iov->iov_len, cred);

	if (vp->v_type != VREG) {
		return (EISDIR);
	}

	/*
	 * MAC cred handling: we are passed the open-time file cred for read/
	 * write, but we need the MAC level and privilege set from the current
	 * process cred.
	 */
	cred = crdup(cred);
	cred->cr_lid = u.u_procp->p_cred->cr_lid;
	cred->cr_wkgpriv = u.u_procp->p_cred->cr_wkgpriv;
	cred->cr_maxpriv = u.u_procp->p_cred->cr_maxpriv;

	if (rp->r_cred)
		crfree(rp->r_cred);
	rp->r_cred = cred;

	if ((ioflag & IO_APPEND) && rw == UIO_WRITE) {
		struct vattr va;

		rlock(rp);
		error = nfs_getattr(vp, &va, 0, cred);
		if (error == 0) {
			uiop->uio_offset = va.va_size;
		}
	}

	if (error == 0) {
		error = rwvp(vp, uiop, rw, ioflag, cred);
	}

	if ((ioflag & IO_APPEND) && rw == UIO_WRITE) {
		runlock(rp);
	}
	NFSLOG(4, "nfs_rdwr returning %d\n", error, 0);
	return (error);
}

STATIC int
rwvp(vp, uio, rw, ioflag, cred)
	register struct vnode *vp;
	register struct uio *uio;
	enum uio_rw rw;
	int ioflag;
	struct cred *cred;
{
	/* LINTED pointer alignment */
	struct rnode *rp = vtor(vp);
	u_int off;
	addr_t base;
	u_int flags;
	register int n, on, newn1, newn2;
	int error = 0;
	int eof = 0;
	int pagecreate;
	page_t *iolpl[MAXBSIZE/PAGESIZE + 2];
	page_t **ppp;

	if (uio->uio_resid == 0) {
		return (0);
	}
	if (uio->uio_offset < 0 || (uio->uio_offset + uio->uio_resid) < 0) {
		return (EINVAL);
	}
	if (rw == UIO_WRITE && vp->v_type == VREG &&
	    uio->uio_offset+uio->uio_resid >
	    u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
		psignal(u.u_procp, SIGXFSZ);
		return (EFBIG);
	}
	RLOCK(rp);
	do {
		off = uio->uio_offset & MAXBMASK;
		on = uio->uio_offset & MAXBOFFSET;
		n = MIN(MAXBSIZE - on, uio->uio_resid);

		if (rw == UIO_READ) {
			int diff;

#ifdef	VNOCACHE
			if (!(vp->v_flag & VNOCACHE) && page_find(vp, off)) {
				(void) nfs_validate_caches(vp, cred);
			}
#else
			if (page_find(vp, off)) {
				(void) nfs_validate_caches(vp, cred);
			}
#endif

			diff = rp->r_size - uio->uio_offset;
			if (diff <= 0) {
				break;
			}
			if (diff < n) {
				n = diff;
				eof = 1;
			}
		} else {
			/*
			 * Keep returning errors on rnode until
			 * rnode goes away.
			 */
			if (rp->r_error) {
				error = rp->r_error;
				break;
			}
		}

		/*
		 * Check to see if we can skip reading in the page
		 * and just allocate the memory.  We can do this
		 * if we are going to rewrite the entire mapping
		 * or if we are going to write to or beyond the current
		 * end of file from the beginning of the mapping.
		 */
                if (rw == UIO_WRITE) {
                        n = as_iolock(uio, iolpl, n, vp, rp->r_size, &pagecreate);
                        if (n == 0) {
                                error = EFAULT;
                                break;
                        }
		} else {
                        pagecreate = 0;
                        iolpl[0] = NULL;
		}

		base = segmap_getmap(segkmap, vp, off);

                if (pagecreate)
                        segmap_pagecreate(segkmap, base + on, (u_int)n, 0);

		/*
		 * divide up the writes which cross page boundaries
		 * because a fault for the second page may cause
		 * the first page to be written out, but since the
		 * file size will not be updated by then, we will
		 * loose data from the first page
		 */
		if ((rw == UIO_WRITE) && (on + n >= PAGESIZE) && on < PAGESIZE) {
			newn1 = PAGESIZE - on;
			newn2 = on + n - PAGESIZE;
			error = uiomove(base + on, newn1, rw, uio);
			if (!error) {
				if (rp->r_size < uio->uio_offset) {
					rp->r_size = uio->uio_offset;
				}
				error = uiomove(base + PAGESIZE, newn2, rw, uio);
			}
		} else {
			error = uiomove(base + on, n, rw, uio);
		}

		/* Now release any pages held by as_iolock. */
		for (ppp = iolpl; *ppp; ppp++ )
			PAGE_RELE(*ppp);

		if (pagecreate && uio->uio_offset <
		    roundup(off + on + n, PAGESIZE)) {
			/*
			 * We created pages w/o initializing them completely,
			 * thus we need to zero the part that wasn't set up.
			 * This happens on a most EOF write cases and if
			 * we had some sort of error during the uiomove.
			 */
			int nzero, nmoved;

			nmoved = uio->uio_offset - (off + on);
			ASSERT(nmoved >= 0 && nmoved <= n);
			nzero = roundup(n, PAGESIZE) - nmoved;
			ASSERT(nzero > 0 && on + nmoved + nzero <= MAXBSIZE);
			(void) kzero(base + on + nmoved, (u_int)nzero);
		}

		if (error == 0) {
			flags = 0;
			if (rw == UIO_WRITE) {
				/*
				 * r_size is the maximum number of
				 * bytes known to be in the file.
				 * Make sure it is at least as high as the
				 * last byte we just wrote into the buffer.
				 */
				if (rp->r_size < uio->uio_offset) {
					rp->r_size = uio->uio_offset;
				}
				/*
				 * Invalidate if entry is not to be cached.
				 */
#ifdef	VNOCACHE
				if (vp->v_flag & VNOCACHE)
					flags = SM_WRITE | SM_INVAL;
				else {
#endif
					rp->r_flags |= RDIRTY;
					if (n + on == MAXBSIZE ||
					    IS_SWAPVP(vp)) {
						/*
						 * Have written a whole block.
						 * Start an asynchronous write
						 * and mark the buffer to
						 * indicate that it won't be
						 * needed again soon.
						 */
						flags = SM_WRITE | SM_ASYNC |
						    SM_DONTNEED;
					}
#ifdef	VNOCACHE
				}
#endif
				if (ioflag & IO_SYNC) {
					flags &= ~SM_ASYNC;
					flags |= SM_WRITE;
				}
			} else {
#ifdef	VNOCACHE
				if (vp->v_flag & VNOCACHE)
					flags = SM_INVAL;
				else {
#endif
					/*
					 * If read a whole block or read to eof,
					 * won't need this buffer again soon.
					 */
					if (n + on == MAXBSIZE ||
					    uio->uio_offset == rp->r_size)
						flags = SM_DONTNEED;
#ifdef	VNOCACHE
				}
#endif
			}
			error = segmap_release(segkmap, base, flags);
		} else {
			(void) segmap_release(segkmap, base, 0);
		}

	} while (error == 0 && uio->uio_resid > 0 && !eof);
	RUNLOCK(rp);

	return (error);
}

/*
 * Flags are composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED}
 */
STATIC int
nfs_writelbn(rp, pp, off, len, flags, cred)
	register struct rnode *rp;
	struct page *pp;
	u_int off;
	u_int len;
	int flags;
	struct cred *cred;
{
	register struct buf *bp;
	int err;

	bp = pageio_setup(pp, len, rtov(rp), B_WRITE | flags);
	if (bp == NULL) {
		pvn_fail(pp, B_WRITE | flags);
		return (ENOMEM);
	}

	bp->b_edev = 0;
	bp->b_dev = 0;
	bp->b_blkno = btodb(off);
	bp_mapin(bp);

	if (rp->r_cred == NULL) {
		if (cred == NULL) {
			cred = u.u_cred; /* XXX need real cred! */
		}
		if (rp->r_cred) {
			crfree(rp->r_cred);
		}
		rp->r_cred = cred;
	}
	cred = rp->r_cred;
	crhold(cred);
	err = nfs_strategy(bp);
	crfree(cred);

	/* LINTED pointer alignment */
	NFSLOG(0xc, "nfs_writelbn %s blkno %d ",
		vtomi(rtov(rp))->mi_hostname, btodb(off));
	NFSLOG(0xc, "pp %x len %d ", pp, len);
	NFSLOG(0xc, "flags %x error %d\n", flags, err);
	return (err);
}

/*
 * Write to file.  Writes to remote server in largest size
 * chunks that the server can handle.  Write is synchronous.
 */
STATIC int
nfswrite(vp, base, offset, count, cred)
	struct vnode *vp;
	caddr_t base;
	u_int offset;
	long count;
	struct cred *cred;
{
	int error;
	struct nfswriteargs wa;
	struct nfsattrstat *ns;
	struct nfsesvattrstat *cns;
	int tsize;
	/* LINTED pointer alignment */
	register struct mntinfo *mi = vtomi(vp);
	int xid;

	NFSLOG(2, "nfswrite %s %x ", mi->mi_hostname, vp);
	NFSLOG(2, "offset = %d, count = %d\n", offset, count);

	ns = (struct nfsattrstat *)kmem_zalloc(sizeof (*ns), KM_SLEEP);
	cns = (struct nfsesvattrstat *)kmem_alloc(sizeof (*cns), KM_SLEEP);
	/*
	 * Temporarily invalidate attr cache since we know mtime will change
	 */
	/* LINTED pointer alignment */
	INVAL_ATTRCACHE(vp);
	do {
		xid = clnt_clts_getxid();
		tsize = MIN(mi->mi_curwrite, count);
		wa.wa_data = base;
		/* LINTED pointer alignment */
		wa.wa_fhandle = *vtofh(vp);
		wa.wa_begoff = offset;
		wa.wa_totcount = tsize;
		wa.wa_count = tsize;
		wa.wa_offset = offset;
		if (mi->mi_protocol == NFS_V2)
			error = rfscall(mi, RFS_WRITE, xid, xdr_writeargs,
			    (caddr_t)&wa, xdr_attrstat, (caddr_t)ns, cred);
		else
			error = rfscall(mi, RFS_WRITE, xid, xdr_writeargs,
			    (caddr_t)&wa, xdr_esvattrstat, (caddr_t)cns, cred);
		if (error == ENFS_TRYAGAIN) {
			error = 0;
			continue;
		}
		if (!error) {
			if (mi->mi_protocol == NFS_V2)
				error = geterrno(ns->ns_status);
			else
				error = geterrno(cns->ns_status);
			/*
			 * Can't check for stale fhandle and purge caches
			 * here because pages are held by nfs_getpage.
			 */
		}
		NFSLOG(8, "nfswrite: sent %d of %d, ", tsize, count);
		NFSLOG(8, "error %d\n", error, 0);
		count -= tsize;
		base += tsize;
		offset += tsize;
	} while (!error && count);

	if (!error) {
		if (mi->mi_protocol == NFS_V2)
			nfs_attrcache(vp, &ns->ns_attr);
		else
			nfs_esvattrcache(vp, &cns->ns_attr);
	} else {
		/*
		 * Since we invalidated the cache above without first
		 * purging cached pages we have to put it back in the
		 * "timed-out" state.
		 */
		/* LINTED pointer alignment */
		PURGE_ATTRCACHE(vp);
	}
	kmem_free((caddr_t)ns, sizeof (*ns));
	kmem_free((caddr_t)cns, sizeof (*cns));
	switch (error) {
	case 0:
#ifndef	SYSV
	case EDQUOT:
#endif
		break;

	case ENOSPC:
		cmn_err(CE_CONT, "NFS write error: on host %s remote file system full\n",
		   mi->mi_hostname);
		break;

	default:
		cmn_err(CE_CONT, "NFS write error %d on host %s fh ",
		    error, mi->mi_hostname);
		/* LINTED pointer alignment */
		printfhandle((caddr_t)vtofh(vp));
		cmn_err(CE_CONT, "\n");
		break;
	}
	NFSLOG(4, "nfswrite: returning %d\n", error, 0);
	return (error);
}

/*
 * Print a file handle
 */
STATIC void
printfhandle(fh)
	caddr_t fh;
{
	int i;
	int fhint[NFS_FHSIZE / sizeof (int)];

	bcopy(fh, (caddr_t)fhint, sizeof (fhint));
	for (i = 0; i < (sizeof (fhint) / sizeof (int)); i++) {
		cmn_err(CE_CONT, "%x ", fhint[i]);
	}
}

/*
 * Read from a file.  Reads data in largest chunks our interface can handle.
 */
STATIC int
nfsread(vp, base, offset, count, residp, cred, vap)
	struct vnode *vp;
	caddr_t base;
	u_int offset;
	long count;
	long *residp;
	struct cred *cred;
	struct vattr *vap;
{
	int error;
	struct nfsreadargs ra;
	struct nfsrdresult rr;
	struct nfsesvrdresult crr;
	register int tsize;
	int rcount, rstatus;
	lid_t tmplid;
	/* LINTED pointer alignment */
	register struct mntinfo *mi = vtomi(vp);
	int xid;

	NFSLOG(2, "nfsread %s %x ", mi->mi_hostname, vp);
	NFSLOG(2, "base: %x, offset = %d, ", base, offset);
	NFSLOG(2, "totcount = %d\n", count, 0);
	do {
		do {
			xid = clnt_clts_getxid();
			tsize = MIN(mi->mi_curread, count);
			rr.rr_data = base;
			crr.rr_data = base;
			/* LINTED pointer alignment */
			ra.ra_fhandle = *vtofh(vp);
			ra.ra_offset = offset;
			ra.ra_totcount = tsize;
			ra.ra_count = tsize;
			if (mi->mi_protocol == NFS_V2) {
				error = rfscall(mi, RFS_READ, xid,
					xdr_readargs, (caddr_t)&ra,
					xdr_rdresult, (caddr_t)&rr, cred);
				if (!error) {
					rcount = rr.rr_count;
					rstatus = rr.rr_status;
				}
			} else {
				error = rfscall(mi, RFS_READ, xid,
					xdr_readargs, (caddr_t)&ra,
					xdr_esvrdresult, (caddr_t)&crr, cred);
				if (!error) {
					rcount = crr.rr_count;
					rstatus = crr.rr_status;
				}
			}
		} while (error == ENFS_TRYAGAIN);

		if (!error) {
			error = geterrno(rstatus);
			/*
			 * Can't purge caches here because pages are held by
			 * nfs_getpage.
			 */
		}
		NFSLOG(8, "nfsread: got %d of %d, ", tsize, count);
		NFSLOG(8, "error %d\n", error, 0);
		if (!error) {
			count -= rcount;
			base += rcount;
			offset += rcount;
		}
	} while (!error && count && rcount == tsize);

	*residp = count;

	if (!error) {
		if (mi->mi_protocol == NFS_V2) {
			nfs_attrcache(vp, &rr.rr_attr);
			nattr_to_vattr(vp, &rr.rr_attr, vap);
		} else {
			nfs_esvattrcache(vp, &crr.rr_attr);
			/* LINTED pointer alignment */
			vtor(vp)->r_aclcnt = acl_getmax();
			/* LINTED pointer alignment */
			esvnattr_to_vattr(vp, &crr.rr_attr, vap, &tmplid, vtor(vp)->r_acl, &vtor(vp)->r_aclcnt);
		}
	}

	NFSLOG(4, "nfsread: returning %d, resid %d\n", error, *residp);
	return (error);
}

/*ARGSUSED*/
STATIC int
nfs_ioctl(vp, com, arg, flag, cred, rvalp)
	struct vnode *vp;
	int com;
	int arg;
	int flag;
	struct cred *cred;
	int *rvalp;
{
	return (ENOTTY);
}

/*ARGSUSED*/
STATIC int
nfs_getattr(vp, vap, flags, cred)
	struct vnode *vp;
	struct vattr *vap;
	int flags;
	struct cred *cred;
{
	int error;
	/* LINTED pointer alignment */
	register struct rnode *rp = vtor(vp);

	/* LINTED pointer alignment */
	NFSLOG(2, "nfs_getattr %s %x\n", vtomi(vp)->mi_hostname, vp);
	if (rp->r_flags & RDIRTY) {
		/*
		 * Since we know we have pages which are dirty because
		 * we went thru rwvp for writing, we sync pages so the
		 * mod time is right.  Note that if a page which is mapped
		 * in user land is modified, the page will not be flushed
		 * until the next sync or appropriate fsync or msync operation.
		 */
		(void) nfs_putpage(vp, 0, 0, 0, cred);
	}
	error = nfsgetattr(vp, vap, cred);
	NFSLOG(4, "nfs_getattr: returns %d\n", error, 0);
	return (error);
}

STATIC int
nfs_setattr(vp, vap, flags, cred)
	register struct vnode *vp;
	register struct vattr *vap;
	struct cred *cred;
	int flags;
{
	int error;
	long mask = vap->va_mask;
	struct nfssaargs args;
	struct nfsesvsaargs cargs;
	struct nfsattrstat *ns;
	struct nfsesvattrstat *cns;
	/* LINTED pointed alignment */
	register struct rnode *rp = vtor(vp);

	/* LINTED pointer alignment */
	NFSLOG(2, "nfs_setattr %s %x\n", vtomi(vp)->mi_hostname, vp);
	if (mask & AT_NOSET)
		return(EINVAL);

	ns = (struct nfsattrstat *)kmem_zalloc(sizeof (*ns), KM_SLEEP);
	cns = (struct nfsesvattrstat *)kmem_alloc(sizeof (*cns), KM_SLEEP);
	rlock(rp);
	if (mask & AT_SIZE) {
		pvn_vptrunc(vp, (u_int)vap->va_size, (u_int)(PAGESIZE -
		    (vap->va_size & PAGEOFFSET)));
		rp->r_size = vap->va_size;
	}
	rp->r_error = nfs_putpage(vp, 0, 0, 0, cred);
	runlock(rp);

	/*
	 * Allow SysV-compatible option to set access and
	 * modified times if root, owner, or write access.
	 *
	 * XXX - For now, va_mtime.tv_nsec == -1 flags this.
	 *
	 * XXX - Until an NFS Protocol Revision, this may be
	 *       simulated by setting the client time in the
	 *       tv_sec field of the access and modified times
	 *       and setting the tv_nsec field of the modified
	 *       time to an invalid value (1,000,000).  This
	 *       may be detected by servers modified to do the
	 *       right thing, but will not be disastrous on
	 *       unmodified servers.
	 * XXX - 1,000,000 is actually a valid nsec value, but
	 *       the protocol says otherwise.
	 */
	if ((mask & AT_MTIME) && !(flags & ATTR_UTIME)) {
		vap->va_atime.tv_sec = hrestime.tv_sec;
		vap->va_atime.tv_nsec = hrestime.tv_nsec;
		vap->va_mtime.tv_sec = hrestime.tv_sec;
		vap->va_mtime.tv_nsec = 1000000000;
	} else
		vap->va_mtime.tv_nsec = 0;

	/* make sure we only set the right fields */
	if (!(mask&AT_MODE))	vap->va_mode = (mode_t)-1;
	if (!(mask&AT_UID))	vap->va_uid = -1;
	if (!(mask&AT_GID))	vap->va_gid = -1;
	if (!(mask&AT_SIZE))	vap->va_size = (u_long)-1;
	/* set if: AT_ATIME not set and
		   AT_MTIME set and ATTR_UTIME not set */
	if (!(mask&AT_ATIME) && (!(mask&AT_MTIME) || (flags&ATTR_UTIME)))
		vap->va_atime.tv_sec = vap->va_atime.tv_nsec = -1;
	if (!(mask&AT_MTIME))	vap->va_mtime.tv_sec = vap->va_mtime.tv_nsec = -1;

	/* LINTED pointer alignment */
	if (vtomi(vp)->mi_protocol == NFS_V2) {
		vattr_to_sattr(vap, &args.saa_sa);
		/* LINTED pointer alignment */
		args.saa_fh = *vtofh(vp);
		/* LINTED pointer alignment */
		error = rfscall(vtomi(vp), RFS_SETATTR, 0, xdr_saargs,
		    (caddr_t)&args, xdr_attrstat, (caddr_t)ns, cred);
		/* hack for systems that won't grow files but won't tell us */
		if ((mask&AT_SIZE) == AT_SIZE && vap->va_size != 0 &&
		    ns->ns_attr.na_size == 0)
			error = EINVAL;
		if (error == 0) {
			error = geterrno(ns->ns_status);
			if (error == 0) {
				timestruc_t	mtime;
				mtime.tv_sec  = ns->ns_attr.na_mtime.tv_sec;
				mtime.tv_nsec = ns->ns_attr.na_mtime.tv_usec*1000;
				nfs_cache_check(vp, mtime);
				nfs_attrcache(vp, &ns->ns_attr);
			} else {
				/* LINTED pointer alignment */
				PURGE_ATTRCACHE(vp);
				PURGE_STALE_FH(error, vp);
			}
		}
	} else {
		/* no LID or ACL to be set */
		/* LINTED pointer alignment */
		vattr_to_esvsattr(vap, &cargs.saa_sa, &vtomi(vp)->mi_addr, NULL, NULL, 0);
		/* LINTED pointer alignment */
		cargs.saa_fh = *vtofh(vp);
		/* LINTED pointer alignment */
		error = rfscall(vtomi(vp), RFS_SETATTR, 0, xdr_esvsaargs,
		    (caddr_t)&cargs, xdr_esvattrstat, (caddr_t)cns, cred);
		/* hack for systems that won't grow files but won't tell us */
		if ((mask&AT_SIZE) == AT_SIZE && vap->va_size != 0 &&
		    cns->ns_attr.na_size == 0)
			error = EINVAL;
		if (error == 0) {
			error = geterrno(cns->ns_status);
			if (error == 0) {
				timestruc_t	mtime;
				mtime.tv_sec  = cns->ns_attr.na_mtime.tv_sec;
				mtime.tv_nsec = cns->ns_attr.na_mtime.tv_usec*1000;
				nfs_cache_check(vp, mtime);
				nfs_esvattrcache(vp, &cns->ns_attr);
			} else {
				/* LINTED pointer alignment */
				PURGE_ATTRCACHE(vp);
				PURGE_STALE_FH(error, vp);
			}
		}
	}
	kmem_free((caddr_t)ns, sizeof (*ns));
	kmem_free((caddr_t)cns, sizeof (*cns));
	NFSLOG(4, "nfs_setattr: returning %d\n", error, 0);
	return (error);
}

#define	TST_GROUP	3
#define	TST_OTHER	6

/* ARGSUSED */
STATIC int
nfs_access(vp, mode, flags, cred)
	struct vnode *vp;
	int mode;
	int flags;
	struct cred *cred;
{
	struct vattr va;
	register int denied_mode, lshift;
	struct nfsaccessargs acca;
	struct nfsaccessres accres;
	int error;
	int i;

	/* LINTED pointer alignment */
	NFSLOG(2, "nfs_access %s %x ", vtomi(vp)->mi_hostname, vp);
	NFSLOG(2, "mode %d uid %d\n", mode, cred->cr_uid);
	/* LINTED pointer alignment */
	if (!nfs_getattr_cache(vp, &va) && vtomi(vp)->mi_protocol == NFS_ESV) {
		/* LINTED pointer alignment */
		acca.acc_fhandle = *vtofh(vp);
		acca.acc_flag = 0;
		if (mode & VWRITE)	acca.acc_flag |= ACCESS_WRITE;
		if (mode & VREAD)	acca.acc_flag |= ACCESS_READ;
		if (mode & VEXEC)	acca.acc_flag |= ACCESS_EXEC;
		/* LINTED pointer alignment */
		error = rfscall(vtomi(vp), RFS_ACCESS, 0, xdr_accessargs,
			(caddr_t)&acca, xdr_accessres, (caddr_t)&accres, cred);
		if (!error)
			error = geterrno(accres.acc_status);
		if (!error) {
			timestruc_t mtime;
			mtime.tv_sec  = accres.acc_attr.na_mtime.tv_sec;
			mtime.tv_nsec = accres.acc_attr.na_mtime.tv_usec * 1000;
			nfs_cache_check(vp, mtime);
			nfs_esvattrcache(vp, &accres.acc_attr);
		}
		NFSLOG(4, "nfs_access: returning %d\n", error, 0);
		return (error);
	} else {
		error = nfsgetattr(vp, &va, cred);
		if (error)
			return (error);
	}
	/*
	 * Access check is based on only
	 * one of owner, group, public.
	 * If not owner, then check group.
	 * If not a member of the group,
	 * then check public access.
	 */
	if (cred->cr_uid == va.va_uid)
		lshift = 0;			/* TST OWNER */
	else if (groupmember(va.va_gid, cred)) {
		mode >>= TST_GROUP;
		lshift = TST_GROUP;
	}
	else {
		mode >>= TST_OTHER;
		lshift = TST_OTHER;
	}
	if ((i = (va.va_mode & mode)) == mode) {
		return (0);
	}

	denied_mode = (mode & (~i));
	denied_mode <<= lshift;

	if ((denied_mode & (VREAD | VEXEC)) && pm_denied(cred, P_DACREAD)) {
		NFSLOG(4, "nfs_access: read access denied %d\n", EACCES, 0);
		return (EACCES);
	}
	if ((denied_mode & VWRITE) && pm_denied(cred, P_DACWRITE)) {
		NFSLOG(4, "nfs_access: write access denied %d\n", EACCES, 0);
		return (EACCES);
	}

	NFSLOG(4, "nfs_access: returning 0\n", 0, 0);
	return (0);
}

STATIC int
nfs_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct cred *cred;
{
	int error;
	struct nfsrdlnres rl;
	struct nfsesvrdlnres crl;
	/* LINTED pointer alignment */
	register struct mntinfo *mi = vtomi(vp);

	NFSLOG(2, "nfs_readlink %s %x\n", mi->mi_hostname, vp);
	if (vp->v_type != VLNK)
		return (ENXIO);
	rl.rl_data = (char *)kmem_alloc(NFS_MAXPATHLEN, KM_SLEEP);
	crl.rl_data = (char *)kmem_alloc(NFS_MAXPATHLEN, KM_SLEEP);
	if (mi->mi_protocol == NFS_V2) {
		error = rfscall(mi, RFS_READLINK, 0, xdr_fhandle,
		    /* LINTED pointer alignment */
		    (caddr_t)vtofh(vp), xdr_rdlnres, (caddr_t)&rl, cred);
		if (!error) {
			error = geterrno(rl.rl_status);
			if (!error) {
				error = uiomove(rl.rl_data, (int)rl.rl_count,
				    UIO_READ, uiop);
			} else {
				PURGE_STALE_FH(error, vp);
			}
		}
	} else {
		error = rfscall(mi, RFS_READLINK, 0, xdr_fhandle,
		    /* LINTED pointer alignment */
		    (caddr_t)vtofh(vp), xdr_esvrdlnres, (caddr_t)&crl, cred);
		if (!error)
			error = geterrno(crl.rl_status);
		if (!error) {
			timestruc_t mtime;
			mtime.tv_sec = crl.rl_attr.na_mtime.tv_sec;
			mtime.tv_nsec = crl.rl_attr.na_mtime.tv_usec * 1000;
			nfs_cache_check(vp, mtime);
			nfs_esvattrcache(vp, &crl.rl_attr);
			error = uiomove(crl.rl_data, (int)crl.rl_count,
			    UIO_READ, uiop);
		} else {
			PURGE_STALE_FH(error, vp);
		}
	}
	kmem_free((caddr_t)rl.rl_data, NFS_MAXPATHLEN);
	kmem_free((caddr_t)crl.rl_data, NFS_MAXPATHLEN);
	NFSLOG(4, "nfs_readlink: returning %d\n", error, 0);
	return (error);
}

/*ARGSUSED*/
STATIC int
nfs_fsync(vp, cred)
	struct vnode *vp;
	struct cred *cred;
{
	register struct rnode *rp;

	/* LINTED pointer alignment */
	NFSLOG(2, "nfs_fsync %s %x\n", vtomi(vp)->mi_hostname, vp);
	/* LINTED pointer alignment */
	rp = vtor(vp);
	rlock(rp);
	rp->r_error = nfs_putpage(vp, 0, 0, 0, cred);
	runlock(rp);
	NFSLOG(4, "nfs_fsync returning %d\n", rp->r_error, 0);
	return (rp->r_error);
}

/*
 * Weirdness: if the file was removed while it was open it got renamed
 * (by nfs_remove) instead.  Here we remove the renamed file.
 */
/*ARGSUSED*/
STATIC void
nfs_inactive(vp, cred)
	register struct vnode *vp;
	struct cred *cred;
{
	register struct rnode *rp;
	struct nfsdiropargs da;
	enum nfsstat status;
	int error;

	/* LINTED pointer alignment */
	NFSLOG(2, "nfs_inactive %s, %x\n", vtomi(vp)->mi_hostname, vp);
	/* LINTED pointer alignment */
	rp = vtor(vp);

        /*
         *  Mark inactive in progress.	This allows VOP_PUTPAGE to
	 *  abort a concurrent attempt to flush a page due to pageout/fsflush.
	 */
	ASSERT((rp->r_flags & RINACTIVE) == 0);
	rp->r_flags |= RINACTIVE;
redo:
	if (vp->v_count != 0) {
		rp->r_flags &= ~RINACTIVE;
		return;
	}

	if (rp->r_unldvp != NULL) {
		/*
		 * Lock down directory where unlinked-open file got renamed.
		 * This keeps a lookup from finding this rnode.
		 */
		/* LINTED pointer alignment */
		rlock(vtor(rp->r_unldvp));

		rp->r_flags &= ~RDIRTY;
		pvn_vptrunc(vp, 0, 0);		/* toss all pages */

		/*
		 * Do the remove operation on the renamed file
		 */
		setdiropargs(&da, rp->r_unlname, rp->r_unldvp);
		/* LINTED pointer alignment */
		error = rfscall(vtomi(rp->r_unldvp), RFS_REMOVE, 0,
		    xdr_diropargs, (caddr_t)&da,
		    xdr_enum, (caddr_t)&status, rp->r_unlcred);
		if (error == 0)
			error = geterrno(status);

		/*
		 * release parent directory
		 */
		/* LINTED pointer alignment */
		runlock(vtor(rp->r_unldvp));

		/*
		 * Release stuff held for the remove
		 */
		VN_RELE(rp->r_unldvp);
		rp->r_unldvp = NULL;
		kmem_free((caddr_t)rp->r_unlname, NFS_MAXNAMLEN);
		rp->r_unlname = NULL;
		crfree(rp->r_unlcred);
		rp->r_unlcred = NULL;
	}

	/*
	 * Check to be sure that the rnode has not been grabbed before
	 * freeing it.
	 */
	if (vp->v_count == 0) {
		if (rp->r_unldvp != NULL) {
			goto redo;
		}
		rp_rmhash(rp);
		rfree(rp);
	}
        rp->r_flags &= ~RINACTIVE;

	NFSLOG(4, "nfs_inactive done\n", 0, 0);
}

int nfs_dnlc = 1;	/* use dnlc */

/*
 * Remote file system operations having to do with directory manipulation.
 */

/* VOP_LOOKUP params have changed so add a stub to handle it	*JSL* */
/*ARGSUSED*/
STATIC int
nfs_lookup(dvp, nm, vpp, pnp, f, rdir, cr)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct vnode *rdir;
	struct cred *cr;
	struct pathname *pnp;
	int f;
{
	return(nfs_oldlookup(dvp, nm, vpp, cr, pnp, f));
}

/* ARGSUSED */
STATIC int
nfs_oldlookup(dvp, nm, vpp, cred, pnp, flags)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct cred *cred;
	struct pathname *pnp;
	int flags;
{
	int error;
	struct nfsdiropargs da;
	struct nfsdiropres *dr;
	struct nfsesvdiropres *cdr;
	/* LINTED pointer alignment */
	register struct mntinfo *mi = vtomi(dvp);

	NFSLOG(2, "nfs_lookup %s %x ", mi->mi_hostname, dvp);
	NFSLOG(2, "'%s'\n", nm, 0);
	/*
	 * Before checking dnlc, validate caches
	 */
	error = nfs_validate_caches(dvp, cred);
	if (error) {
		return (error);
	}

	/* LINTED pointer alignment */
	rlock(vtor(dvp));
	*vpp = (struct vnode *)dnlc_lookup(dvp, nm, cred);
	/* LINTED pointer alignment */
	if (*vpp && (vtor(*vpp)->r_flags & (RMLD|REFFMLD))) {
		/* We shouldn't have gotten an MLD. Try again */
		VN_RELE(*vpp);
		dnlc_remove(dvp, nm);
		*vpp = 0;
	}
	if (*vpp) {
		VN_HOLD(*vpp);

		/*
		 *	Make sure we can search this directory (after the
		 *	fact).  It's done here because over the wire lookups
		 *	verify permissions on the server.  VOP_ACCESS will
		 *	one day go over the wire, so let's use it sparingly.
		 */
		error = VOP_ACCESS(dvp, VEXEC, 0, cred);
		if (error) {
			VN_RELE(*vpp);
			/* LINTED pointer alignment */
			runlock(vtor(dvp));
			return (error);
		}
	} else {
		dr  = (struct nfsdiropres *)kmem_alloc(sizeof (*dr), KM_SLEEP);
		cdr = (struct nfsesvdiropres *)kmem_alloc(sizeof(*cdr), KM_SLEEP);
		setdiropargs(&da, nm, dvp);

		if (mi->mi_protocol == NFS_V2) {
			error = rfscall(mi, RFS_LOOKUP, 0, xdr_diropargs,
			    (caddr_t)&da, xdr_diropres, (caddr_t)dr, cred);
			if (error == 0)
				error = geterrno(dr->dr_status);
			if (error == 0) {
				PURGE_STALE_FH(error, dvp);
				*vpp = makenfsnode(&dr->dr_fhandle,
				    &dr->dr_attr, dvp->v_vfsp);
			}
		} else {
			error = rfscall(mi, RFS_LOOKUP, 0, xdr_diropargs,
			    (caddr_t)&da, xdr_esvdiropres, (caddr_t)cdr, cred);
			if (error == 0)
				error = geterrno(cdr->dr_status);
			if (error == 0) {
				PURGE_STALE_FH(error, dvp);
				*vpp = makeesvnfsnode(&cdr->dr_fhandle,
				    &cdr->dr_attr, dvp->v_vfsp);
			}
		}
		/*
		 * Several cases to consider if we're in MLD virtual mode
		 * (and are using the ESV protocol: we'll never get an MLD
		 * with the v.2 protocol):
		 * 1) We just got a parent MLD
		 *    -> lookup the child for our level and use it.
		 *    -> if the child doesn't exist create it.
		 * 2) We just got a parent MLD and are looking for ".."
		 *    -> lookup ".." in the parent and return the grandparent
		 *    a) The parent MLD is the current global root directory
		 *       -> go back to the child
		 *    b) The parent MLD is the filesystem root directory
		 *       -> replace with the vnode covered until not a root dir,
		 *          then do VOP_LOOKUP in that new dir.
		 *
		 * NOTE: the VOP_LOOKUP above is ok because rfscall/makenfsnode
		 *       do not automatically deflect through MLD links.
		 */
		if (error == 0 &&
		    !(cred->cr_flags & CR_MLDREAL) &&
		    (mi->mi_protocol == NFS_ESV) &&
		    /* LINTED pointer alignment */
		    (vtor(*vpp)->r_flags & RMLD)) {
			if (nm[0] == '.' && nm[1] == '.' && nm[2] == '\0') {
				struct vnode *gpvp;

				NFSLOG(8, "nfs_oldlookup: looking for \"..\" in a MLD effective subdirectory\n", 0, 0);
				/* Adapted from lookuppn() */
checkforroot:
				if (VN_CMP(*vpp, u.u_rdir) || VN_CMP(*vpp, rootdir)) {
					VN_RELE(*vpp);
					*vpp = dvp;
					VN_HOLD(*vpp);
					goto skip;
				}
				if ((*vpp)->v_flag & VROOT) {
					gpvp = *vpp;
					*vpp = (*vpp)->v_vfsp->vfs_vnodecovered;
					VN_HOLD(*vpp);
					VN_RELE(gpvp);
					goto checkforroot;
				}
				if ((error = MAC_VACCESS(*vpp,VEXEC,cred)) == 0)
					error = VOP_LOOKUP(*vpp, nm, &gpvp,
						    (struct pathname *)NULL, 0,
						    (struct vnode *)NULL, cred);
					if (!error) {
						VN_RELE(*vpp);
						*vpp = gpvp;
				}
			} else {
				char effname[2*sizeof(lid_t)+1];
				struct vnode *effvp;

				NFSLOG(8, "nfs_oldlookup: in a parent MLD, deflecting at LID %d\n", cred->cr_lid, 0);
				fs_itoh(cred->cr_lid, effname);
				error = VOP_LOOKUP(*vpp, effname, &effvp,
						(struct pathname *)NULL, 0,
						(struct vnode *)NULL, cred);
				if (!error) {
					VN_RELE(*vpp);
					*vpp = effvp;
					/* LINTED pointer alignment */
					vtor(*vpp)->r_flags |= REFFMLD;
					/* make sure it's a directory */
					if ((*vpp)->v_type != VDIR) {
						error = ENOTDIR;
						VN_RELE(*vpp);
					}
				}
				if (error == ENOENT) {
					struct cred *tmpcred;
					struct vattr effva;

					NFSLOG(8, "nfs_oldlookup: effective MLD subdir not found, creating it\n", 0, 0);
					tmpcred = crdup(cred);
					tmpcred->cr_wkgpriv |= pm_privbit(P_DACWRITE);
					/* LINTED pointer alignment */
					effva = vtor(*vpp)->r_attr;
					effva.va_mask = AT_TYPE|AT_MODE;
					error = VOP_MKDIR(*vpp, effname, &effva,
							  &effvp, tmpcred);
					crfree(tmpcred);
					if (!error) {
						VN_RELE(*vpp);
						*vpp = effvp;
						/* LINTED pointer alignment */
						vtor(*vpp)->r_flags |= REFFMLD;
					}
				}
			}
		}
		/* If a symlink set the level to the parent directory's level */
		if (error == 0 && (*vpp)->v_type == VLNK) {
			(*vpp)->v_lid = dvp->v_lid;
		}
		if (error == 0) {
			if (nfs_dnlc &&
			    (dvp->v_vfsp->vfs_fsid.val[1] ==
			     (*vpp)->v_vfsp->vfs_fsid.val[1]) &&
				    /* LINTED pointer alignment */
			    !(vtor(dvp) ->r_flags & (RMLD|REFFMLD)) &&
				    /* LINTED pointer alignment */
			    !(vtor(*vpp)->r_flags & (RMLD|REFFMLD))) {
				dnlc_enter(dvp, nm, *vpp, cred);
			}
		} else {
			*vpp = (struct vnode *)0;
		}

skip:
		kmem_free((caddr_t)dr, sizeof (*dr));
		kmem_free((caddr_t)cdr, sizeof (*cdr));
	}
	/*
	 * If vnode is a device create special vnode
	 */
	if (!error && ISVDEV((*vpp)->v_type)) {
		struct vnode *newvp;

		newvp = specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type, cred);
		VN_RELE(*vpp);
		*vpp = newvp;
	}
	/* LINTED pointer alignment */
	runlock(vtor(dvp));
	NFSLOG(4, "nfs_lookup returning %d vp = %x\n", error, *vpp);
	return (error);
}

/*ARGSUSED*/
STATIC int
nfs_create(dvp, nm, va, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *va;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct cred *cred;
{
	int error;
	struct nfscreatargs args;
	struct nfsesvcreatargs cargs;
	struct nfsdiropres *dr;
	struct nfsesvdiropres *cdr;
	/* LINTED pointer alignment */
	register struct mntinfo *mi = vtomi(dvp);

	NFSLOG(2, "nfs_create %s %x ", mi->mi_hostname, dvp);
	NFSLOG(2, "'%s' excl=%d, ", nm, exclusive);
	NFSLOG(2, "mode=%o\n", mode, 0);
	if (exclusive == EXCL) {
		/*
		 * This is buggy: there is a race between the lookup and the
		 * create.  We should send the exclusive flag over the wire.
		 */
		error = nfs_oldlookup(dvp, nm, vpp, cred, (struct pathname *) NULL, 0);
		if (!error) {
			VN_RELE(*vpp);
			return (EEXIST);
		}
	}
	*vpp = (struct vnode *)0;

	if (mi->mi_protocol == NFS_V2) {
		dr = (struct nfsdiropres *)kmem_alloc(sizeof (*dr), KM_SLEEP);
		setdiropargs(&args.ca_da, nm, dvp);
	} else {
		cdr = (struct nfsesvdiropres *)kmem_alloc(sizeof(*cdr), KM_SLEEP);
		setdiropargs(&cargs.ca_da, nm, dvp);
	}

	/*
	 * Decide what the group-id of the created file should be.
	 * Set it in attribute list as advisory...then do a setattr
	 * if the server didn't get it right the first time.
	 */
	va->va_gid = (short) setdirgid(dvp);
	/* also set the uid, as vn_open() doesn't set it */
	va->va_uid = cred->cr_uid;

	/*
	 * This is a completely gross hack to make mknod
	 * work over the wire until we can wack the protocol
	 */
#define	IFCHR		0020000		/* character special */
#define	IFBLK		0060000		/* block special */
#define	IFSOCK		0140000		/* socket */
	if (va->va_type == VCHR) {
		va->va_mode |= IFCHR;
		va->va_size = (u_long)va->va_rdev;
		va->va_mask |= AT_SIZE;
	} else if (va->va_type == VBLK) {
		va->va_mode |= IFBLK;
		va->va_size = (u_long)va->va_rdev;
		va->va_mask |= AT_SIZE;
	} else if (va->va_type == VFIFO) {
		va->va_mode |= IFCHR;		/* xtra kludge for namedpipe */
		va->va_size = (u_long)NFS_FIFO_DEV;	/* blech */
		va->va_mask |= AT_SIZE;
#ifndef	SYSV
	/*
	 *	System V doesn't believe in these, even on other machines'
	 *	file systems - fix this.
	 *	XXX
	 */
	} else if (va->va_type == VSOCK) {
		va->va_mode |= IFSOCK;
#endif
	}

	va->va_atime.tv_sec = va->va_atime.tv_nsec = -1;
	va->va_mtime.tv_sec = va->va_mtime.tv_nsec = -1;
	if (mi->mi_protocol == NFS_V2) {
		vattr_to_sattr(va, &args.ca_sa);
		/* mark size not set if AT_SIZE not in va_mask */
		if (!(va->va_mask & AT_SIZE))
			args.ca_sa.sa_size = (u_long)-1;
	} else {
		/*
		 * XXX When ACL's are supported we need to deal with
		 * default ACL's here!!!
		 */
		vattr_to_esvsattr(va, &cargs.ca_sa, &mi->mi_addr,
				  &cred->cr_lid, NULL, 0);
		if (!(va->va_mask & AT_SIZE))
			cargs.ca_sa.sa_size = (u_long)-1;
	}
	/* LINTED pointer alignment */
	rlock(vtor(dvp));
	if (nm && *nm)
		dnlc_remove(dvp, nm);
	if (mi->mi_protocol == NFS_V2)
		error = rfscall(mi, RFS_CREATE, 0, xdr_creatargs, (caddr_t)&args,
		    xdr_diropres, (caddr_t)dr, cred);
	else
		error = rfscall(mi, RFS_CREATE, 0, xdr_esvcreatargs, (caddr_t)&cargs,
		    xdr_esvdiropres, (caddr_t)cdr, cred);

	/* LINTED pointer alignment */
	PURGE_ATTRCACHE(dvp);	/* mod time changed */
	if (!error)
		error = geterrno((mi->mi_protocol == NFS_V2) ?
				 dr->dr_status : cdr->dr_status);
	if (!error) {
		short gid;

		if (mi->mi_protocol == NFS_V2)
			*vpp = makenfsnode(&dr->dr_fhandle, &dr->dr_attr,
			    dvp->v_vfsp);
		else
			*vpp = makeesvnfsnode(&cdr->dr_fhandle, &cdr->dr_attr,
			    dvp->v_vfsp);
		if (va->va_size == 0) {
			/* LINTED pointer alignment */
			(vtor(*vpp))->r_size = 0;
		}
		if (nfs_dnlc && nm && *nm &&
		    /* LINTED pointer alignment */
		    !(vtor(dvp)->r_flags & (RMLD|REFFMLD))) {
			dnlc_enter(dvp, nm, *vpp, cred);
		}

		/*
		 * Make sure the gid was set correctly.
		 * If not, try to set it (but don't lose
		 * any sleep over it).
		 */
		gid = va->va_gid;
		if (mi->mi_protocol == NFS_V2)
			nattr_to_vattr(*vpp, &dr->dr_attr, va);
		else {
			/* LINTED pointer alignment */
			vtor(*vpp)->r_aclcnt = acl_getmax();
			/* LINTED pointer alignment */
			esvnattr_to_vattr(*vpp, &cdr->dr_attr, va, &(*vpp)->v_lid, vtor(*vpp)->r_acl, &vtor(*vpp)->r_aclcnt);
		}
		if (gid != va->va_gid) {
			struct vattr vattr;

			vattr.va_mask = AT_GID;
			vattr.va_gid = gid;
			(void) nfs_setattr(*vpp, &vattr, 0, cred);
			va->va_gid = vattr.va_gid;
		}

		/*
		 * If vnode is a device create special vnode
		 */
		if (ISVDEV((*vpp)->v_type)) {
			struct vnode *newvp;

			newvp = specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type, cred);
			VN_RELE(*vpp);
			*vpp = newvp;
		}
	} else {
		PURGE_STALE_FH(error, dvp);
	}
	/* LINTED pointer alignment */
	runlock(vtor(dvp));
	if (mi->mi_protocol == NFS_V2)
		kmem_free((caddr_t)dr, sizeof (*dr));
	else
		kmem_free((caddr_t)cdr, sizeof (*cdr));
	NFSLOG(4, "nfs_create returning %d\n", error, 0);
	return (error);
}

/*
 * Weirdness: if the vnode to be removed is open
 * we rename it instead of removing it and nfs_inactive
 * will remove the new name.
 */
STATIC int
nfs_remove(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct cred *cred;
{
	int error;
	struct nfsdiropargs da;
	enum nfsstat status;
	struct vnode *vp;
	struct vnode *oldvp;
	struct vnode *realvp;
	char *tmpname;
	/* LINTED pointer alignment */
	register struct rnode *drp = vtor(dvp);
	register struct rnode *rp;

	/* LINTED pointer alignment */
	NFSLOG(2, "nfs_remove %s %s\n", vtomi(dvp)->mi_hostname, nm);
	status = NFS_OK;
	error = nfs_oldlookup(dvp, nm, &vp, cred, (struct pathname *) NULL, 0);
	/*
	 * Lookup may have returned a non-nfs vnode!
	 * get the real vnode.
	 */
	if (error == 0 && VOP_REALVP(vp, &realvp) == 0) {
		oldvp = vp;
		vp = realvp;
	} else {
		oldvp = NULL;
	}
	if (error == 0 && vp != NULL) {
		/* LINTED pointer alignment */
		rp = vtor(vp);
		rlock(drp);
		/*
		 * We need to flush the name cache so we can
		 * check the real reference count on the vnode
		 */
		dnlc_purge_vp(vp);

		if ((vp->v_count > 1) && rp->r_unldvp == NULL) {
			tmpname = newname();
			error = nfs_rename(dvp, nm, dvp, tmpname, cred);
			if (error) {
				kmem_free((caddr_t)tmpname, NFS_MAXNAMLEN);
			} else {
				VN_HOLD(dvp);
				rp->r_unldvp = dvp;
				rp->r_unlname = tmpname;
				if (rp->r_unlcred != NULL) {
					crfree(rp->r_unlcred);
				}
				crhold(cred);
				rp->r_unlcred = cred;
			}
		} else {
			rp->r_flags &= ~RDIRTY;
			pvn_vptrunc(vp, 0, 0);		/* toss all pages */
			setdiropargs(&da, nm, dvp);
			/* LINTED pointer alignment */
			error = rfscall(vtomi(dvp), RFS_REMOVE, 0, xdr_diropargs,
			    (caddr_t)&da, xdr_enum, (caddr_t)&status, cred);
			/* LINTED pointer alignment */
			PURGE_ATTRCACHE(dvp);	/* mod time changed */
			/* LINTED pointer alignment */
			PURGE_ATTRCACHE(vp);	/* link count changed */
			PURGE_STALE_FH(error ? error : geterrno(status), dvp);
		}
		runlock(drp);
		if (oldvp) {
			VN_RELE(oldvp);
		} else {
			VN_RELE(vp);
		}
	}
	if (error == 0) {
		error = geterrno(status);
	}
	NFSLOG(4, "nfs_remove: returning %d\n", error, 0);
	return (error);
}

STATIC int
nfs_link(tdvp, svp, tnm, cred)
	struct vnode *tdvp;
	struct vnode *svp;
	char *tnm;
	struct cred *cred;
{
	int error;
	struct nfslinkargs args;
	enum nfsstat status;
	struct vnode *realvp;
	/* LINTED pointer alignment */
	register struct rnode *tdrp = vtor(tdvp);
	/* LINTED pointer alignment */
	register struct mntinfo *smi = vtomi(svp);

	if (VOP_REALVP(svp, &realvp) == 0) {
		svp = realvp;
	}

	NFSLOG(2, "nfs_link from %s %x ", smi->mi_hostname, svp);
	NFSLOG(2, "to %s %x ", smi->mi_hostname, tdvp);
	NFSLOG(2, "'%s'\n", tnm, 0);
	/* LINTED pointer alignment */
	args.la_from = *vtofh(svp);
	setdiropargs(&args.la_to, tnm, tdvp);
	rlock(tdrp);
	error = rfscall(smi, RFS_LINK, 0, xdr_linkargs, (caddr_t)&args,
	    xdr_enum, (caddr_t)&status, cred);
	/* LINTED pointer alignment */
	PURGE_ATTRCACHE(tdvp);	/* mod time changed */
	/* LINTED pointer alignment */
	PURGE_ATTRCACHE(svp);	/* link count changed */
	runlock(tdrp);
	if (!error) {
		error = geterrno(status);
		PURGE_STALE_FH(error, svp);
		PURGE_STALE_FH(error, tdvp);
	}
	NFSLOG(4, "nfs_link returning %d\n", error, 0);
	return (error);
}

STATIC int
nfs_rename(odvp, onm, ndvp, nnm, cred)
	struct vnode *odvp;
	char *onm;
	struct vnode *ndvp;
	char *nnm;
	struct cred *cred;
{
	int error;
	enum nfsstat status;
	struct nfsrnmargs args;
	struct vnode *realvp;

	/* LINTED pointer alignment */
	NFSLOG(2, "nfs_rename from %s %x ", vtomi(odvp)->mi_hostname, odvp);
	/* LINTED pointer alignment */
	NFSLOG(2, "'%s' to %s ", onm, vtomi(ndvp)->mi_hostname);
	NFSLOG(2, "%x '%s'\n", ndvp, nnm);

	if (VOP_REALVP(ndvp, &realvp) == 0)
		ndvp = realvp;

	if (!strcmp(onm, ".") || !strcmp(onm, "..") || !strcmp(nnm, ".") ||
	    !strcmp (nnm, "..")) {
		error = EINVAL;
	} else {
		/* LINTED pointer alignment */
		rlock(vtor(odvp));
		dnlc_remove(odvp, onm);
		dnlc_remove(ndvp, nnm);
		if (ndvp != odvp) {
			/* LINTED pointer alignment */
			rlock(vtor(ndvp));
		}
		setdiropargs(&args.rna_from, onm, odvp);
		setdiropargs(&args.rna_to, nnm, ndvp);
		/* LINTED pointer alignment */
		error = rfscall(vtomi(odvp), RFS_RENAME, 0, xdr_rnmargs,
		    (caddr_t)&args, xdr_enum, (caddr_t)&status, cred);
		/* LINTED pointer alignment */
		PURGE_ATTRCACHE(odvp);	/* mod time changed */
		/* LINTED pointer alignment */
		PURGE_ATTRCACHE(ndvp);	/* mod time changed */
		/* LINTED pointer alignment */
		runlock(vtor(odvp));
		if (ndvp != odvp) {
			/* LINTED pointer alignment */
			runlock(vtor(ndvp));
		}
		if (!error) {
			error = geterrno(status);
			PURGE_STALE_FH(error, odvp);
			PURGE_STALE_FH(error, ndvp);
		}
	}
	NFSLOG(4, "nfs_rename returning %d\n", error, 0);
	return (error);
}

STATIC int
nfs_mkdir(dvp, nm, va, vpp, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *va;
	struct vnode **vpp;
	struct cred *cred;
{
	int error;
	struct nfscreatargs args;
	struct nfsesvcreatargs cargs;
	struct nfsdiropres *dr;
	struct nfsesvdiropres *cdr;
	/* LINTED pointer alignment */
	register struct mntinfo *mi = vtomi(dvp);

	NFSLOG(2, "nfs_mkdir %s %x ", mi->mi_hostname, dvp);
	NFSLOG(2, "'%s'\n", nm, 0);
	dr = (struct  nfsdiropres *)kmem_alloc(sizeof (*dr), KM_SLEEP);
	cdr = (struct  nfsesvdiropres *)kmem_alloc(sizeof (*cdr), KM_SLEEP);
	if (mi->mi_protocol == NFS_V2)
		setdiropargs(&args.ca_da, nm, dvp);
	else
		setdiropargs(&cargs.ca_da, nm, dvp);

	/*
	 * Decide what the group-id and set-gid bit of the created directory
	 * should be.  May have to do a setattr to get the gid right.
	 */
	va->va_gid = (short) setdirgid(dvp);
	va->va_mode = (u_short) setdirmode(dvp, va->va_mode);
	/* also set the uid */
	va->va_uid = cred->cr_uid;

	if (mi->mi_protocol == NFS_V2)
		vattr_to_sattr(va, &args.ca_sa);
	else
		/*
		 * XXX When ACL's are supported we need to deal with
		 * default ACL's here !!!
		 */
		vattr_to_esvsattr(va, &cargs.ca_sa, &mi->mi_addr, &cred->cr_lid,
				  NULL, 0);
	/* LINTED pointer alignment */
	rlock(vtor(dvp));
	dnlc_remove(dvp, nm);
	if (mi->mi_protocol == NFS_V2)
		error = rfscall(mi, RFS_MKDIR, 0, xdr_creatargs, (caddr_t)&args,
		    xdr_diropres, (caddr_t)dr, cred);
	else
		error = rfscall(mi, RFS_MKDIR, 0, xdr_esvcreatargs, (caddr_t)&cargs,
		    xdr_esvdiropres, (caddr_t)cdr, cred);
	/* LINTED pointer alignment */
	PURGE_ATTRCACHE(dvp);	/* mod time changed */
	/* LINTED pointer alignment */
	runlock(vtor(dvp));
	if (!error) {
		error = geterrno((mi->mi_protocol == NFS_V2) ? dr->dr_status : cdr->dr_status);
		PURGE_STALE_FH(error, dvp);
	}
	if (!error) {
		short gid;

		/*
		 * Due to a pre-4.0 server bug the attributes that come back
		 * on mkdir are not correct. Use them only to set the vnode
		 * type in makenfsnode.
		 */
		if (mi->mi_protocol == NFS_V2)
			*vpp = makenfsnode(&dr->dr_fhandle, &dr->dr_attr,
					   dvp->v_vfsp);
		else
			*vpp = makeesvnfsnode(&cdr->dr_fhandle, &cdr->dr_attr,
					      dvp->v_vfsp);
		/* LINTED pointer alignment */
		PURGE_ATTRCACHE(*vpp);

		/* LINTED pointer alignment */
		if (nfs_dnlc && !(vtor(*vpp)->r_flags & (REFFMLD|RMLD))) {
			dnlc_enter(dvp, nm, *vpp, cred);
		}

		/*
		 * Make sure the gid was set correctly.
		 * If not, try to set it (but don't lose
		 * any sleep over it).
		 */
		gid = va->va_gid;
		if (mi->mi_protocol == NFS_V2)
			nattr_to_vattr(*vpp, &dr->dr_attr, va);
		else {
			/* LINTED pointer alignment */
			vtor(*vpp)->r_aclcnt = acl_getmax();
			esvnattr_to_vattr(*vpp, &cdr->dr_attr, va,
					  /* LINTED pointer alignment */
					  &(*vpp)->v_lid, vtor(*vpp)->r_acl,
					  /* LINTED pointer alignment */
					  &vtor(*vpp)->r_aclcnt);
		}
		if (gid != va->va_gid) {
			va->va_mask = AT_GID;
			va->va_gid = gid;
			(void) nfs_setattr(*vpp, va, 0, cred);
		}
	} else {
		*vpp = (struct vnode *)0;
	}
	kmem_free((caddr_t)dr, sizeof(*dr));
	kmem_free((caddr_t)cdr, sizeof(*cdr));
	NFSLOG(4, "nfs_mkdir returning %d\n", error, 0);
	return (error);
}

/* ARGSUSED */
STATIC int
nfs_rmdir(dvp, nm, cdir, cred)
	struct vnode *dvp;
	char *nm;
	struct vnode *cdir;
	struct cred *cred;
{
	int error;
	enum nfsstat status;
	struct nfsdiropargs da;
	struct vnode *vp;

	/* LINTED pointer alignment */
	NFSLOG(2, "nfs_rmdir %s %x ", vtomi(dvp)->mi_hostname, dvp);
	NFSLOG(2, "'%s'\n", nm, 0);
	/*
	 *	Attempt to prevent a rmdir(".") from succeeding
	 */
	if (error = VOP_LOOKUP(dvp, nm, &vp, (struct pathname *) 0, 0, (struct vnode *) 0, cred))
		return(error);
	else {
			if (VN_CMP(vp, cdir)) {
				VN_RELE(vp);
				return (EINVAL);
			}
			else
				VN_RELE(vp);
	}

	setdiropargs(&da, nm, dvp);
	/* LINTED pointer alignment */
	rlock(vtor(dvp));
	dnlc_purge_vp(dvp);
	/* LINTED pointer alignment */
	error = rfscall(vtomi(dvp), RFS_RMDIR, 0, xdr_diropargs, (caddr_t)&da,
	    xdr_enum, (caddr_t)&status, cred);
	/* LINTED pointer alignment */
	PURGE_ATTRCACHE(dvp);	/* mod time changed */
	/* LINTED pointer alignment */
	runlock(vtor(dvp));
	if (!error) {
		error = geterrno(status);
		if (error == NFSERR_NOTEMPTY)
			error = NFSERR_EXIST;
		PURGE_STALE_FH(error, dvp);
	}
	NFSLOG(4, "nfs_rmdir returning %d\n", error, 0);
	return (error);
}

STATIC int
nfs_symlink(dvp, lnm, tva, tnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *tva;
	char *tnm;
	struct cred *cred;
{
	int error;
	struct nfsslargs args;
	struct nfsesvslargs cargs;
	enum nfsstat status;
	/* LINTED pointer alignment */
	register struct mntinfo *mi = vtomi(dvp);

	NFSLOG(2, "nfs_symlink %s %x ", mi->mi_hostname, dvp);
	NFSLOG(2, "'%s' to '%s'\n", lnm, tnm);
	if (mi->mi_protocol == NFS_V2) {
		setdiropargs(&args.sla_from, lnm, dvp);
		vattr_to_sattr(tva, &args.sla_sa);
		args.sla_sa.sa_uid = u.u_cred->cr_uid;
		args.sla_sa.sa_gid = setdirgid(dvp);
		if (!(tva->va_mask & AT_SIZE))
			args.sla_sa.sa_size = (u_long)-1;
		if (!(tva->va_mask & AT_ATIME)) {
			args.sla_sa.sa_atime.tv_sec = (u_long)-1;
			args.sla_sa.sa_atime.tv_usec = (u_long)-1;
		}
		if (!(tva->va_mask & AT_MTIME)) {
			args.sla_sa.sa_mtime.tv_sec = (u_long)-1;
			args.sla_sa.sa_mtime.tv_usec = (u_long)-1;
		}
		args.sla_tnm = tnm;
		error = rfscall(mi, RFS_SYMLINK, 0, xdr_slargs,
			      (caddr_t)&args, xdr_enum, (caddr_t)&status, cred);
	} else {
		setdiropargs(&cargs.sla_from, lnm, dvp);
		vattr_to_esvsattr(tva, &cargs.sla_sa, &mi->mi_addr,
				  &cred->cr_lid, NULL, 0);
		cargs.sla_sa.sa_uid = u.u_cred->cr_uid;
		cargs.sla_sa.sa_gid = setdirgid(dvp);
		if (!(tva->va_mask & AT_SIZE))
			cargs.sla_sa.sa_size = (u_long)-1;
		if (!(tva->va_mask & AT_ATIME)) {
			cargs.sla_sa.sa_atime.tv_sec = (u_long)-1;
			cargs.sla_sa.sa_atime.tv_usec = (u_long)-1;
		}
		if (!(tva->va_mask & AT_MTIME)) {
			cargs.sla_sa.sa_mtime.tv_sec = (u_long)-1;
			cargs.sla_sa.sa_mtime.tv_usec = (u_long)-1;
		}
		cargs.sla_tnm = tnm;
		error = rfscall(mi, RFS_SYMLINK, 0, xdr_esvslargs,
			     (caddr_t)&cargs, xdr_enum, (caddr_t)&status, cred);
	}
	/* LINTED pointer alignment */
	PURGE_ATTRCACHE(dvp);	/* mod time changed */
	if (!error) {
		error = geterrno(status);
		PURGE_STALE_FH(error, dvp);
	}
	NFSLOG(4, "nfs_sysmlink: returning %d\n", error, 0);
	return (error);
}

/*
 * Read directory entries.
 * There are some weird things to look out for here.  The uio_offset
 * field is either 0 or it is the offset returned from a previous
 * readdir.  It is an opaque value used by the server to find the
 * correct directory block to read.  The byte count must be at least
 * vtoblksz(vp) bytes.  The count field is the number of blocks to
 * read on the server.  This is advisory only, the server may return
 * only one block's worth of entries.  Entries may be compressed on
 * the server.
 */
STATIC int
nfs_readdir(vp, uiop, cred, eofp)
	struct vnode *vp;
	register struct uio *uiop;
	struct cred *cred;
	int *eofp;
{
	int error = 0;
	struct iovec *iovp;
	unsigned alloc_count, count;
	struct nfsrddirargs rda;
	struct nfsrddirres  rd;
	struct nfsesvrddirres crd;
	struct rnode *rp;
	/* LINTED pointer alignment */
	struct mntinfo *mi = vtomi(vp);

	if (eofp)
		*eofp = 0;
	/* LINTED pointer alignment */
	rp = vtor(vp);

	/*
	 *	N.B.:  It appears here that we're treating the directory
	 *	cookie as an offset.  Not true.  It's simply that getdents
	 *	passes us the cookie to use in the uio_offset field of a
	 *	uio structure.
	 */
	if ((rp->r_flags & REOF) &&
	    (rp->r_lastcookie == (u_long)uiop->uio_offset)) {
		if (eofp)
			*eofp = 1;
		return (0);
	}
	iovp = uiop->uio_iov;
	alloc_count = count = iovp->iov_len;

	/*
	 *	UGLINESS: SunOS 3.2 servers apparently cannot always handle an
	 *	RFS_READDIR request with rda_count set to more than 0x400. So
	 *	we reduce the request size here purely for compatibility.
	 */
	if (count > 0x400)
		count = 0x400;

	NFSLOG(2, "nfs_readdir %s %x ", mi->mi_hostname, vp);
	NFSLOG(2, "count %d offset %d\n", count, uiop->uio_offset);
	/*
	 * XXX We should do some kind of test for count >= DEV_BSIZE
	 */
	if (uiop->uio_iovcnt != 1) {
		return (EINVAL);
	}

	/* MAC cred handling: see nfs_rdwr() */
	cred = crdup(cred);
	cred->cr_lid = u.u_procp->p_cred->cr_lid;
	cred->cr_wkgpriv = u.u_procp->p_cred->cr_wkgpriv;
	cred->cr_maxpriv = u.u_procp->p_cred->cr_maxpriv;

	rda.rda_offset = uiop->uio_offset;
	rd.rd_entries = (struct dirent *)kmem_alloc(alloc_count, KM_SLEEP);
	crd.rd_entries = (struct dirent *)kmem_alloc(alloc_count, KM_SLEEP);
	/* LINTED pointer alignment */
	rda.rda_fh = *vtofh(vp);
	do {
		count = MIN(count, mi->mi_curread);
		rda.rda_count = count;
		rd.rd_size = count;
		crd.rd_size = count;

		if (mi->mi_protocol == NFS_V2)
			error = rfscall(mi, RFS_READDIR, 0, xdr_rddirargs,
				(caddr_t)&rda, xdr_getrddirres, (caddr_t)&rd, cred);
		else
			error = rfscall(mi, RFS_READDIR, 0, xdr_rddirargs,
				(caddr_t)&rda, xdr_esvgetrddirres, (caddr_t)&crd, cred);
	} while (error == ENFS_TRYAGAIN);

	crfree(cred);

	if (!error) {
		error = geterrno((mi->mi_protocol == NFS_V2) ? rd.rd_status : crd.rd_status);
		PURGE_STALE_FH(error, vp);
	}
	if (!error) {
		/*
		 * move dir entries to user land
		 */
		if (mi->mi_protocol == NFS_V2) {
			if (rd.rd_size) {
				error = uiomove((caddr_t)rd.rd_entries,
				    (int)rd.rd_size, UIO_READ, uiop);
				rda.rda_offset = rd.rd_offset;
				uiop->uio_offset = rd.rd_offset;
			}
			if (rd.rd_eof) {
				rp->r_flags |= REOF;
				rp->r_lastcookie = uiop->uio_offset;
				if (!error && eofp)
					*eofp = 1;
			}
		} else {
			timestruc_t mtime;
			if (crd.rd_size) {
				error = uiomove((caddr_t)crd.rd_entries,
				    (int)crd.rd_size, UIO_READ, uiop);
				rda.rda_offset = crd.rd_offset;
				uiop->uio_offset = crd.rd_offset;
			}
			if (crd.rd_eof) {
				rp->r_flags |= REOF;
				rp->r_lastcookie = uiop->uio_offset;
				if (!error && eofp)
					*eofp = 1;
			}
			mtime.tv_sec = crd.rd_attr.na_mtime.tv_sec;
			mtime.tv_nsec = crd.rd_attr.na_mtime.tv_usec * 1000;
			nfs_cache_check(vp, mtime);
			nfs_esvattrcache(vp, &crd.rd_attr);
		}
	}
	kmem_free((caddr_t)rd.rd_entries, alloc_count);
	kmem_free((caddr_t)crd.rd_entries, alloc_count);
	NFSLOG(4, "nfs_readdir: returning %d resid %d, ",
	    error, uiop->uio_resid);
	NFSLOG(4, "offset %d\n", uiop->uio_offset, 0);
	return (error);
}

STATIC struct buf async_bufhead = {
	B_HEAD,
	(struct buf *)NULL, (struct buf *)NULL,
	&async_bufhead, &async_bufhead,
};

int async_daemon_ready;		/* number of async biod's ready */
int async_daemon_count;		/* number of existing biod's */

#ifndef	SYSV
int nfs_wakeup_one_biod = 1;
#endif

STATIC int
nfs_strategy(bp)
	register struct buf *bp;
{
	NFSLOG(2, "nfs_strategy bp %x lbn %d\n", bp, bp->b_blkno);

	if (async_daemon_ready > 0 && (bp->b_flags & B_ASYNC)) {

		binstailfree(bp, &async_bufhead);

		async_daemon_ready--;
#ifndef	SYSV
		if (nfs_wakeup_one_biod == 1)	{
			wakeup_one((caddr_t)&async_bufhead);
		} else {
			wakeprocs((caddr_t)&async_bufhead, PRMPT);
		}
#else
		wakeprocs((caddr_t)&async_bufhead, PRMPT);
#endif
		return (0);
	} else {
		return (do_bio(bp));
	}
}

async_daemon()
{
	register struct buf *bp;
	struct rnode *rp;
	struct cred *cred;

	/*
	 * First release resources. In some implementations, relvm
	 * may not actually free the address space Hence the flag ISADAEMON.
	 */
	relvm(u.u_procp, ISADAEMON);

	async_daemon_count++;
	if (setjmp(&u.u_qsav)) {
		if (async_daemon_count == 0) {
			/*
			 * We already were processing requests below
			 * and we were signaled again.  So this time,
			 * just give up and abort all the requests.
			 */
			while ((bp = async_bufhead.b_actf) != &async_bufhead) {
				bremfree(bp);
				bp->b_flags |= B_ERROR;
				/*
				 * Since we are always ASYNC pvn_done
				 * will free the buf.
				 */
				pvn_done(bp);
			}
		} else {
			async_daemon_count--;
			async_daemon_ready--;
			/*
			 * If we were the last async daemon,
			 * process all the queued requests.
			 */
			if (async_daemon_count == 0) {
				while ((bp = async_bufhead.b_actf) !=
				    &async_bufhead) {
					bremfree(bp);
					/* LINTED pointer alignment */
					rp = vtor(bp->b_vp);
					/*
					 * Since we are ASYNC do_bio will
					 * free the bp.
					 */
					cred = rp->r_cred;
					crhold(cred);
					rp->r_error = do_bio(bp);
					crfree(cred);
					if (rp->r_error == NFS_CACHEINVALERR) {
						nfs_purge_caches(rtov(rp));
						rp->r_error = 0;
					} else if (rp->r_error == NFS_EOF) {
						rp->r_error = 0;
					}
				}
			}
		}
		{
			struct proc *p = u.u_procp;
			p->p_cursig = 0;
			if (p->p_curinfo) {
				kmem_free((caddr_t)p->p_curinfo,
					sizeof(*p->p_curinfo));
				p->p_curinfo = NULL;
			}
		}
		exit(CLD_EXITED, 0);
	}

	for (;;) {
		async_daemon_ready++;
		while ((bp = async_bufhead.b_actf) == &async_bufhead) {
			(void) sleep((caddr_t)&async_bufhead, PZERO + 1);
		}
		bremfree(bp);
		/* LINTED pointer alignment */
		rp = vtor(bp->b_vp);
		cred = rp->r_cred;
		crhold(cred);
		rp->r_error = do_bio(bp);
		crfree(cred);
		if (rp->r_error == NFS_CACHEINVALERR) {
			nfs_purge_caches(rtov(rp));
			rp->r_error = 0;
		} else if (rp->r_error == NFS_EOF) {
			rp->r_error = 0;
		}
	}
	/* NOTREACHED */
}

STATIC int
do_bio(bp)
	register struct buf *bp;
{
	/* LINTED pointer alignment */
	register struct rnode *rp = vtor(bp->b_vp);
	struct vattr va;
	long count;
	int error;
	int read, async;

	read = bp->b_flags & B_READ;
	async = bp->b_flags & B_ASYNC;
	NFSLOG(2, "do_bio: addr %x, blk %d, ", bp->b_un.b_addr, bp->b_blkno);
	NFSLOG(2, "offset %d, size %d, ", dbtob(bp->b_blkno), bp->b_bcount);
	NFSLOG(2, "B_READ %x B_ASYNC %x\n", read, async);

	if (read) {
		error = bp->b_error = nfsread(bp->b_vp, bp->b_un.b_addr,
		    (u_int)dbtob(bp->b_blkno), (long)bp->b_bcount,
		    (long *)&bp->b_resid, rp->r_cred, &va);
		if (!error) {
			if (bp->b_resid) {
				/*
				 * Didn't get it all because we hit EOF,
				 * zero all the memory beyond the EOF.
				 */
				bzero(bp->b_un.b_addr +
				    (bp->b_bcount - bp->b_resid),
				    (u_int)bp->b_resid);
			}
			if (bp->b_resid == bp->b_bcount &&
			    dbtob(bp->b_blkno) >= rp->r_size) {
				/*
				 * We didn't read anything at all as we are
				 * past EOF.  Return an error indicator back
				 * but don't destroy the pages (yet).
				 */
				error = NFS_EOF;
			}
		}
	} else {
		/*
		 * If the write fails and it was asynchronous
		 * all future writes will get an error.
		 */
		if (rp->r_error == 0) {
			count = MIN(bp->b_bcount, rp->r_size -
			    dbtob(bp->b_blkno));
			if (count < 0) {
				cmn_err(CE_PANIC, "do_bio: write count < 0");
			}
			error = bp->b_error = nfswrite(bp->b_vp,
			    bp->b_un.b_addr, (u_int)dbtob(bp->b_blkno),
			    count, rp->r_cred);
		} else
			error = bp->b_error = rp->r_error;
	}

	if (!error && read) {
		if (!CACHE_VALID(rp, va.va_mtime)) {
			/*
			 * read, if cache is not valid mark this bp
			 * with an error so it will be freed by pvn_done
			 * and return a special error, NFS_CACHEINVALERR,
			 * so caller can flush caches and re-do the operation.
			 */
			error = NFS_CACHEINVALERR;
			bp->b_error = EINVAL;
		} else {
			nfs_attrcache_va(rtov(rp), &va);
		}
	}

	if (error != 0 && error != NFS_EOF) {
		bp->b_flags |= B_ERROR;
	}

	/*
	 * Call pvn_done() to free the bp and pages.  If not ASYNC
	 * then we have to call pageio_done() to free the bp.
	 */
	pvn_done(bp);
	if (!async) {
		pageio_done(bp);
	} 

	NFSLOG(4, "do_bio: error %d, bp %x ", error, bp);
	NFSLOG(4, "B_READ %x B_ASYNC %d\n", read, async);
	return (error);
}

/* ARGSUSED */
STATIC int
nfs_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	return (ENOSYS);
}

STATIC void
nfs_rwlock(vp)
	struct vnode *vp;
{
	/* LINTED pointer alignment */
	RLOCK(vtor(vp));
}

STATIC void
nfs_rwunlock(vp)
	struct vnode *vp;
{
	/* LINTED pointer alignment */
	RUNLOCK(vtor(vp));
}

/* ARGSUSED */
STATIC int
nfs_seek(vp, ooff, noffp)
	struct vnode *vp;
	off_t ooff;
	off_t *noffp;
{
	return *noffp < 0 ? EINVAL : 0;
}

int nfs_nra = 1;	/* number of pages to read ahead */
int nfs_lostpage;	/* number of times we lost original page */

/*
 * Called from pvn_getpages or nfs_getpage to get a particular page.
 * When we are called the rnode has already locked by nfs_getpage.
 */
/*ARGSUSED*/
STATIC int
nfs_getapage(vp, off, protp, pl, plsz, seg, addr, rw, cred)
	struct vnode *vp;
	u_int off;
	u_int *protp;
	struct page *pl[];		/* NULL if async IO is requested */
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct cred *cred;
{
	register struct rnode *rp;
	register u_int bsize;
	struct buf *bp;
	struct page *pp, *pp2, **ppp, *pagefound;
	daddr_t lbn;
	u_int io_off, io_len;
	u_int blksize, blkoff;
	int err;
	int readahead;
	/* int pgaddr; */

	/* LINTED pointer alignment */
	rp = vtor(vp);
	NFSLOG(2, "nfs_getapage: vp %x size %d ", vp, rp->r_size);
	NFSLOG(2, "off %d pl %x ", off, pl);
	NFSLOG(2, "addr %x\n", addr, 0);
	bsize = vp->v_vfsp->vfs_bsize;
reread:
	err = 0;
	lbn = off / bsize;
	blkoff = lbn * bsize;

#ifdef	VNOCACHE
	if (rp->r_nextr == off && !(vp->v_flag & VNOCACHE)) {
		readahead = nfs_nra;
	} else {
		readahead = 0;
	}
#else
	if (rp->r_nextr == off) {
		readahead = nfs_nra;
	} else {
		readahead = 0;
	}
#endif

	NFSLOG(8, "nfs_getapage: nextr %d off %d ", rp->r_nextr, off);
	NFSLOG(8, "size %d ra %d ", rp->r_size, readahead);

again:
	if ((pagefound = page_find(vp, off)) == NULL) {
		/*
		 * Need to go to server to get a block
		 */

		if (blkoff < rp->r_size && blkoff + bsize > rp->r_size) {
			/*
			 * If less than a block left in
			 * file read less than a block.
			 */
			if (rp->r_size <= off) {
				/*
				 * Trying to access beyond EOF,
				 * set up to get at least one page.
				 */
				blksize = off + PAGESIZE - blkoff;
			} else {
				blksize = rp->r_size - blkoff;
			}
		} else {
			blksize = bsize;
		}

		pp = pvn_kluster(vp, off, seg, addr, &io_off, &io_len,
		    blkoff, blksize, 0);
		/*
		 * If someone else got there first, try again.
		 */
		if (pp == NULL)
			goto again;

		if (pl != NULL) {
			register int sz;

			if (plsz >= io_len) {
				/*
				 * Everything fits, set up to load
				 * up and hold all the pages.
				 */
				pp2 = pp;
				sz = io_len;
			} else {
				/*
				 * Set up to load plsz worth
				 * starting at the needed page.
				 */
				for (pp2 = pp; pp2->p_offset != off;
				    pp2 = pp2->p_next) {
					ASSERT(pp2->p_next->p_offset !=
					    pp->p_offset);
				}
                                if (sz > plsz)
					sz = plsz;
			}

			ppp = pl;
			do {
				PAGE_HOLD(pp2);
				*ppp++ = pp2;
				pp2 = pp2->p_next;
				sz -= PAGESIZE;
			} while (sz > 0);
			*ppp = NULL;		/* terminate list */
		}

		/*
		 * Now round the request size up to page boundaries.
		 * This insures that the entire page will be
		 * initialized to zeroes if EOF is encountered.
		 */
		io_len = ptob(btopr(io_len));

		bp = pageio_setup(pp, io_len, vp, pl == NULL ?
		    (B_ASYNC | B_READ) : B_READ);

		bp->b_blkno = btodb(io_off);
		bp->b_dev = 0;
		bp->b_edev = 0;
		/* pgaddr = pfntokv (page_pptonum(pp)); */
		/* bp->b_un.b_addr = (caddr_t) (pgaddr); */
		bp_mapin(bp);

		/*
		 * If doing a write beyond what we believe is EOF,
		 * don't bother trying to read the pages from the
		 * server, we'll just zero the pages here.  We
		 * don't check that the rw flag is S_WRITE here
		 * because some implementations may attempt a
		 * read access to the buffer before copying data.
		 */
		if (io_off >= rp->r_size && seg == segkmap) {
			bzero(bp->b_un.b_addr, io_len);
			pvn_done(bp);
			if (pl != NULL)
				pageio_done(bp);
		} else {
			err = nfs_strategy(bp);
		}
		/* bp is now invalid! */

		if (err == NFS_EOF) {
			/*
			 * If doing a write system call just return
			 * zeroed pages, else user tried to get pages
			 * beyond EOF, return error.  We don't check
			 * that the rw flag is S_WRITE here because
			 * some implementations may attempt a read
			 * access to the buffer before copying data.
			 */
			if (seg == segkmap) {
				err = 0;
			} else {
				err = EFAULT;
			}
		}

		rp->r_nextr = io_off + io_len;
		cnt.v_pgin++;
		cnt.v_pgpgin += btopr(io_len);
		vminfo.v_pgin++;
		vminfo.v_pgpgin += btopr(io_len);
		NFSLOG(8, "OTW\n", 0, 0);
	} else
		NFSLOG(8, "\n", 0, 0);

	while (!err && readahead > 0 && (blkoff + bsize < rp->r_size)) {
		addr_t addr2;

		readahead--;
		lbn++;
		blkoff += bsize;
		addr2 = addr + (blkoff - off);

		if (blkoff < rp->r_size && blkoff + bsize > rp->r_size) {
			/*
			 * If less than a block left in
			 * file read less than a block.
			 */
			blksize = rp->r_size - blkoff;
		} else {
			blksize = bsize;
		}

		/*
		 * If addr is now in a different seg,
		 * don't bother trying with read-ahead.
		 */
		if (addr2 >= seg->s_base + seg->s_size) {
			pp2 = NULL;
			NFSLOG(8, "nfs_getapage: ra out of seg\n", 0, 0);
		} else {
			pp2 = pvn_kluster(vp, blkoff, seg, addr2,
			    &io_off, &io_len, blkoff, blksize, 1);
			if (pp2 == NULL) {
				NFSLOG(8, "nfs_getapage: RA CACHE off %d size %d\n",
				    off, rp->r_size);
			}
		}

		if (pp2 != NULL) {
			/*
			 * Now round the request size up to page boundaries.
			 * This insures that the entire page will be
			 * initialized to zeroes if EOF is encountered.
			 */
			io_len = ptob(btopr(io_len));

			bp = pageio_setup(pp2, io_len, vp, B_READ | B_ASYNC);

			bp->b_dev = 0;
			bp->b_edev = 0;
			bp->b_blkno = btodb(io_off);
			/* pgaddr = pfntokv (page_pptonum(pp2)); */
			/* bp->b_un.b_addr = (caddr_t) (pgaddr + bsize); */
			bp_mapin(bp);

			NFSLOG(8, "nfs_getapage: RA OTW off %d size %d\n",
			    off, rp->r_size);
			err = nfs_strategy(bp);	/* bp is now invalid! */

			/*
			 * Ignore all read ahead errors except those
			 * that might invalidate the primary read.
			 */
			if (err != NFS_EOF && err != NFS_CACHEINVALERR) {
				err = 0;
			}

			cnt.v_pgin++;
			cnt.v_pgpgin += btopr(io_len);
			vminfo.v_pgin++;
			vminfo.v_pgpgin += btopr(io_len);
		}
	}

	if (pagefound != NULL) {
		int s;
		NFSLOG(8, "CACHE\n", 0, 0);
		/*
		 * We need to be careful here because if the page was
		 * previously on the free list, we might have already
		 * lost it at interrupt level.
		 */
		s = splvm();
		if (pagefound->p_vnode == vp && pagefound->p_offset == off) {
			/*
			 * If the page is intransit or if
			 * it is on the free list call page_lookup
			 * to try and wait for / reclaim the page.
			 */
			if (pagefound->p_intrans || pagefound->p_free)
				pagefound = page_lookup(vp, off);
		}
		(void) splx(s);
		if (pagefound == NULL || pagefound->p_offset != off ||
		    pagefound->p_vnode != vp || pagefound->p_gone) {
			nfs_lostpage++;
			goto reread;
		}
		if (pl != NULL) {
			PAGE_HOLD(pagefound);
			pl[0] = pagefound;
			pl[1] = NULL;
			rp->r_nextr = off + PAGESIZE;
		}
	}

	if (err && pl != NULL) {
		for (ppp = pl; *ppp != NULL; *ppp++ = NULL)
			PAGE_RELE(*ppp);
	}

	PURGE_STALE_FH(err, vp);

	if (err == NFS_EOF || err == NFS_CACHEINVALERR) {
		/*
		 * Cache invalid due to bad attributes or size,
		 * purge caches, and re-do operation.
		 */
		nfs_purge_caches(vp);
		goto reread;
	}

	NFSLOG(4, "nfs_getapage: returning %d\n", err, 0);
	return (err);
}

/*
 * Return all the pages from [off..off+len) in file
 */
STATIC int
nfs_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw, cred)
	struct vnode *vp;
	u_int off, len;
	u_int *protp;
	struct page *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct cred *cred;
{
	/* LINTED pointer alignment */
	struct rnode *rp = vtor(vp);
	int err;

	if (vp->v_flag & VNOMAP)
		return (ENOSYS);

	if (protp != NULL)
		*protp = PROT_ALL;

	RLOCK(rp);

	if (rp->r_cred == NULL) {
		if (cred == NULL) {
			cred = u.u_cred;	/* XXX need real cred! */
		}
		crhold(cred);
		rp->r_cred = cred;
	}

	/*
	 * Now valididate that the caches are up to date.
	 */
	(void) nfs_validate_caches(vp, rp->r_cred);

	/*
	 * If we are getting called as a side effect of a nfs_rdwr()
	 * write operation the local file size might not be extended yet.
	 * In this case we want to be able to return pages of zeroes.
	 */
	if (off + len > rp->r_size + PAGEOFFSET && seg != segkmap) {
		RUNLOCK(rp);
		return (EFAULT);		/* beyond EOF */
	}

	if (len <= PAGESIZE)
		err = nfs_getapage(vp, off, protp, pl, plsz, seg, addr,
		    rw, cred);
	else
		err = pvn_getpages(nfs_getapage, vp, off, len, protp, pl, plsz,
		    seg, addr, rw, cred);
	RUNLOCK(rp);

	return (err);
}

/*ARGSUSED*/
STATIC int
nfs_putpage(vp, off, len, flags, cred)
	struct vnode *vp;
	u_int off;
	u_int len;
	int flags;
	struct cred *cred;
{
	register struct rnode *rp;
	register struct page *pp;
	struct page *dirty, *io_list;
	register u_int io_off, io_len;
	daddr_t lbn;
	u_int lbn_off;
	u_int bsize;
	int vpcount;
	int err = 0;

	NFSLOG(2, "nfs_putpage: vp %x, off %d, ", vp, off);
	NFSLOG(2, "len %d, flags %x, ", len, flags);
	NFSLOG(2, "cred %x\n", cred, 0);

	if (vp->v_flag & VNOMAP) {
		NFSLOG(0xc, "nfs_putpage: VNOMAP\n", 0, 0);
		return (ENOSYS);
	}

	if (len == 0 && (flags & B_INVAL) == 0 &&
	    (vp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		NFSLOG(0xc, "nfs_putpage: len == 0 && read-only\n", 0, 0);
		return (0);
	}

	/* LINTED pointer alignment */
	rp = vtor(vp);
	if (vp->v_pages == NULL || off >= rp->r_size) {
		NFSLOG(0xc, "nfs_putpage: no pages or past EOF\n", 0, 0);
		return (0);
	}

        /*
         * Return if an inactive is already in progres on this
	 * rnode since the page will be written out by that process.
	 */
	if ((rp->r_flags & RLOCKED) && (rp->r_owner != curproc->p_pid) &&
            (rp->r_flags & RINACTIVE)) {
		return(EAGAIN);
	}

	bsize = MAX(vp->v_vfsp->vfs_bsize, PAGESIZE);
	vpcount = vp->v_count;
	if (vp->v_count == 0) {
		/* LINTED pointer alignment */
		((struct mntinfo *)(vp->v_vfsp->vfs_data))->mi_refct++;
	}
	VN_HOLD(vp);

again:
	if (len == 0) {
		/*
		 * Call pvn_vplist_dirty to push out all dirty page.
		 */
		pvn_vplist_dirty(vp, off, flags);
		goto out;
	} else {
		/*
		 * Do a range from [off...off + len) via page_find.
		 * We set limits so that we kluster to bsize boundaries.
		 */
		if (off >= rp->r_size) {
			dirty = NULL;
		} else {
			u_int fsize, eoff, offlo, offhi;

			fsize = (rp->r_size + PAGEOFFSET) & PAGEMASK;
			eoff = MIN(off + len, fsize);
			offlo = (off / bsize) * bsize;
			offhi = roundup(eoff, bsize);
			dirty = pvn_range_dirty(vp, off, eoff, offlo, offhi,
			    flags);
		}
	}

	/*
	 * Destroy read ahead value (since we are really going to write)
	 */

	if (dirty != NULL)
		rp->r_nextr = 0;

	/*
	 * Now pp will have the list of kept dirty pages marked for
	 * write back.  It will also handle invalidation and freeing
	 * of pages that are not dirty.  All the pages on the list
	 * returned need to still be dealt with here.
	 */

	/*
	 * Handle all the dirty pages not yet dealt with.
	 */
	while ((pp = dirty) != NULL) {
		/*
		 * Pull off a contiguous chunk that fits in one lbn
		 */
		io_off = pp->p_offset;
		lbn = io_off / bsize;

		page_sub(&dirty, pp);
		io_list = pp;
		io_len = PAGESIZE;
		lbn_off = lbn * bsize;

		while (dirty != NULL && dirty->p_offset < lbn_off + bsize &&
		    dirty->p_offset == io_off + io_len) {
			pp = dirty;
			page_sub(&dirty, pp);
			page_sortadd(&io_list, pp);
			io_len += PAGESIZE;
		}

		/*
		 * Check for page length rounding problems
		 */
		if (io_off + io_len > lbn_off + bsize) {
			ASSERT((io_off+io_len) - (lbn_off+bsize) < PAGESIZE);
			io_len = lbn_off + bsize - io_off;
		}

		err = nfs_writelbn(rp, io_list, io_off, io_len, flags, cred);
		if (err)
			break;
	}

	if (err != 0) {
		if (dirty != NULL)
			pvn_fail(dirty, B_WRITE | flags);
	} else if (off == 0 && len >= rp->r_size) {
		/*
		 * If doing "synchronous invalidation", make
		 * sure that all the pages are actually gone.
		 */
		if ((flags & (B_INVAL | B_ASYNC)) == B_INVAL &&
                    (!pvn_vpempty(vp)))
			goto again;
	}

out:
	/*
	 * Instead of using VN_RELE here we are careful to only call
	 * the inactive routine if the vnode reference count is now zero,
	 * but it wasn't zero coming into putpage.  This is to prevent
	 * recursively calling the inactive routine on a vnode that
	 * is already considered in the `inactive' state.
	 * XXX - inactive is a relative term here (sigh).
	 */
	if (--vp->v_count == 0) {
		if (vpcount > 0) {
			(void)nfs_inactive(vp, rp->r_cred);
		} else {
			/* LINTED pointer alignment */
			((struct mntinfo *)(vp->v_vfsp->vfs_data))->mi_refct--;
		}
	}
	NFSLOG(0x4, "nfs_putpage: returning %d\n", err, 0);
	return (err);
}

/*ARGSUSED*/
STATIC int
nfs_map(vp, off, as, addrp, len, prot, maxprot, flags, cred)
	struct vnode *vp;
	u_int off;
	struct as *as;
	addr_t *addrp;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct cred *cred;
{
	struct segvn_crargs vn_a;
	int error;

	if ((int)off < 0 || (int)(off + len) < 0)
		return (EINVAL);

	if (vp->v_flag & VNOMAP)
		return (ENOSYS);

	if (vp->v_type != VREG)
		return (ENODEV);

#ifdef	VNOCACHE
	/*
	 * Check to see if the vnode is currently marked as not cachable.
	 * If so, we have to refuse the map request as this violates the
	 * don't cache attribute.
	 */
	if (vp->v_flag & VNOCACHE)
		return (EIO);
#endif

	if ((flags & MAP_FIXED) == 0) {
		map_addr(addrp, len, (off_t)off, 1);
		if (*addrp == NULL)
			return (ENOMEM);
	} else {
		/*
		 * User specified address - blow away any previous mappings
		 */
		(void) as_unmap(as, *addrp, len);
	}

	/* MAC cred handling: see nfs_rdwr() */
	cred = crdup(cred);
	cred->cr_lid = u.u_procp->p_cred->cr_lid;
	cred->cr_wkgpriv = u.u_procp->p_cred->cr_wkgpriv;
	cred->cr_maxpriv = u.u_procp->p_cred->cr_maxpriv;

	vn_a.vp = vp;
	vn_a.offset = off;
	vn_a.type = flags & MAP_TYPE;
	vn_a.prot = (u_char)prot;
	vn_a.maxprot = (u_char)maxprot;
	vn_a.cred = cred;
	vn_a.amp = NULL;

	error = as_map(as, *addrp, len, segvn_create, (caddr_t)&vn_a);
	crfree(cred);
	return (error);
}

/*ARGSUSED*/
STATIC int
nfs_addmap(vp, off, as, addr, len, prot, maxprot, flags, cred)
	struct vnode *vp;
	u_int off;
	struct as *as;
	addr_t addr;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct cred *cred;
{
	if (vp->v_flag & VNOMAP)
		return (ENOSYS);
	/* do nothing - nfs does not keep mapping counts */
	return 0;
}

/* ARGSUSED */
STATIC int
nfs_frlock(vp, cmd, bfp, flag, offset, cr)
	struct vnode *vp;
	int cmd;
	struct flock *bfp;
	int flag;
	off_t offset;
	struct cred *cr;
{
	struct vattr va;
	int error, unlk=0;
	lockhandle_t lh;
	short whence;

	NFSLOG(2, "nfs_frlock(): cmd %d, flag %d, ", cmd, flag);
	NFSLOG(2, "offset %d, start %d, ", offset, bfp->l_start);
	NFSLOG(2, "len %d, whence %d\n", bfp->l_len, bfp->l_whence);

	if (cmd != F_GETLK && cmd != F_SETLK && cmd != F_SETLKW) {
		NFSLOG(1, "nfs_frlock: Invalid command %d\n", cmd, 0);
		return EINVAL;
	}

	switch (bfp->l_type) {
                case F_RDLCK:
                        if (!((cmd == F_GETLK) || (cmd == F_RGETLK)) &&
                                !(flag & FREAD)) {
                                return EBADF;
                        }
                        break;
 
                case F_WRLCK:
                        if (!((cmd == F_GETLK) || (cmd == F_RGETLK)) &&
                                !(flag & FWRITE)) {
                                return EBADF;
                        }
                        break;
 
                case F_UNLCK:
			unlk = 1;
                        break;

		default:
			return EINVAL;
        } 

#ifdef VNOCACHE
	/*
	 * If we are setting a lock mark the vnode VNOCACHE so the page
	 * cache does not give inconsistent results on locked files shared
	 * between clients.  The VNOCACHE flag is never turned off as long
	 * as the vnode is active because it is hard to figure out when the
	 * last lock is gone.
	 * XXX - what if some already has the vnode mapped in?
	 */
	if (((vp->v_flag & VNOCACHE) == 0) &&
	    (bfp->l_type != F_UNLCK) && (cmd != F_GETLK)) {
		vp->v_flag |= VNOCACHE;
		vtor(vp)->r_error = nfs_putpage(vp, 0, 0, B_INVAL, cred);
	}
#endif

	/* Convert start to be relative to beginning of file */
	whence = bfp->l_whence;
	if (error = convoff(vp, bfp, 0, offset)) {
		NFSLOG(1, "nfs_frlock: convoff: error %d\n", error, 0);
		return (error);
	}

        /* set pid and sysid into flock struct */
	bfp->l_pid = u.u_procp->p_epid;
	bfp->l_sysid = u.u_procp->p_sysid;

	lh.lh_vp = vp;
	/* LINTED pointer alignment */
	lh.lh_servername = vtomi(vp)->mi_hostname;
	/* LINTED pointer alignment */
	bcopy((caddr_t)vtofh(vp), (caddr_t)&lh.lh_id, sizeof (fhandle_t));

	error = klm_lockctl(&lh, bfp, cmd, cr, u.u_procp->p_epid);

	/* Force a call to server to get fresh attributes */

	if (!error && !unlk) {
		error = nfs_getattr_otw(vp, &va, cr);
		if (!error) {
			nfs_cache_check(vp, va.va_mtime);
			nfs_attrcache_va(vp, &va);
		}
	}
			
	(void) convoff(vp, bfp, whence, offset);

	if ((!error) && (cmd == F_SETLK || cmd == F_SETLKW) &&
	    (bfp->l_type != F_UNLCK))
		u.u_procp->p_flag |= SLKDONE;

	NFSLOG(4, "NFS_FRLOCK: error=%d\n", error, 0);
	return (error);
}

/*
 * Free storage space associated with the specified vnode.  The portion
 * to be freed is specified by bfp->l_start and bfp->l_len (already
 * normalized to a "whence" of 0).
 *
 * This is an experimental facility whose continued existence is not
 * guaranteed.  Currently, we only support the special case
 * of l_len == 0, meaning free to end of file.
 */
/* ARGSUSED */
STATIC int
nfs_space(vp, cmd, bfp, flag, offset, cr)
	struct vnode *vp;
	int cmd;
	struct flock *bfp;
	int flag;
	off_t offset;
	struct cred *cr;
{
	int error;

	ASSERT(vp->v_type == VREG);
	if (cmd != F_FREESP)
		return (EINVAL);

	if ((error = convoff(vp, bfp, 0, offset)) == 0) {
		ASSERT(bfp->l_start >= 0);
		if (bfp->l_len == 0) {
			struct vattr va;

			va.va_size = bfp->l_start;
			va.va_mask = AT_SIZE;
			error = nfs_setattr(vp, &va, 0, cr);
		}
		else
			error = EINVAL;
	}

	return (error);
}

/*ARGSUSED*/
STATIC int
nfs_realvp(vp, vpp)
	struct vnode *vp;
	struct vnode **vpp;
{
	return (EINVAL);
}

/*ARGSUSED*/
STATIC int
nfs_delmap(vp, off, as, addr, len, prot, maxprot, flags, cr)
	struct vnode *vp;
	u_int off;
	struct as *as;
	addr_t addr;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct cred *cr;
{
	if (vp->v_flag & VNOMAP)
		return (ENOSYS);
	/* nothing to do here - nfs_map doesn't keep reference counts (yet?) */
	return 0;
}

/*
 * Make a multi-level directory. This operation is allowed only if the client is
 * using the ESV protocol. We try to make a MLD, using SA_SETMLD, and send
 * the request remote. If the response is success but NA_TSTMLD finds no MLD,
 * remove the (ordinary) directory we just created and return ENOSYS.
 */
/*ARGSUSED*/
STATIC int
nfs_makemld(dvp, nm, vap, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *vap;
	struct vnode **vpp;
	struct cred *cred;
{
	struct nfsesvcreatargs cargs;
	struct nfsesvdiropres cdr;
	/* LINTED pointer alignment */
	register struct mntinfo *mi = vtomi(dvp);
	int error;

	NFSLOG(2, "nfs_makemld %s, %x ", mi->mi_hostname, dvp);
	NFSLOG(2, "'%s'\n", nm, 0);

	if (mi->mi_protocol != NFS_ESV) {
		NFSLOG(5, "nfs_makemld: not ESV\n", 0, 0);
		return (ENOSYS);
	}

	setdiropargs(&cargs.ca_da, nm , dvp);
	vap->va_gid = (short)setdirgid(dvp);
	vap->va_mode = (u_short)setdirmode(dvp, vap->va_mode);
	vap->va_uid = cred->cr_uid;
	/* XXX - need to deal with default ACL entries here */
	vattr_to_esvsattr(vap, &cargs.ca_sa, &mi->mi_addr, &cred->cr_lid,
			  NULL, 0);
	SA_SETMLD(&cargs.ca_sa);

	/* LINTED pointer alignment */
	rlock(vtor(dvp));
	dnlc_remove(dvp, nm);
	error = rfscall(mi, RFS_MKDIR, 0, xdr_esvcreatargs, (caddr_t)&cargs,
			xdr_esvdiropres, (caddr_t)&cdr, cred);

	/* LINTED pointer alignment */
	PURGE_ATTRCACHE(dvp);	/* mod time changed */
	/* LINTED pointer alignment */
	runlock(vtor(dvp));
	if (!error) {
		error = geterrno(cdr.dr_status);
		PURGE_STALE_FH(error, dvp);
		if (error == EINVAL)
			error = ENOSYS;		/* Many errors may simply mean
						   the remote machine doesn't
						   suuport MLD's */
	}
	if (!error) {
		short gid;

		*vpp = makeesvnfsnode(&cdr.dr_fhandle, &cdr.dr_attr, dvp->v_vfsp);
		/* LINTED pointer alignment */
		PURGE_ATTRCACHE(dvp);
		if (!NA_TSTMLD(&cdr.dr_attr)) {
			/* created an ordinary directory. Remove it and return ENOSYS */
			struct nfsdiropargs rmda;
			enum nfsstat rmstatus;

			setdiropargs(&rmda, nm, dvp);
			/*LINTED pointer alignment */
			rlock(vtor(dvp));
			(void)rfscall(mi, RFS_RMDIR, 0, xdr_diropargs, (caddr_t)&rmda,
					xdr_enum, (caddr_t)&rmstatus, cred);
			/*LINTED pointer alignment */
			runlock(vtor(dvp));
			VN_RELE(*vpp);
			*vpp = NULL;
			return (ENOSYS);
		}
		gid = vap->va_gid;
		/* LINTED pointer alignment */
		vtor(*vpp)->r_aclcnt = acl_getmax();
		esvnattr_to_vattr(*vpp, &cdr.dr_attr, vap, &(*vpp)->v_lid,
				  /* LINTED pointer alignment */
				  vtor(*vpp)->r_acl, &vtor(*vpp)->r_aclcnt);
		if (gid != vap->va_gid) {
			vap->va_mask = AT_GID;
			vap->va_gid = gid;
			(void) nfs_setattr(*vpp, vap, 0, cred);
		}
	} else
		*vpp = (struct vnode *)0;
	NFSLOG(4, "nfs_makemld: returning %d\n", error, 0);
	return (error);
}

/*
 * nfs_setlevel: reset the file level (ESV protocol only). First make some
 * sanity checks, then send an NFSPROC_SETATTR request with only the file
 * level set. Set the local level based on the response. We don't do the
 * full permissions checks here - the server must perform them. We also don't
 * check for file tranquility, as that information is not available.
 */
/* ARGSUSED */
STATIC int
nfs_setlevel(vp, level, cred)
	struct vnode *vp;
	lid_t level;
	struct cred *cred;
{
	struct nfsesvsaargs cargs;
	struct nfsesvattrstat cns;
	struct vattr va;
	int error;
	/* LINTED pointer alignment */
	struct mntinfo *mi = vtomi(vp);

	NFSLOG(2, "nfs_setlevel %d %s\n", level, mi->mi_hostname);
	if (mi->mi_protocol != NFS_ESV)
		return (ENOSYS);
	if (vp->v_flag & VROOT)
		return (EBUSY);
	/* LINTED pointer alignment */
	if (vtor(vp)->r_attr.va_uid != cred->cr_uid &&
	    pm_denied(cred, P_OWNER))
		return (EPERM);

	va.va_uid = (uid_t)-1;
	va.va_gid = (gid_t)-1;
	va.va_size = (size_t)-1;
	va.va_mode = (mode_t)-1;
	va.va_atime.tv_sec = (u_long)-1;
	va.va_atime.tv_nsec = (u_long)-1;
	va.va_mtime.tv_sec = (u_long)-1;
	va.va_mtime.tv_nsec = (u_long)-1;
	vattr_to_esvsattr(&va, &cargs.saa_sa, &mi->mi_addr, &level, NULL, 0);
	/* LINTED pointer alignment */
	cargs.saa_fh = *vtofh(vp);
	error = rfscall(mi, RFS_SETATTR, 0, xdr_esvsaargs, (caddr_t)&cargs,
			xdr_esvattrstat, (caddr_t)&cns, cred);
	if (!error)
		error = geterrno(cns.ns_status);
	if (!error) {
		timestruc_t mtime;
		mtime.tv_sec = cns.ns_attr.na_mtime.tv_sec;
		mtime.tv_nsec = cns.ns_attr.na_mtime.tv_usec * 1000;
		nfs_cache_check(vp, mtime);
		nfs_esvattrcache(vp, &cns.ns_attr);
	} else {
		/* LINTED pointer alignment */
		PURGE_ATTRCACHE(vp);
		PURGE_STALE_FH(error, vp);
	}
	NFSLOG(4, "nfs_setlevel: returning %d\n", error, 0);
	return (error);
}

/* search u.u_flist for this vnode, and if it's locked unlock it */
STATIC int
nfs_lockrelease(vp, flag, offset, cred)
	struct vnode *vp;
	int flag;
	off_t offset;
	struct cred *cred;
{
	NFSLOG(2, "nfs_lockrelease: flag %d offset %d\n", flag, offset);
	/*
	 * Only do extra work if the process has done record-locking.
	 */
	if (u.u_procp->p_flag & SLKDONE) {
		struct file *ufppp;
		struct ufchunk *ufpp;
		struct flock ld;
		int i, locked;

		locked = 0;		/* innocent until proven guilty */
		u.u_procp->p_flag &= ~SLKDONE;	/* reset process flag */
		/*
		 * Check all open files to see if there's a lock
		 * possibly held for this vnode.
		 */
		for (ufpp = &u.u_flist; ufpp != NULL; ufpp = ufpp->uf_next)
			for (i=0; i < NFPCHUNK; i++) {
				if (((ufppp = ufpp->uf_ofile[i]) != NULL) &&
				    (ufpp->uf_pofile[i] & UF_FDLOCK)) {

					/* the current file has an active lock */
					if (ufppp->f_vnode == vp) {

						/* release this lock */
						locked = 1;	/* (later) */
						ufpp->uf_pofile[i] &= ~UF_FDLOCK;
					} else {
						/* another file is locked */
						u.u_procp->p_flag |= SLKDONE;
					}
				}
			} /* for all files */

		/*
		 * If 'locked' is set, release any locks that this process
		 * is holding on this file.  If record-locking on any other
		 * files was detected, the process was marked (SLKDONE) to
		 * run thru this loop again at the next file close.
		 */
		if (locked) {
			ld.l_type = F_UNLCK;	/* set to unlock entire file */
			ld.l_whence = 0;	/* unlock from start of file */
			ld.l_start = 0;
			ld.l_len = 0;		/* do entire file */
			return (nfs_frlock(vp, F_SETLK, &ld, flag,
				offset, cred));
		}
	}
	return (0);
}

STATIC int
nfs_allocstore(vp, off, len, cred)
	struct vnode	*vp;
	u_int		off;
	u_int		len;
	struct cred	*cred;
{
 	/* NFS needs no blocks allocated, so succeed silently */
	return 0;
}
