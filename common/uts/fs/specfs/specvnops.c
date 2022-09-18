/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)uts-comm:fs/specfs/specvnops.c	1.31.3.14"
#ident	"$Header: $"

#include <acc/dac/acl.h>
#include <acc/mac/cca.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/fs_subr.h>
#include <fs/specfs/devmac.h>
#include <fs/specfs/snode.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/open.h>
#include <io/poll.h>
#include <io/strsubr.h>
#include <io/termios.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/seg_dev.h>
#include <mem/seg_kmem.h>
#include <mem/seg_map.h>
#include <mem/seg_vn.h>
#include <mem/swap.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/mod/mod_k.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern int nodev();
extern struct vnode *devsec_cloneopen();

extern	struct	modctl	*mod_shadowcsw[];
extern	struct	modctl	*mod_shadowbsw[];
extern proc_t *curproc;

STATIC int device_close();

STATIC int spec_open();
STATIC int spec_close();
STATIC int spec_read();
STATIC int spec_write();
STATIC int spec_ioctl();
STATIC int spec_getattr();
STATIC int spec_setattr();
STATIC int spec_access();
STATIC int spec_fsync();
STATIC void spec_inactive();
STATIC int spec_fid();
STATIC int spec_seek();
STATIC int spec_frlock();
STATIC int spec_realvp();
STATIC int spec_getpage();
extern int spec_putpage();
STATIC int spec_map();
STATIC int spec_addmap();
STATIC int spec_delmap();
STATIC int spec_poll();
STATIC int spec_getacl();
STATIC int spec_getaclcnt();
STATIC int spec_setacl();
STATIC int spec_setlevel();
STATIC int spec_getdvstat();
STATIC int spec_setdvstat();
STATIC int spec_allocstore();

extern int spec_segmap();

struct vnodeops spec_vnodeops = {
	spec_open,
	spec_close,
	spec_read,
	spec_write,
	spec_ioctl,
	fs_setfl,
	spec_getattr,
	spec_setattr,
	spec_access,
	fs_nosys,	/* lookup */
	fs_nosys,	/* create */
	fs_nosys,	/* remove */
	fs_nosys,	/* link */
	fs_nosys,	/* rename */
	fs_nosys,	/* mkdir */
	fs_nosys,	/* rmdir */
	fs_nosys,	/* readdir */
	fs_nosys,	/* symlink */
	fs_nosys,	/* readlink */
	spec_fsync,
	spec_inactive,
	spec_fid,
	fs_rwlock,
	fs_rwunlock,
	spec_seek,
	fs_cmp,
	spec_frlock,
	fs_nosys,	/* space */
	spec_realvp,
	spec_getpage,
	spec_putpage,
	spec_map,
	spec_addmap,
	spec_delmap,
	spec_poll,
	fs_nosys,	/* not used */
	fs_pathconf,
	spec_allocstore,
	spec_getacl,	
	spec_getaclcnt,
	spec_setacl,
	spec_setlevel,
	spec_getdvstat,
	spec_setdvstat,
	fs_nosys,	/* makemld */
	fs_nosys,	/* filler */
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

/*
 * Allow handler of special files to initialize and validate before
 * actual I/O.
 */
STATIC int
spec_open(vpp, flag, cr)
	struct vnode **vpp;
	int flag;
	struct cred *cr;
{
	register unsigned int maj;
	register dev_t dev;
	dev_t newdev;
	o_pid_t *oldttyp;
	struct vnode *nvp;
	struct vnode *vp = *vpp;
	struct vnode *cvp = VTOS(vp)->s_commonvp;
	struct modctl *modctlp = NULL, *nmodctlp = NULL;
	int error = 0;
	label_t	saveq;
	int macmode = 0;

	ASSERT(vp->v_type == VCHR || vp->v_type == VBLK);

	flag &= ~FCREAT;		/* paranoia */

	/*
	 * This flag is specific to the pre-SVR4 RFS.  We turn it off here
	 * to avoid passing it to the innocent.
	 */
	flag &= ~FNMFS;

	newdev = dev = vp->v_rdev;

	/*
	 * Catch half-opens.
	 */
	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {
		(void) spec_close(vp, flag & FMASK, 1, (off_t) 0, cr);
		u.u_qsav = saveq;
		return EINTR;
	}

	VTOS(vp)->s_count++;	/* one more open reference */
	VTOS(cvp)->s_count++;	/* one more open reference */

	if (mac_installed) {
		/* check for mandatory access control for devices */
		if (flag & FREAD)
			macmode |= VREAD;
		if (flag & (FWRITE|FTRUNC))
			macmode |= VWRITE;
		error = spec_access(vp, macmode, MAC_ACC, cr);
		if (error != 0)
			goto done;
	}

	switch (vp->v_type) {

	case VCHR:
		if ((maj = getmajor(dev)) >= cdevcnt) {
			error = ENXIO;
			break;
		}
		oldttyp = u.u_ttyp; 	/* used only by old tty drivers */

		/*
		 * If the driver is loadable, increment its
		 * reference count.
		 */
		if(modctlp = mod_shadowcsw[maj])	{
			MOD_HOLD(modctlp);
		}

		if (cdevsw[maj].d_str) {
			if ((error = stropen(cvp, &newdev, flag, cr)) == 0) {
				struct stdata *stp = cvp->v_stream;
				if (dev != newdev) {
					/*
					 * Clone open.
					 */
					vp->v_stream = NULL;
					cvp->v_stream = NULL;
					if ((nvp = devsec_cloneopen(vp,newdev, VCHR))
					  == NULL) {
						vp->v_stream = NULL;
						cvp->v_stream = NULL;
						strclose(vp, flag, cr);
						nmodctlp = mod_shadowcsw[getmajor(newdev)];
						error = ENOMEM;
						break;
					}
					/*
					 * STREAMS clones inherit fsid and
					 * stream.
					 */
					VTOS(nvp)->s_fsid = VTOS(vp)->s_fsid;
					VTOS(vp)->s_count--;
					VTOS(cvp)->s_count--;
					nvp->v_vfsp = vp->v_vfsp;
					nvp->v_stream = stp;
					cvp = VTOS(nvp)->s_commonvp;
					cvp->v_stream = stp;
					stp->sd_vnode = cvp;
					stp->sd_strtab =
					  cdevsw[getmajor(newdev)].d_str;
					VN_RELE(vp);
					VTOS(nvp)->s_count++;
					VTOS(cvp)->s_count++;
					*vpp = nvp;
				} else {
					/*
					 * Normal open.
					 */
					vp->v_stream = stp;
				}
        			if (oldttyp == NULL && u.u_ttyp != NULL) {
					/* 
					 * Pre SVR4 driver has allocated the
					 * stream as a controlling terminal -
					 * check against SVR4 criteria and
					 * deallocate it if it fails.
					 *
					 * Controlling tty must be open for
					 * write for enhanced security.
					 */
					if ((flag&FNOCTTY) 
					  || !(flag&FWRITE)
					  || !strctty(u.u_procp, stp)) {
						*u.u_ttyp = 0;
						u.u_ttyp = NULL;
					}
				} else if (stp->sd_flag & STRISTTY) {
					/*
					 * This is a post SVR4 tty driver -
					 * try to allocate it as a
					 * controlling terminal.
					 *
					 * Controlling tty must be open for
					 * write for enhanced security.
					 */
					if (!(flag&FNOCTTY) && (flag&FWRITE))
						(void)strctty(u.u_procp, stp);
				}
			}
		} else {
			if ((error = (*cdevsw[maj].d_open)
			  (&newdev, flag, OTYP_CHR, cr)) == 0
			    && dev != newdev) {
				/*
				 * Clone open.
				 *
				 * Allocate new snode for clone and
				 * copy security attributes from
				 * master clone.
				 */
				if ((nvp = devsec_cloneopen(vp,newdev, VCHR)) == NULL) {
					(*cdevsw[getmajor(newdev)].d_close)
					  (newdev, flag, OTYP_CHR, cr);
					nmodctlp = mod_shadowcsw[getmajor(newdev)];
					error = ENOMEM;
					break;
				}

				/*
				 * Character clones inherit fsid.
				 */
				VTOS(nvp)->s_fsid = VTOS(vp)->s_fsid;
				VTOS(vp)->s_count--;
				VTOS(cvp)->s_count--;
				nvp->v_vfsp = vp->v_vfsp;
				cvp = VTOS(nvp)->s_commonvp;
				VN_RELE(vp);
				VTOS(nvp)->s_count++;
				VTOS(cvp)->s_count++;
				*vpp = nvp;
			}

			if (oldttyp == NULL && u.u_ttyp != NULL) {
                                register proc_t *pp = u.u_procp;
                                register sess_t *sp = pp->p_sessp;
				if ((flag&FNOCTTY)
				     || pp->p_pid != sp->s_sid
				     || !(flag&FWRITE)) {
                                        *u.u_ttyp = 0;
                                        u.u_ttyp = NULL;
                                } else {
					extern vnode_t *makectty();
                                        alloctty(pp, 
					  makectty(VTOS(*vpp)->s_commonvp));
                                        u.u_ttyd = (o_dev_t)cmpdev(sp->s_dev);
                                }
                        }
		}
		break;

	case VBLK:
		newdev = dev;
		if ((maj = getmajor(dev)) >= bdevcnt) {
			error = ENXIO;
			break;
		}

		/*
		 * If the driver is loadable, increment its
		 * reference count.
		 */
		if(modctlp = mod_shadowbsw[maj])	{
			MOD_HOLD(modctlp);
		}

		if ((error = (*bdevsw[maj].d_open)(&newdev, flag,
		    OTYP_BLK, cr)) == 0 && dev != newdev) {
			/*
			 * Clone open.
			 *
			 * Allocate new snode for clone and copy
			 * security attributes from master clone.
			 */
			maj = getmajor(newdev);
			if ((nvp = devsec_cloneopen(vp,newdev, VBLK)) == NULL) {
				(*bdevsw[maj].d_close)
				  (newdev, flag, OTYP_BLK, cr);
				nmodctlp = mod_shadowbsw[maj];
				error = ENOMEM;
				break;
			}

			/*
			 * Block clones inherit fsid.
			 */
			VTOS(nvp)->s_fsid = VTOS(vp)->s_fsid;
			VTOS(vp)->s_count--;
			VTOS(cvp)->s_count--;
			nvp->v_vfsp = vp->v_vfsp;
			cvp = VTOS(nvp)->s_commonvp;
			VN_RELE(vp);
			VTOS(nvp)->s_count++;
			VTOS(cvp)->s_count++;
			vp = *vpp = nvp;
		}

		/*
		 * If device size is unknown, call the size routine
		 * to see if size can be determined at this point,
		 * now that the device has been opened.
		 */
		if (VTOS(vp)->s_size == UNKNOWN_SIZE) {
			if (VTOS(cvp)->s_size == UNKNOWN_SIZE) {
				int (*sizef)() = bdevsw[maj].d_size;
				int size;

				if (sizef != nulldev &&
				    (size = (*sizef)(newdev)) != -1)
					VTOS(cvp)->s_size = dtob(size);
			}
			VTOS(vp)->s_size = VTOS(cvp)->s_size;
		}
		break;

	default:
		cmn_err(CE_PANIC, "spec_open: type not VCHR or VBLK\n");
		break;
	}
done:
	if (error != 0) {
		VTOS(*vpp)->s_count--;	/* one less open reference */
		VTOS(cvp)->s_count--;   /* one less open reference */

		/*
		 * If the module is loadable,
		 * decrement its reference count on failure.
		 */
		if(modctlp)	{
			MOD_RELE(modctlp);
		}
		if(nmodctlp && nmodctlp != modctlp)	{
			MOD_RELE(nmodctlp);
		}
	}
	u.u_qsav = saveq;
	return error;
}

/* ARGSUSED */
STATIC int
spec_close(vp, flag, count, offset, cr)
	struct vnode *vp;
	int flag;
	int count;
	off_t offset;
	struct cred *cr;
{
	register struct snode *sp;
	enum vtype type;
	dev_t dev;
	unsigned int	maj;
	struct	modctl	*modctlp = NULL;
	register struct vnode *cvp = VTOS(vp)->s_commonvp;
	int error = 0;
	label_t	saveq;

	cleanlocks(vp, u.u_procp->p_epid, u.u_procp->p_sysid);
	if (vp->v_stream)
		strclean(vp);
	ASSERT(count >= 1);
	if (count > 1)
		return 0;

	saveq = u.u_qsav;
	if (setjmp(&u.u_qsav)) {	/* catch half-closes */
		u.u_qsav = saveq;
		return EINTR;
	}

	sp = VTOS(vp);
	sp->s_count--;		/* one fewer open reference */
	VTOS(cvp)->s_count--;	/* one fewer open reference */

	dev = sp->s_dev;
	type = vp->v_type;
	maj = getmajor(dev);

	ASSERT(type == VCHR || type == VBLK);

	/* added for enhanced security, last close on a snode */

	if ((mac_installed) && (sp->s_count == 0))
		devsec_close(sp);

	if(type == VBLK)	{
		modctlp = mod_shadowbsw[maj];
	}
	else	{
		modctlp = mod_shadowcsw[maj];
	}

	/*
	 * Only call the close routine when the last open reference through
	 * any [s,v]node goes away.
	 */
	if (!stillreferenced(dev, type))
		error = device_close(vp, flag, cr);
	u.u_qsav = saveq;

	/*
	 * If the driver is loadable, decrement its reference count.
	 */
	if(modctlp)	{
		MOD_RELE(modctlp);
	}
	return error;
}

STATIC int
device_close(vp, flag, cr)
	struct vnode *vp;
	int flag;
	struct cred *cr;
{
	struct snode *sp = VTOS(vp);
	dev_t dev = sp->s_dev;
	enum vtype type = vp->v_type;
	register int error;
	int otype;

	switch (type) {

	case VCHR:
		if (cdevsw[getmajor(dev)].d_str) {
			error = strclose(sp->s_commonvp, flag, cr);
			vp->v_stream = NULL;
		} else {
			error = (*cdevsw[getmajor(dev)].d_close)
			  (dev, flag, OTYP_CHR, cr);
		}
		break;

	case VBLK:
		/*
		 * On last close a block device we must
		 * invalidate any in-core blocks so that we
		 * can, for example, change floppy disks.
		 */
		(void) spec_putpage(sp->s_commonvp, 0, 0, B_INVAL,
		  (struct cred *) 0);
		bflush(dev);
		binval(dev);

		error = (*bdevsw[getmajor(dev)].d_close)
		  (dev, flag, OTYP_BLK, cr);
		break;
	}

	return error;
}

/* ARGSUSED */
STATIC int
spec_read(vp, uiop, ioflag, fcr)
	register struct vnode *vp;
	register struct uio *uiop;
	int ioflag;
	struct cred *fcr;
{
	struct cred *cr = VCURRENTCRED(fcr);	/* refer to vnode.h */
	int error;
	struct snode *sp = VTOS(vp);
	register dev_t dev = sp->s_dev;
	register unsigned on, n;
	unsigned long bdevsize;
	off_t off;
	struct vnode *blkvp;
	int oresid;

	ASSERT(fcr != NULL);

	if (uiop->uio_resid == 0)
		return 0;

	ASSERT(vp->v_type == VCHR || vp->v_type == VBLK);

	/* security check if device is in private state or if it is dynamic */

	if ((mac_installed) &&
	    (error = spec_access(vp, VREAD, MAC_ACC | MAC_RW, cr)))
		return error;

	if (!IS_SWAPVP(vp) && WRITEALLOWED(vp, cr)) {
		MAC_ASSERT (sp, MAC_SAME);
		smark(sp, SACC);
	}

	if (vp->v_type == VCHR) {
		if (cdevsw[getmajor(dev)].d_str)
			error = strread(vp, uiop, cr);
		else 
			error = (*cdevsw[getmajor(dev)].d_read)(dev, uiop, cr);
		return error;
	}

	/*
	 * Block device.
	 */
	error = 0;
	blkvp = sp->s_commonvp;
	bdevsize = sp->s_size;
	oresid = uiop->uio_resid;
	do {
		int diff;
		caddr_t base;

		off = uiop->uio_offset & MAXBMASK;
		on = uiop->uio_offset & MAXBOFFSET;
		n = MIN(MAXBSIZE - on, uiop->uio_resid);
		diff = bdevsize - uiop->uio_offset;

		if (diff <= 0) 
			break;
		if (diff < n)
			n = diff;

		base = segmap_getmap(segkmap, blkvp, off);
		error = as_fault(&kas, base + on, n, F_SOFTLOCK, S_WRITE);
		if (error) {
			(void) segmap_release(segkmap, base, 0);
			return EIO;
		}

		if ((error = uiomove(base + on, n, UIO_READ, uiop)) == 0) {
			int flags = 0;
			/*
			 * If we read a whole block, we won't need this
			 * buffer again soon.  Don't mark it with
			 * SM_FREE, as that can lead to a deadlock
			 * if the block corresponds to a u-page.
			 * (The keep count never drops to zero, so
			 * waiting for "i/o to complete" never
			 * terminates; this points out a flaw in
			 * our locking strategy.)
			 */
			if (n + on == MAXBSIZE)
				flags = SM_DONTNEED;
			(void) as_fault(&kas, base + on, n, F_SOFTUNLOCK, S_OTHER);
			error = segmap_release(segkmap, base, flags);
		} else {
			(void) as_fault(&kas, base + on, n, F_SOFTUNLOCK, S_OTHER);
			(void) segmap_release(segkmap, base, 0);
			if (bdevsize == UNKNOWN_SIZE) {
				error = 0;
				break;
			}
		}
	} while (error == 0 && uiop->uio_resid > 0 && n != 0);

	if (oresid != uiop->uio_resid)		/* partial read */
		error = 0;

	return error;
}

STATIC int
spec_write(vp, uiop, ioflag, fcr)
	struct vnode *vp;
	register struct uio *uiop;
	int ioflag;
	struct cred *fcr;
{
	struct cred *cr = VCURRENTCRED(fcr);	/* refer to vnode.h */
	int error;
	struct snode *sp = VTOS(vp);
	register dev_t dev = sp->s_dev;
	register unsigned n, on;
	unsigned long bdevsize;
	off_t off;
	struct vnode *blkvp;
	int oresid;
	page_t *iolpl[MAXBSIZE/PAGESIZE + 2];
	page_t **ppp;

	ASSERT(fcr != NULL);

	/* security checks if device is private or dynamic */

	if ((mac_installed) &&
	    (error= spec_access(vp, VWRITE, MAC_ACC|MAC_RW,cr)))
		return error;
								
	ASSERT(vp->v_type == VCHR || vp->v_type == VBLK);

	if (vp->v_type == VCHR) {
		smark(sp, SUPD|SCHG);
		if (cdevsw[getmajor(dev)].d_str)
			error = strwrite(vp, uiop, cr);
		else
			error = (*cdevsw[getmajor(dev)].d_write)(dev, uiop, cr);
		return error;
	} else { /* 
		  * Make asynch writes synch for block special files.
		  */
		if (!(ioflag & IO_SYNC))
			ioflag |= IO_SYNC;
	}

	/*
	 * Block device.
	 */
	if (uiop->uio_resid == 0)
		return 0;

	error = 0;
	blkvp = sp->s_commonvp;
	bdevsize = sp->s_size;
	oresid = uiop->uio_resid;
	do {
		int diff, pagecreate;
		caddr_t base;
		int resid;
		int wcnt;

		off = uiop->uio_offset & MAXBMASK;
		on = uiop->uio_offset & MAXBOFFSET;
		n = MIN(MAXBSIZE - on, uiop->uio_resid);
		diff = bdevsize - uiop->uio_offset;
		resid = uiop->uio_resid;

		if (diff <= 0) {
			error = ENXIO;
			break;
		}
		if (diff < n)
			n = diff;

		/*
		 * as_iolock will determine if we are properly
		 * page-aligned to do the pagecreate case, and if so,
		 * will hold the "from" pages until after the uiomove
		 * to avoid deadlocking and to catch the case of
		 * writing a file to itself.
		 */
		n = as_iolock(uiop, iolpl, n, blkvp, bdevsize, &pagecreate);
		if (n == 0) {
			error = EFAULT;
			break;
		}


		base = segmap_getmap(segkmap, blkvp, off);

		if (pagecreate) {
			segmap_pagecreate(segkmap, base + on, (u_int)n, 0);
		}

		error = uiomove(base + on, n, UIO_WRITE, uiop);
		wcnt = resid - uiop->uio_resid;

		/* release any pages held by as_iolock */
		for (ppp = iolpl; *ppp; ppp++ )
			PAGE_RELE(*ppp);

		if (pagecreate
		  && uiop->uio_offset < roundup(off + on + n, PAGESIZE)) {
			/*
			 * We created pages w/o initializing them completely,
			 * thus we need to zero the part that wasn't set up.
			 * This can happen if we write to the end of the device
			 * or if we had some sort of error during the uiomove.
			 */
			int nzero, nmoved;

			nmoved = uiop->uio_offset - (off + on);
			ASSERT(nmoved >= 0 && nmoved <= n);
			nzero = roundup(n, PAGESIZE) - nmoved;
			ASSERT(nzero > 0 && on + nmoved + nzero <= MAXBSIZE);
			(void) kzero(base + on + nmoved, (u_int)nzero);
		}

		if (error == 0) {
			int flags = 0;

			/*
			 * Force write back for synchronous write cases.
			 */
			if (ioflag & IO_SYNC)
				flags = SM_WRITE;
			else if (n + on == MAXBSIZE || IS_SWAPVP(vp)) {
				/*
				 * Have written a whole block.
				 * Start an asynchronous write and
				 * mark the buffer to indicate that
				 * it won't be needed again soon.
				 * Push swap files here, since it
				 * won't happen anywhere else.
				 */
				flags = SM_WRITE | SM_ASYNC | SM_DONTNEED;
			}
			smark(sp, SUPD|SCHG);
			error = segmap_release(segkmap, base, flags);
			if (error)
				uioupdate(uiop, -wcnt);
		} else {
			(void) segmap_release(segkmap, base, SM_INVAL);
			uioupdate(uiop, -wcnt);
		}
	} while (error == 0 && uiop->uio_resid > 0 && n != 0);

	if (oresid != uiop->uio_resid)		/* partial write */
		error = 0;

	return error;
}

STATIC int
spec_ioctl(vp, cmd, arg, mode, cr, rvalp)
	register struct vnode *vp;
	int cmd;
	int arg;
	int mode;
	struct cred *cr;
	int *rvalp;
{
	dev_t dev;
	int error;

	if (vp->v_type != VCHR)
		return ENOTTY;

	/*
	 * Security checks if device is private or dynamic.
	 */

	if (mac_installed) {
		switch(cmd) {

		case TCGETS:
		case TCGETA:
			/*
			 * Only check state of device and MAC
			 * acc on dynamic devices.
			 */
			if (error = spec_access(vp, VREAD, MAC_ACC| MAC_IO, cr))
				return error;
			break;
		default:
			if (error = spec_access(vp, VWRITE, MAC_ACC, cr))
				return error;
			break;
		}
	}
	dev = VTOS(vp)->s_dev;
	if (cdevsw[getmajor(dev)].d_str)
		error = strioctl(vp, cmd, arg, mode, U_TO_K, cr, rvalp);
	else
		error = (*cdevsw[getmajor(dev)].d_ioctl)(dev, cmd, arg, 
			mode, cr, rvalp);
	return error;
}

STATIC int
spec_getattr(vp, vap, flags, cr)
	register struct vnode *vp;
	register struct vattr *vap;
	int flags;
	struct cred *cr;
{
	int error;
	register struct snode *sp;
	register struct vnode *realvp;

	if (flags & ATTR_COMM && vp->v_type == VBLK)
		vp = VTOS(vp)->s_commonvp;
	
	sp = VTOS(vp);
	if ((realvp = sp->s_realvp) == NULL) {
		/*
		 * No real vnode behind this one.  Fill in the fields
		 * from the snode.
		 *
		 * This code should be refined to return only the
		 * attributes asked for instead of all of them.
		 */
		vap->va_type = vp->v_type;
		vap->va_mode = 0;
		vap->va_uid = vap->va_gid = 0;
		vap->va_fsid = sp->s_fsid;
		vap->va_nodeid = (long)sp & 0xFFFF;	/* may not be unique */
		vap->va_nlink = 0;
		vap->va_rdev = sp->s_dev;
		vap->va_blksize = MAXBSIZE;
	} else if (error = VOP_GETATTR(realvp, vap, flags, cr))
		return error;

	vap->va_size = (sp->s_size == UNKNOWN_SIZE ? 0 : sp->s_size);
	vap->va_nblocks = btod(vap->va_size);

	vap->va_atime.tv_sec = sp->s_atime;
	vap->va_atime.tv_nsec = 0;
	vap->va_mtime.tv_sec = sp->s_mtime;
	vap->va_mtime.tv_nsec = 0;
	vap->va_ctime.tv_sec = sp->s_ctime;
	vap->va_ctime.tv_nsec = 0;
	vap->va_vcode = 0;

	return 0;
}

STATIC int
spec_setattr(vp, vap, flags, cr)
	struct vnode *vp;
	register struct vattr *vap;
	int flags;
	struct cred *cr;
{
	register struct snode *sp = VTOS(vp);
	register struct vnode *realvp;
	int error;

	if ((realvp = sp->s_realvp) == NULL)
		error = 0;	/* no real vnode to update */
	else
		error = VOP_SETATTR(realvp, vap, flags, cr);
	if (error == 0) {
		/*
		 * If times were changed, update snode.
		 */
		if (vap->va_mask & AT_ATIME)
			sp->s_atime = vap->va_atime.tv_sec;
		if (vap->va_mask & AT_MTIME) {
			sp->s_mtime = vap->va_mtime.tv_sec;
			sp->s_ctime = hrestime.tv_sec;
		}
	}
	return error;
}

/*
 * This is the central routine that will perform mandatory and discretionary
 * access checks on block or character special files.   The additional field
 * has been used to encode what kind of access is to be done:
 *
 * DAC_ACC - require discretionary access checks
 * MAC_ACC - require mandatory access checks
 * MAC_RW | MAC_ACC  - require mandatory access checks for read,write
 *			type operations
 *
 * For read,write type operations, mandatory access checks are
 * only performed for dynamic devices.
 */
STATIC int
spec_access(vp, mode, flags, cr)
	struct vnode *vp;
	int mode;
	int flags;
	struct cred *cr;
{
	register struct vnode *realvp;
	struct snode *sp = VTOS(vp);
	int driver_flag = 0;
	int macmode = mode;

	if (flags == 0)
		flags = DAC_ACC;
	if (flags & MAC_ACC) {
		if (STATE(sp) == DEV_PRIVATE) {
			if (pm_denied(cr, P_DEV)) {
				if (flags & MAC_RW)
					return EIO;
				else	return EPERM;
			}
		}
		if (sp->s_secflag & D_NOSPECMACDATA)
			goto out;
		if (sp->s_secflag & D_RDWEQ)
			macmode = VWRITE;
		if (flags & (MAC_RW | MAC_IO)) {
			if ((MODE(sp) == DEV_DYNAMIC) &&
			    (MAC_VACCESS(vp, macmode, cr))) {
				if (flags & MAC_IO)
					return EACCES;
				else return EBADF;
			}
		}
		else if (MAC_VACCESS(vp, macmode, cr))
			return EACCES;
	}
out:	if (flags & DAC_ACC) {
		if ((realvp = VTOS(vp)->s_realvp) != NULL)
			return VOP_ACCESS(realvp, mode, flags, cr);
	}
	return 0;
}

/*
 * In order to sync out the snode times without multi-client problems,
 * make sure the times written out are never earlier than the times
 * already set in the vnode.
 */
STATIC int
spec_fsync(vp, cr)
	struct vnode *vp;
	struct cred *cr;
{
	register struct snode *sp = VTOS(vp);
	register struct vnode *realvp;
	struct vattr va, vatmp;

	/*
	 * If times didn't change, don't flush anything.
	 */
	if ((sp->s_flag & (SACC|SUPD|SCHG)) == 0 && vp->v_type != VBLK)
		return 0;

	sp->s_flag &= ~(SACC|SUPD|SCHG);

	if (vp->v_type == VBLK && sp->s_commonvp != vp
	  && sp->s_commonvp->v_pages != NULL && !IS_SWAPVP(sp->s_commonvp))
		(void) VOP_PUTPAGE(sp->s_commonvp, 0, 0, 0, sys_cred);

	/*
	 * If no real vnode to update, don't flush anything.
	 */
	if ((realvp = sp->s_realvp) == NULL)
		return 0;

	vatmp.va_mask = AT_ATIME|AT_MTIME;
	if (VOP_GETATTR(realvp, &vatmp, 0, cr) == 0) {
		if (vatmp.va_atime.tv_sec > sp->s_atime)
			va.va_atime = vatmp.va_atime;
		else {
			va.va_atime.tv_sec = sp->s_atime;
			va.va_atime.tv_nsec = 0;
		}
		if (vatmp.va_mtime.tv_sec > sp->s_mtime)
			va.va_mtime = vatmp.va_mtime;
		else {
			va.va_mtime.tv_sec = sp->s_mtime;
			va.va_mtime.tv_nsec = 0;
		}
		va.va_mask = AT_ATIME|AT_MTIME;
		(void) VOP_SETATTR(realvp, &va, 0, cr);
	}
	(void) VOP_FSYNC(realvp, cr);
	return 0;
}

STATIC void
spec_inactive(vp, cr)
	struct vnode *vp;
	struct cred *cr;
{
	register struct snode *sp = VTOS(vp);
	register struct vnode *cvp;

	/*
	 * Must sdelete() first to prevent a race when spec_fsync() sleeps.
	 */
	sdelete(sp);

	if (sp->s_realvp)
		(void) spec_fsync(vp, cr);

	if (vp->v_type == VBLK && vp->v_pages != NULL)
		VOP_PUTPAGE(sp->s_commonvp, 0, 0, B_INVAL, sys_cred);

	if (sp->s_realvp) {
		VN_RELE(sp->s_realvp);
		sp->s_realvp = NULL;
	}

	cvp = sp->s_commonvp;
	if (cvp && VN_CMP(cvp, vp) == 0)
		VN_RELE(cvp);

	kmem_free((caddr_t)sp, sizeof (*sp));
}

STATIC int
spec_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	register struct vnode *realvp;

	if ((realvp = VTOS(vp)->s_realvp) != NULL)
		return VOP_FID(realvp, fidpp);
	else
		return EINVAL;
}

/* ARGSUSED */
STATIC int
spec_seek(vp, ooff, noffp)
	struct vnode *vp;
	off_t ooff;
	off_t *noffp;
{
	return 0;
}

/* ARGSUSED */
STATIC int
spec_frlock(vp, cmd, bfp, flag, offset, cr)
	register struct vnode *vp;
	int cmd;
	struct flock *bfp;
	int flag;
	off_t offset;
	struct cred *cr;
{

	register struct snode *csp = VTOS(VTOS(vp)->s_commonvp);

	/*
	 * Code to fail frlock if file was mapped used to be here
	 * It was removed because the lock attempt should only fail 
	 * for mandatory locks, and specfs does not support mandatory 
	 * locks.
	 */
	/* if (csp->s_mapcnt > 0)
		return EAGAIN; */

	return fs_frlock(vp, cmd, bfp, flag, offset, cr);
}

STATIC int
spec_realvp(vp, vpp)
	register struct vnode *vp;
	register struct vnode **vpp;
{
	register struct snode *sp = VTOS(vp);
	struct vnode *rvp;

	vp = sp->s_realvp;
	if (vp && VOP_REALVP(vp, &rvp) == 0)
		vp = rvp;
	*vpp = vp;
	return 0;
}

/*
 * klustsize should be a multiple of PAGESIZE and <= MAXPHYS.
 *
 * XXX -- until drivers understand pageio it's not safe to kluster
 * more than a page at a time.
 *
 * XXX -- The primary user of spec_getapage is read on the swap vnode.
 * In svr4.0, clustering I/O's on the swap device makes little or no
 * sense since all allocations are done page at a time and since the
 * anon blocks are pushed and popped on and off the front like a stack,
 * random disk location is guaranteed. Since this is the case, we will
 * disable clustering in getapage, leaving source customers the freedom
 * to cluster if it helps on their architecture.
 * KLUSTSIZE is defined in snode.h but we will replace it here with PAGESIZE.
 */

STATIC int klustsize = PAGESIZE;
STATIC int spec_ra = 1;
STATIC int spec_lostpage;	/* number of times we lost original page */

/* ARGSUSED */
STATIC int
spec_getapage(vp, off, protp, pl, plsz, seg, addr, rw, cr)
	register struct vnode *vp;
	u_int off;
	u_int *protp;
	struct page *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct cred *cr;
{
	register struct snode *sp;
	struct buf *bp, *bp2;
	struct page *pp, *pp2, **ppp, *pagefound;
	u_int io_off, io_len;
	u_int blksz, blkoff;
	int dora, err;
	u_int xlen;
	int adj_klustsize;

	sp = VTOS(vp);

reread:
	err = 0;
	bp = NULL;
	bp2 = NULL;
	dora = 0;

/**************************************************************
	if (spec_ra && sp->s_nextr == off)
		dora = 1;
	else
		dora = 0;
	if (sp->s_size == UNKNOWN_SIZE) {
		dora = 0;
		adj_klustsize = PAGESIZE;
	} else
		adj_klustsize = klustsize;
***************************************************************/

	adj_klustsize = klustsize;

again:
	if ((pagefound = page_find(vp, off)) == NULL) {
		/*
		 * Need to really do disk I/O to get the page.
		 */
		blkoff = (off / adj_klustsize) * adj_klustsize;
		if (sp->s_size == UNKNOWN_SIZE) {
			blksz = PAGESIZE;
		} else {
			if (blkoff + adj_klustsize <= sp->s_size)
				blksz = adj_klustsize;
			else
				blksz = sp->s_size - blkoff;
		}

		pp = pvn_kluster(vp, off, seg, addr, &io_off, &io_len,
		    blkoff, blksz, 0);
		/*
		 * If someone else got there first, try again.
		 */
		if (pp == NULL)
			goto again;

		if (pl != NULL) {
			int sz;

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
				  pp2 = pp2->p_next)
					ASSERT(pp2->p_next->p_offset !=
					  pp->p_offset);
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

		bp = pageio_setup(pp, io_len, vp,
		  ppp == NULL ? (B_ASYNC|B_READ) : B_READ);

		bp->b_dev = cmpdev(vp->v_rdev);
		bp->b_edev = vp->v_rdev;
		bp->b_blkno = btodt(io_off);

		/*
		 * Zero part of page which we are not
		 * going to be reading from disk now.
		 */
		xlen = io_len & PAGEOFFSET;
		if (xlen != 0)
			pagezero(pp->p_prev, xlen, PAGESIZE - xlen);

		(*bdevsw[getmajor(vp->v_rdev)].d_strategy)(bp);
		vminfo.v_pgin++;
		vminfo.v_pgpgin += btopr(io_len);
		sp->s_nextr = io_off + io_len;
		/*
		 * Mark the level of the process actually
		 * faulting in the page.  Anonymous pages
		 * are skipped.
		 */
		if (mac_installed && !IS_SWAPVP(vp))
			pp->p_lid = cr->cr_lid;
	}

	if (dora) {
		u_int off2;
		addr_t addr2;

		off2 = ((off / klustsize) + 1) * klustsize;
		addr2 = addr + (off2 - off);

		/*
		 * If addr is now in a different seg or we are past
		 * EOF then don't bother trying with read-ahead.
		 */
		if (addr2 >= seg->s_base + seg->s_size || off2 >= sp->s_size)
			pp2 = NULL;
		else {
			if (off2 + klustsize <= sp->s_size)
				blksz = klustsize;
			else
				blksz = sp->s_size - off2;

			pp2 = pvn_kluster(vp, off2, seg, addr2, &io_off,
			  &io_len, off2, blksz, 1);
		}

		if (pp2 != NULL) {
			bp2 = pageio_setup(pp2, io_len, vp, B_READ|B_ASYNC);

			bp2->b_dev = cmpdev(vp->v_rdev);
			bp2->b_edev = vp->v_rdev;
			bp2->b_blkno = btodt(io_off);
			/*
			 * Zero part of page which we are not
			 * going to be reading from disk now.
			 */
			xlen = io_len & PAGEOFFSET;
			if (xlen != 0)
				pagezero(pp2->p_prev, xlen, PAGESIZE - xlen);

			(*bdevsw[getmajor(vp->v_rdev)].d_strategy)(bp2);
			vminfo.v_pgin++;
			vminfo.v_pgpgin += btopr(io_len);
			/*
			 * Mark the level of the process actually
			 * faulting in the page.  Anonymous pages
			 * are skipped.
			 */
			if (mac_installed && !IS_SWAPVP(vp))
				pp2->p_lid = cr->cr_lid;
		}
	}

	if (bp != NULL && pl != NULL) {
		err = biowait(bp);
		pageio_done(bp);
	} else if (pagefound != NULL) {
		int s;

		/*
		 * We need to be careful here because if the page was
		 * previously on the free list, we might have already
		 * lost it at interrupt level.
		 */
		s = splvm();
		if (pagefound->p_vnode == vp && pagefound->p_offset == off) {
			/*
			 * If the page is still in transit or if
			 * it is on the free list, call page_lookup
			 * to try and wait for / reclaim the page.
			 */
			if (pagefound->p_intrans || pagefound->p_free)
				pagefound = page_lookup(vp, off);
		}
		(void) splx(s);
		if (pagefound == NULL || pagefound->p_offset != off
		  || pagefound->p_vnode != vp || pagefound->p_gone) {
			spec_lostpage++;
			goto reread;
		}
		if (pl != NULL) {
			PAGE_HOLD(pagefound);
			pl[0] = pagefound;
			pl[1] = NULL;
			sp->s_nextr = off + PAGESIZE;
		}
	}

	if (err && pl != NULL) {
		for (ppp = pl; *ppp != NULL; *ppp++ = NULL)
			PAGE_RELE(*ppp);
	}

	return err;
}

/*
 * Return all the pages from [off..off+len) in block device.
 */
STATIC int
spec_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw, cr)
	struct vnode *vp;
	u_int off, len;
	u_int *protp;
	struct page *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct cred *cr;
{
	struct snode *sp = VTOS(vp);
	int err;

	ASSERT(vp->v_type == VBLK && sp->s_commonvp == vp);

	if (vp->v_flag & VNOMAP)
		return ENOSYS;

	if (off + len > sp->s_size + PAGEOFFSET)
		return EFAULT;	/* beyond EOF */

	if (protp != NULL)
		*protp = PROT_ALL;
/*
 * Snode lock - This code will function correctly without the SNODE
 * lock in place. However, the snode lock serves as a barrier to creating
 * new pages which under load keeps the system from bottlenecking at
 * the disk queue with the identity of all the pages destroyed.
 * For large memory configurations where many free pages always exist
 * this lock could be removed. Because of the fundamental changes in
 * system behavior caused by removing this lock it will be left in
 * until we get a chance to look at memory management changes needed
 * to offset or compensate for the performance losses caused by its removal.
 */

	if (curproc != proc_sched)
		SNLOCK(sp);
	if (len <= PAGESIZE)
		err = spec_getapage(vp, off, protp, pl, plsz, seg, addr,
		  rw, cr);
	else
		err = pvn_getpages(spec_getapage, vp, off, len, protp, pl,
		  plsz, seg, addr, rw, cr);
	if (curproc != proc_sched)
		SNUNLOCK(sp);
	return err;
}

/*
 * Flags are composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED}.
 */
STATIC int
spec_wrtblk(vp, pp, off, len, flags)
	register struct vnode *vp;
	struct page *pp;
	uint off;
	uint len;
	int flags;
{
	register struct buf *bp;
	int error;

	if ((bp = pageio_setup(pp, len, vp, B_WRITE | flags)) == NULL) {
		pvn_fail(pp, B_WRITE | flags);
		return ENOMEM;
	}

	bp->b_dev = cmpdev(vp->v_rdev);
	bp->b_edev = vp->v_rdev;
	bp->b_blkno = btodt(off);

	(*bdevsw[getmajor(vp->v_rdev)].d_strategy)(bp);

	/*
	 * If async, assume that pvn_done will handle the pages when
	 * I/O is done.
	 */
	if (flags & B_ASYNC)
		/* strat routine posted an error? */
		return((bp->b_flags & B_ERROR) ? bp->b_error : 0);

	error = biowait(bp);
	pageio_done(bp);

	return error;

}

/*
 * Flags are composed of {B_ASYNC, B_INVAL, B_DIRTY B_FREE, B_DONTNEED}.
 * If len == 0, do from off to EOF.
 *
 * The normal cases should be len == 0 & off == 0 (entire vp list),
 * len == MAXBSIZE (from segmap_release actions), and len == PAGESIZE
 * (from pageout).
 */
int
spec_putpage(vp, off, len, flags, cr)
	register struct vnode *vp;
	u_int off;
	u_int len;
	int flags;
	struct cred *cr;
{
	register struct snode *sp = VTOS(vp);
	register struct page *pp;
	struct page *dirty, *io_list;
	register u_int io_off, io_len;
	int vpcount;
	int err = 0;
	int adj_klustsize;

	if (vp->v_flag & VNOMAP)
		return ENOSYS;

	if (vp->v_pages == NULL || off >= sp->s_size)
		return 0;

	ASSERT(vp->v_type == VBLK && sp->s_commonvp == vp);

	vpcount = vp->v_count;
	VN_HOLD(vp);

	if (sp->s_size == UNKNOWN_SIZE)
		adj_klustsize = PAGESIZE;
	else
		adj_klustsize = klustsize;

	if (len == 0) {

		/*
		 * Search the entire vp list for pages >= off.
		 */
		pvn_vplist_dirty(vp, off, flags);
		goto out;
	} else {
		u_int fsize, eoff, offlo, offhi;

		/*
		 * Do a range from [off...off + len) via page_find.
		 * We set limits so that we kluster to klustsize boundaries.
		 */
		fsize = (sp->s_size + PAGEOFFSET) & PAGEMASK;
		eoff = MIN(off + len, fsize);
		offlo = (off / adj_klustsize) * adj_klustsize;
		offhi = roundup(eoff, adj_klustsize);
		dirty = pvn_range_dirty(vp, off, eoff, offlo, offhi, flags);
	}

	/*
	 * Now pp will have the list of kept dirty pages marked for
	 * write-back.  It will also handle invalidation and freeing
	 * of pages that are not dirty.  All the pages on the list
	 * returned must still be dealt with here.
	 */

	/*
	 * Handle all the dirty pages not yet dealt with.
	 */
	while ((pp = dirty) != NULL) {
		/*
		 * Pull off a contiguous chunk
		 */
		page_sub(&dirty, pp);
		io_list = pp;
		io_off = pp->p_offset;
		io_len = PAGESIZE;

#if 1 /* XXX */
		while (dirty != NULL && dirty->p_offset == io_off + io_len) {
			pp = dirty;
			page_sub(&dirty, pp);
			page_sortadd(&io_list, pp);
			io_len += PAGESIZE;
			if (io_len >= adj_klustsize - PAGEOFFSET)
				break;
		}
#endif

		/*
		 * Check for page length rounding problems
		 */
		if (io_off + io_len > sp->s_size) {
			ASSERT((io_off + io_len) - sp->s_size < PAGESIZE);
			io_len = sp->s_size - io_off;
		}
		if (err = spec_wrtblk(vp, io_list, io_off, io_len, flags))
			break;
	}

	if (err && dirty != NULL)
		pvn_fail(dirty, B_WRITE | flags);

out:
	/*
	 * Instead of using VN_RELE here we are careful to
	 * call the inactive routine only if the vnode
	 * reference count is now zero but wasn't zero
	 * coming into putpage.  This is to prevent calling
	 * the inactive routine on a vnode that is already
	 * considered to be in the "inactive" state.
	 * Inactive is a relative term here.
	 */
	if (--vp->v_count == 0 && vpcount > 0)
		spec_inactive(vp, cr);

	return err;
}

STATIC int
spec_poll(vp, events, anyyet, reventsp, phpp)
	vnode_t *vp;
	short events;
	int anyyet;
	short *reventsp;
	struct pollhead **phpp;
{
	register dev_t dev;
	int error;

	if (vp->v_type == VBLK)
		error = fs_poll(vp, events, anyyet, reventsp, phpp);
	else {
		ASSERT(vp->v_type == VCHR);
		dev = vp->v_rdev;
		if (cdevsw[getmajor(dev)].d_str) {
			ASSERT(vp->v_stream != NULL);
			error = strpoll(vp->v_stream, events, anyyet,
			  reventsp, phpp);
		} else if (cdevsw[getmajor(dev)].d_poll)
			error = (*cdevsw[getmajor(dev)].d_poll)
			  (dev, events, anyyet, reventsp, phpp);
		else
			error = fs_poll(vp, events, anyyet, reventsp, phpp);
	}
	return error;
}

/*
 * This routine is called through the cdevsw[] table to handle
 * traditional mmap'able devices that support a d_mmap function.
 */
/* ARGSUSED */
int
spec_segmap(dev, off, as, addrp, len, prot, maxprot, flags, cred)
	dev_t dev;
	u_int off;
	struct as *as;
	addr_t *addrp;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct cred *cred;
{
	struct segdev_crargs dev_a;
	int (*mapfunc)();
	register int i;

	if ((mapfunc = cdevsw[getmajor(dev)].d_mmap) == nodev)
		return ENODEV;

	/*
	 * Character devices that support the d_mmap
	 * interface can only be mmap'ed shared.
	 */
	if ((flags & MAP_TYPE) != MAP_SHARED)
		return EINVAL;

	/*
	 * Check to ensure that the entire range is
	 * legal and we are not trying to map in
	 * more than the device will let us.
	 */
	for (i = 0; i < len; i += PAGESIZE) {
		if ((*mapfunc)(dev, off + i, maxprot) == (int)NOPAGE)
			return ENXIO;
	}

	if ((flags & MAP_FIXED) == 0) {
		/*
		 * Pick an address w/o worrying about
		 * any vac alignment contraints.
		 */
		map_addr(addrp, len, (off_t)off, 0);
		if (*addrp == NULL)
			return ENOMEM;
	} else {
		/*
		 * User-specified address; blow away any previous mappings.
		 */
		(void) as_unmap(as, *addrp, len);
	}

	dev_a.mapfunc = mapfunc;
	dev_a.dev = dev;
	dev_a.offset = off;
	dev_a.prot = (u_char)prot;
	dev_a.maxprot = (u_char)maxprot;

	return as_map(as, *addrp, len, segdev_create, (caddr_t)&dev_a);
}

STATIC int
spec_map(vp, off, as, addrp, len, prot, maxprot, flags, fcred)
	struct vnode *vp;
	u_int off;
	struct as *as;
	addr_t *addrp;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct cred *fcred;
{
	struct cred *cred = VCURRENTCRED(fcred);	/* refer to vnode.h */
	register int error = 0;
	struct vnode *cvp = VTOS(vp)->s_commonvp;
	int macmode = 0;

	ASSERT(cvp != NULL);
	if (vp->v_flag & VNOMAP)
		return ENOSYS;

	/* checks for enhanced security */
	if (mac_installed) {
		if ((maxprot & PROT_READ) || (maxprot & PROT_EXEC))                                  		macmode |= VREAD;
		if (maxprot & PROT_WRITE)
			macmode |=VWRITE;
		if (error =spec_access(vp, macmode, MAC_ACC|MAC_RW, cred))
			return error;
	}

	/*
	 * Code to fail mapping attempt if file was locked used to be here
	 * It was removed because the mapping attempt should only fail 
	 * for mandatory locks, and specfs does not support mandatory 
	 * locks.
	 */
	
	
	if (vp->v_type == VCHR) {
		int (*segmap)();
		dev_t dev = vp->v_rdev;

		/*
		 * Character device: let the device driver
		 * pick the appropriate segment driver.
		 */
		segmap = cdevsw[getmajor(dev)].d_segmap;
		if (segmap == nodev) {
			if (cdevsw[getmajor(dev)].d_mmap == nodev)
				return ENODEV;

			/*
			 * For cdevsw[] entries that specify a d_mmap
			 * function but don't have a d_segmap function,
			 * we default to spec_segmap for compatibility.
			 */
			segmap = spec_segmap;
		}

		return (*segmap)(dev, off, as, addrp, len, prot, maxprot,
		    flags, cred);
	} else if (vp->v_type == VBLK) {
		struct segvn_crargs vn_a;
		struct vnode *cvp;

		/*
		 * Block device, use the underlying commonvp name for pages.
		 */
		cvp = VTOS(vp)->s_commonvp;
		ASSERT(cvp != NULL);

		if ((int)off < 0 || (int)(off + len) < 0)
			return EINVAL;

		/*
		 * Don't allow a mapping beyond the last page of the device.
		 */
		if (off + len > ((VTOS(cvp)->s_size + PAGEOFFSET) & PAGEMASK))
			return ENXIO;
	
		if ((flags & MAP_FIXED) == 0) {
			map_addr(addrp, len, (off_t)off, 1);
			if (*addrp == NULL)
				return ENOMEM;
		} else {
			/*
			 * User-specified address; blow away any
			 * previous mappings.
			 */
			(void) as_unmap(as, *addrp, len);
		}

		vn_a.vp = cvp;
		vn_a.offset = off;
		vn_a.type = flags & MAP_TYPE;
		vn_a.prot = (u_char)prot;
		vn_a.maxprot = (u_char)maxprot;
		vn_a.cred = cred;
		vn_a.amp = NULL;

		error = as_map(as, *addrp, len, segvn_create, (caddr_t)&vn_a);
	} else
		return ENODEV;

	return error;
}

STATIC int
spec_addmap(vp, off, as, addr, len, prot, maxprot, flags, cred)
	struct vnode *vp;
	u_int off;
	struct as *as;
	addr_t addr;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct cred *cred;
{
	register int error = 0;
	struct vnode *cvp = VTOS(vp)->s_commonvp;

	ASSERT(cvp != NULL);
	if (vp->v_flag & VNOMAP)
		return ENOSYS;
	VTOS(cvp)->s_mapcnt += btopr(len);
	return 0;
}

STATIC int
spec_delmap(vp, off, as, addr, len, prot, maxprot, flags, cred)
	struct vnode *vp;
	u_int off;
	struct as *as;
	addr_t addr;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct cred *cred;
{
	struct snode *csp;

	if (vp->v_flag & VNOMAP)
		return ENOSYS;

	csp = VTOS(VTOS(vp)->s_commonvp);

	csp->s_mapcnt -= btopr(len);

	if (csp->s_mapcnt < 0)
		cmn_err(CE_PANIC, "spec_unmap: fewer than 0 mappings");

	/*
	 * Call the close routine when the last reference of any
	 * kind through any [s,v]node goes away.
	 */
	if (!stillreferenced((dev_t)csp->s_dev, vp->v_type))
		/* want real file flags here. */
		(void) device_close(vp, 0, cred);
	return 0;
}


/*
 * Here are all the vnode  operations added for security 
 *
 */

/* 
 * This operation returns the acls for a block or character special file
 */
STATIC int
spec_getacl(vp, nentries, dentriesp, aclbufp, cr, rvalp)
	register struct vnode	*vp;
	register long  		nentries;
	register long		*dentriesp;
	register struct acl 	*aclbufp;
	struct	cred		*cr;
	int			*rvalp;
{
	struct vnode *realvp;

	if ((realvp = VTOS(vp)->s_realvp) != NULL)
		return VOP_GETACL(realvp, nentries, dentriesp, aclbufp, cr, rvalp);
	else
		return ENOSYS;		
}

/* 
 * This operation returns the number of acls for a block or character special file
 */

STATIC int
spec_getaclcnt(vp, cr, rvalp)
	register struct vnode	*vp;
	struct	cred		*cr;
	int			*rvalp;
{
	struct vnode *realvp;

	if ((realvp = VTOS(vp)->s_realvp) != NULL)
		return VOP_GETACLCNT(realvp, cr, rvalp);
	else
		return ENOSYS;		
}


/*
 * This function sets acls on a block or character special file
 */

STATIC int
spec_setacl(vp, nentries, dentries, aclbufp, cr)
	register struct vnode	*vp;
	register long  		nentries;
	register long		dentries;
	register struct acl 	*aclbufp;
	struct	cred		*cr;
{

	struct vnode *realvp;

	if ((realvp = VTOS(vp)->s_realvp) != NULL)
		return VOP_SETACL(realvp, nentries, dentries, aclbufp, cr);
	else
		return ENOSYS;		
}

/*
 * This routine will set the level on a block or character special file.
 * After passing privilege and mac access checks based on the device
 * state, public or private, the level of the vnode is changed before
 * calling the file system dependent level.  If the file system does not support
 * labelling, then the  operations will fail.
 * Note that validitiy of the new level is performed in fs independent level.
 * Ownership check is done in fs dependent level where the disk inode resides.
 */

STATIC int
spec_setlevel(vp, level, cr)
	struct vnode *vp;
	lid_t level;
	struct cred *cr;
{
	
	register struct snode *sp = VTOS(vp);
	register int error=0;
	lid_t savelid;
	struct vnode *realvp;
	struct vnode *cvp= VTOS(vp)->s_commonvp;
	struct snode *csp = VTOS(cvp);
	

	/*
	 * If MAC is not installed, set the level on the real
	 * (SFS) file.
	 */
	if (mac_installed == 0) {
		if (realvp = (sp)->s_realvp)
			error = VOP_SETLEVEL(realvp, level, cr);
		goto out;
	}


	ASSERT((STATE(sp)== DEV_PUBLIC) || (STATE(sp) == DEV_PRIVATE));
	switch (STATE(sp)) {

	case DEV_PUBLIC: 
		if (error = MAC_VACCESS(vp,VWRITE,cr))
			goto out;
                if (pm_denied(cr,P_SETFLEVEL)) {
                    if ((MAC_ACCESS(MACDOM, level, vp->v_lid) == 0) &&
		        (pm_denied(cr, P_MACUPGRADE))){
				error = EPERM;
				goto out;
			}
		}
		if ((MODE(sp) == DEV_STATIC) && ((csp)->s_count || (csp)->s_mapcnt)){
			error = EBUSY;
			goto out;
		}
		break;
	case DEV_PRIVATE:
		if (error = pm_denied(cr, P_DEV)){
			error = EPERM;
			goto out;
		 }
		 break;
	}
	/* device has range, new level must be enclosed by device range */
	if ((REL_FLAG(sp) != DEV_SYSTEM) &&
            ((error = MAC_ACCESS(MACDOM, HI_LEVEL(sp), level)) ||
             (error = MAC_ACCESS(MACDOM, level, LO_LEVEL(sp))))) {
		error = ERANGE;
		goto out;
	}
	/* clone device, no disk vnode */
	if ((realvp = (sp)->s_realvp) == NULL) {
		vp->v_lid = level;
		cvp->v_lid = level;
	}
	else {
		/* there is real disk vnode */
		savelid = vp->v_lid;
		vp->v_lid = level;
		cvp->v_lid = level;
		if (error = VOP_SETLEVEL(realvp, level, cr)) {
			vp->v_lid = savelid;
			cvp->v_lid = savelid;
			goto out;
		}
	}
out:
	return error;
}

/* 
 * This routine will return the security attributes of a device.
 * The snode is locked to return consistent info and prevent a
 * race condition between an ongoing devstat set and level change.
 */

STATIC  int
spec_getdvstat(vp, bufp, cr)
	register struct vnode *vp;
	struct devstat *bufp;
	struct cred *cr;
{
	struct snode *sp= VTOS(vp);
	struct snode *csp = VTOS(VTOS(vp)->s_commonvp);

	bufp->dev_relflag = REL_FLAG(sp);
	bufp->dev_state = STATE(sp);
	bufp->dev_mode = MODE(sp);
	bufp->dev_hilevel = HI_LEVEL(sp);
	bufp->dev_lolevel = LO_LEVEL(sp);
	bufp->dev_usecount = (((csp)->s_count || (csp)->s_mapcnt)? 1:0);
	return 0;
}

/*
 * This routine will set the security attributes of a device.
 * There are three options for setting these attributes:
 * DEV_SYSTEM: releases the current attributes and resets them to system defaults
 * DEV_LASTCLOSE and DEV_PERSISTENT: sets the security attributes on a snode,
 * since these attributes must survive across system calls, the snode and
 * its common are kept incore by increasing reference counts on the vnode inside
 * the snode and the vnode inside the common snode. When invoking this operation
 * with dev_system flag, if a security attribute structure was allocated, it is released
 * and the reference count on the snode and its common is decremented.
 */

STATIC int
spec_setdvstat(vp, bufp, cr)
	struct vnode *vp;
	struct devstat *bufp;
	struct cred *cr;

{
	register int error=0;
	struct snode *sp = VTOS(vp);
	struct snode *csp = VTOS(sp->s_commonvp);


	switch(bufp->dev_relflag) {
	
	case DEV_LASTCLOSE:
	case DEV_PERSISTENT:
		{
		/* 
		 * Need to check for valid lids being passed.
		 * Hi level must dominate lo level.
		 * Mode must be valid : static or dynamic.
		 * State must be valid: static or dynamic.
		 * Current device level must be within new hi and new lo.
		 * If all checks pass and data structure exists,
		 * straight copy; otherwise new structure is allocated
		 * before copying info in kernel.
		 * Privilege check done at fs independent level.
		 */
		if ((bufp->dev_mode != DEV_STATIC ) && 
	         	(bufp->dev_mode != DEV_DYNAMIC))
			return EINVAL;
		if ((bufp->dev_state != DEV_PUBLIC) && 
			(bufp->dev_state != DEV_PRIVATE))
			return EINVAL;
			
                /*
                 * The level range must be validated as well.
                 * Validating one level is sufficient.
                 */
		if (mac_valid(bufp->dev_hilevel)
                ||  MAC_ACCESS(MACDOM, bufp->dev_hilevel, bufp->dev_lolevel))
                        return EINVAL;

                if (MAC_ACCESS(MACDOM, bufp->dev_hilevel, vp->v_lid) ||
                    MAC_ACCESS(MACDOM, vp->v_lid, bufp->dev_lolevel)){
			return EINVAL;
		}
		if (sp->s_dsecp == NULL)
			devsec_dcicreat(sp);
		sp->s_dstate = bufp->dev_state;
		sp->s_dmode = bufp->dev_mode;
		sp->s_dsecp->d_hilid = bufp->dev_hilevel;
		sp->s_dsecp->d_lolid = bufp->dev_lolevel;
		sp->s_dsecp->d_relflag = bufp->dev_relflag;
		csp->s_dstate = bufp->dev_state;
		csp->s_dmode = bufp->dev_mode;
		break;
		}
	case DEV_SYSTEM: {
		/* release security attributes to system setting */
		if ((sp)->s_dsecp != NULL)
			devsec_dcifree(sp);
		break;
		}
	default:
		error = EINVAL;
		break;
	}
	return error;
}

STATIC int
spec_allocstore(vp, off, len, cred)
	struct vnode	*vp;
	u_int		off;
	u_int		len;
	struct cred	*cred;
{
 	/* Specfs needs no blocks allocated, so succeed silently */
	return 0;
}
