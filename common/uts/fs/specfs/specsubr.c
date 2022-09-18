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

#ident	"@(#)uts-comm:fs/specfs/specsubr.c	1.21.3.10"
#ident	"$Header: $"

#include <acc/mac/cca.h>
#include <acc/mac/mac.h>
#include <fs/buf.h>
#include <fs/file.h>
#include <fs/specfs/devmac.h>
#include <fs/specfs/snode.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/open.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <io/termios.h>
#include <mem/kmem.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/mod/mod_k.h>

STATIC dev_t specdev;

STATIC struct snode *sfind();
STATIC struct vnode *commonvp();
STATIC void sinsert();

extern struct vfsops spec_vfsops;

extern struct	modctl	*mod_shadowcsw[];

KSTATIC struct snode *spectable[SPECTBSIZE];	/* snode hash table */
STATIC int spechashlk;				/* a lock for spectable */

/* 
 * Macros to lock and to unlock snode hash table, used during modifications
 * and during traversal of the hash list with possibility of sleep
 */

#define SPECTBLOCK() { \
		while (spechashlk > 0)  \
			(void) sleep((caddr_t)(spectable), PINOD); \
		spechashlk++; \
}

#define SPECTBUNLOCK() { \
		spechashlk=0; \
		wakeup((caddr_t)(spectable)); \
}



/*
 * Return a shadow special vnode for the given dev.
 * If no snode exists for this dev create one and put it
 * in a table hashed by <dev,realvp>.  If the snode for
 * this dev is already in the table return it (ref count is
 * incremented by sfind).  The snode will be flushed from the
 * table when spec_inactive calls sdelete.
 *
 * The fsid is inherited from the real vnode so that clones
 * can be found.
 *
 */
struct vnode *
specvp(vp, dev, type, cr)
	struct vnode *vp;
	dev_t dev;
	vtype_t type;
	struct cred *cr;
{
	register struct snode *sp, *nsp;
	register struct vnode *svp;
	extern struct vnode *fifovp();
	extern struct vnode *xnamvp();
	struct vattr va;


	if (vp == NULL)
		return NULL;
	if (vp->v_type == VFIFO)
		return fifovp(vp, cr);

	if (vp->v_type == VXNAM)
		return xnamvp(vp, cr);

	ASSERT(vp->v_type == type);
	ASSERT(vp->v_rdev == dev);

	if ((sp = sfind(dev, type, vp)) == NULL) {
                /*
                 * Strictly speaking, this MAC_ASSUME is incorrect.
                 * No MAC check has been done on this vnode.  However,
                 * this ensures that the CCA tool understands that
                 * *vp and *sp are at the same level, and the later
                 * MAC_UNKLEV(sp) ensures that the level relationship
                 * to *sp is correct.
                 */
                MAC_ASSUME (vp, MAC_SAME);

		sp = (struct snode *)kmem_zalloc(sizeof(*sp), KM_SLEEP);
		STOV(sp)->v_op = &spec_vnodeops;

		/*
		 * Init the times in the snode to those in the vnode.
		 */
		va.va_mask = AT_TIMES;
		if (VOP_GETATTR(vp, &va, 0, cr) == 0) {
			sp->s_atime = va.va_atime.tv_sec;
			sp->s_mtime = va.va_mtime.tv_sec;
			sp->s_ctime = va.va_ctime.tv_sec;
			sp->s_fsid = va.va_fsid;
		} else
			sp->s_fsid = specdev;
		sp->s_realvp = vp;
		VN_HOLD(vp);
		sp->s_dev = dev;
		svp = STOV(sp);
		svp->v_rdev = dev;
		svp->v_count = 1;
		svp->v_data = (caddr_t)sp;
		svp->v_type = type;
		svp->v_vfsp = vp->v_vfsp;
		/* 
		 * if the file system does not support labeling
		 * vnode will be labelled with the level floor of
		 * of the mounted file system if vfsp is NON_NULL 
		 */
		if (sp->s_realvp->v_macflag & VMAC_SUPPORT)
			svp->v_lid = sp->s_realvp->v_lid;
		else {
			if  (sp->s_realvp->v_vfsp)
				svp->v_lid = sp->s_realvp->v_vfsp->vfs_macfloor;
		}
		/* initialization for security */
		if (mac_installed) {
				
			switch (svp->v_type) {

			case VCHR:
				if ((cdevcnt > getmajor(dev)) &&
					(cdevsw[getmajor(dev)].d_flag))
						sp->s_secflag = 
						(*cdevsw[getmajor(dev)].d_flag) 
						& SECMASK;			
				else sp->s_secflag = 0;
				break;	
			case VBLK:
				if ((bdevcnt > getmajor(dev)) &&
					(bdevsw[getmajor(dev)].d_flag))
						sp->s_secflag = 
						(*bdevsw[getmajor(dev)].d_flag)
						& SECMASK;			
				else sp->s_secflag = 0;
				break;
			}
			sp->s_dstate = ((sp->s_secflag & D_INITPUB) ? DEV_PUBLIC :DEV_PRIVATE);
			sp->s_dmode = DEV_STATIC;
			sp->s_dsecp = NULL;
		}
		if (type == VBLK || type == VCHR) {
			sp->s_commonvp = commonvp(dev, type);
			if (sp->s_commonvp->v_lid == 0)
			/* it means that the common vnode was not initialized */
				sp->s_commonvp->v_lid = svp->v_lid;
			svp->v_stream = sp->s_commonvp->v_stream;
			if (vp->v_type == VBLK)
				sp->s_size = VTOS(sp->s_commonvp)->s_size;
		}
		svp->v_macflag |= VMAC_DOPEN;
		svp->v_macflag |= VMAC_SUPPORT;

                MAC_UNKLEV (sp);        /* see comment above */
		if ((nsp = sfind(dev, type, vp)) == NULL)
                        sinsert(sp);
		else {		/* Lost the race */
                        if (sp->s_commonvp)
                                VN_RELE(sp->s_commonvp);
                        kmem_free((caddr_t)sp, sizeof (*sp));
                        sp = nsp;
		}
	}
	return STOV(sp);
}

/*
 * Try to pre-validate attributes that will be passed to a subsequent
 * specvp() call.  This function is not required to catch all possible
 * error cases for specvp(), but we catch as many as we can.
 */
int
specpreval(type, dev, cr)
	vtype_t type;
	dev_t dev;
	struct cred *cr;
{
	if (type == VFIFO)
		return fifopreval(type, dev, cr);

	if (type == VXNAM)
		return xnampreval(type, dev, cr);

	return 0;
}

/*
 * Return a special vnode for the given dev; no vnode is supplied
 * for it to shadow.  Always create a new snode and put it in the
 * table hashed by <dev,NULL>.  The snode will be flushed from the
 * table when spec_inactive() calls sdelete().
 */
struct vnode *
makespecvp(dev, type)
	register dev_t dev;
	register vtype_t type;
{
	register struct snode *sp;
	register struct vnode *svp;

	sp = (struct snode *)kmem_zalloc(sizeof(*sp), KM_SLEEP);
	svp = STOV(sp);
	svp->v_op = &spec_vnodeops;
	svp->v_type = type;
	svp->v_rdev = dev;
	svp->v_count = 1;
	svp->v_data = (caddr_t)sp;
	svp->v_vfsp = NULL;
	sp->s_atime = hrestime.tv_sec;
	sp->s_mtime = hrestime.tv_sec;
	sp->s_ctime = hrestime.tv_sec;
	sp->s_fsid = specdev;
	sp->s_commonvp = commonvp(dev, type);
	svp->v_stream = sp->s_commonvp->v_stream;
	sp->s_size = VTOS(sp->s_commonvp)->s_size;
	sp->s_realvp = NULL;
	sp->s_dev = dev;
	/* initialization for security */
	if (mac_installed) {
		switch (svp->v_type) {

		case VCHR:
			if ((cdevcnt > getmajor(dev)) &&
				(cdevsw[getmajor(dev)].d_flag))
					sp->s_secflag = 
					(*cdevsw[getmajor(dev)].d_flag) 
					& SECMASK;			
			else sp->s_secflag = 0;
			break;	
		case VBLK:
			if ((bdevcnt > getmajor(dev)) &&
				(bdevsw[getmajor(dev)].d_flag))
					sp->s_secflag = 
					(*bdevsw[getmajor(dev)].d_flag) 
					& SECMASK;			
			else sp->s_secflag = 0;
			break;
		}
		/*
		 * state is initialized to DEV_PUBLIC 
		 * needed for bfs support for unprivileged I/O
		 */
		sp->s_dstate = DEV_PUBLIC; 
		sp->s_dmode = DEV_STATIC;
		sp->s_dsecp = NULL;
	}
	svp->v_lid = u.u_cred->cr_lid;
	svp->v_macflag |= VMAC_DOPEN;
	svp->v_macflag |= VMAC_SUPPORT;
	if (sp->s_commonvp->v_lid == 0)
		sp->s_commonvp->v_lid = u.u_cred->cr_lid;  
	sinsert(sp);
	return svp;
}

/*
 * Find a special vnode that refers to the given device
 * of the given type.  Never return a "common" vnode.
 * Return NULL if a special vnode does not exist.
 * HOLD the vnode before returning it.
 */
struct vnode *
specfind(dev, type)
	dev_t dev;
	vtype_t type;
{
	register struct snode *st;
	register struct vnode *nvp;

	st = spectable[SPECTBHASH(dev)];
	while (st != NULL) {
		if (st->s_dev == dev) {
			nvp = STOV(st);
			if (nvp->v_type == type && st->s_commonvp != nvp) {
				VN_HOLD(nvp);
				return nvp;
			}
		}
		st = st->s_next;
	}
	return NULL;
}

/*
 * Check to see if a device is still in use, given the snode.  The
 * device may still be in use if someone has it open either through
 * the same snode or a different snode, or if someone has it mapped.
 * If it is still open, return 1, otherwise return 0.
 */
int
stillreferenced(dev, type)
	dev_t dev;
	vtype_t type;
{
	register struct snode *sp, *csp;
	register struct vnode *vp;

	if ((vp = specfind(dev, type)) == NULL)
		return 0;
	VN_RELE(vp);
	sp = VTOS(vp);
	csp = VTOS(sp->s_commonvp);
	if (csp->s_count > 0)		/* another snode exists */
		return 1;
	if (csp->s_mapcnt > 0)		/* mappings to device exist */
		return 1;
	return 0;
}

/*
 * Given a device vnode, return the common
 * vnode associated with it.
 */
struct vnode *
common_specvp(vp)
	register struct vnode *vp;
{
	register struct snode *sp;

	if ((vp->v_type != VBLK) && (vp->v_type != VCHR) || 
	  vp->v_op != &spec_vnodeops)
		return vp;
	sp = VTOS(vp);
	return sp->s_commonvp;
}

/*
 * Returns a special vnode for the given dev.  The vnode is the
 * one which is "common" to all the snodes which represent the
 * same device.  For use ONLY by SPECFS.
 */

STATIC
struct vnode *
commonvp(dev, type)
	dev_t dev;
	vtype_t type;
{
	register struct snode *sp,*nsp;
	register struct vnode *svp;

	if ((sp = sfind(dev, type, NULL)) == NULL) {
		sp = (struct snode *)kmem_zalloc(sizeof(*sp), KM_SLEEP);
		STOV(sp)->v_op = &spec_vnodeops;
		sp->s_realvp = NULL;
		sp->s_dev = dev;
		sp->s_fsid = specdev;
		svp = STOV(sp);
		svp->v_rdev = dev;
		svp->v_count = 1;
		svp->v_data = (caddr_t)sp;
		svp->v_type = type;
		svp->v_vfsp = NULL;
		sp->s_commonvp = STOV(sp); /* points to itself */
		if (type == VBLK && getmajor(dev) < bdevcnt) {
			int (*sizef)() = bdevsw[getmajor(dev)].d_size;
			int size;

			if (sizef != nulldev && (size = (*sizef)(dev)) != -1)
				sp->s_size = dtob(size);
			else 
				sp->s_size = UNKNOWN_SIZE;
		}
		/* initialization for security */
		if (mac_installed) {
			switch (svp->v_type) {

			case VCHR:
				if ((cdevcnt > getmajor(dev)) &&
					(cdevsw[getmajor(dev)].d_flag))
						sp->s_secflag = 
						(*cdevsw[getmajor(dev)].d_flag) 
						& SECMASK;			
				else sp->s_secflag = 0;
				break;	
			case VBLK:
				if ((bdevcnt > getmajor(dev)) &&
					(bdevsw[getmajor(dev)].d_flag))
						sp->s_secflag = 
						(*bdevsw[getmajor(dev)].d_flag) 
						& SECMASK;			
				else sp->s_secflag = 0;
				break;
			}
			sp->s_dstate = ((sp->s_secflag & D_INITPUB) ? DEV_PUBLIC :DEV_PRIVATE);
			sp->s_dmode = DEV_STATIC;
			sp->s_dsecp = NULL;
		}
		svp->v_macflag |= VMAC_DOPEN;
		svp->v_macflag |= VMAC_SUPPORT;
		if ((nsp = sfind(dev, type, NULL)) == NULL)
			sinsert(sp);
		else {
			kmem_free((caddr_t)sp, sizeof (*sp));
			sp = nsp;
		}
	}
	return STOV(sp);
}

/*
 * Snode lookup stuff.
 * These routines maintain a table of snodes hashed by dev so
 * that the snode for an dev can be found if it already exists.
 */

/*
 * Put a snode in the table.
 */
STATIC void
sinsert(sp)
	struct snode *sp;
{
	/* lock hash table during modification */
	SPECTBLOCK();
	sp->s_next = spectable[SPECTBHASH(sp->s_dev)];
	spectable[SPECTBHASH(sp->s_dev)] = sp;
	SPECTBUNLOCK();
	return;
}

/*
 * Remove an snode from the hash table.
 * The realvp is not released here because spec_inactive() still
 * needs it to do a spec_fsync().
 */
void
sdelete(sp)
	struct snode *sp;
{
	struct snode *st;
	struct snode *stprev = NULL;

	/* lock hash table during modification */
	SPECTBLOCK();
	st = spectable[SPECTBHASH(sp->s_dev)];
	while (st != NULL) {
		if (st == sp) {
			if (stprev == NULL)
				spectable[SPECTBHASH(sp->s_dev)] = st->s_next;
			else
				stprev->s_next = st->s_next;
			break;
		}
		stprev = st;
		st = st->s_next;
	}
	SPECTBUNLOCK();
	return;
}

/*
 * Lookup an snode by <dev, type, vp>.
 * ONLY looks for snodes with non-NULL s_realvp members and
 * common snodes (with s_commonvp poining to its vnode).
 */
STATIC struct snode *
sfind(dev, type, vp)
	dev_t dev;
	vtype_t type;
	struct vnode *vp;
{
	register struct snode *st;
	register struct vnode *svp;

	st = spectable[SPECTBHASH(dev)];
	while (st != NULL) {
		svp = STOV(st);
		if (st->s_dev == dev && svp->v_type == type
		  && VN_CMP(st->s_realvp, vp)
		  && (vp != NULL || st->s_commonvp == svp)) {
			VN_HOLD(svp);
			return st;
		}
		st = st->s_next;
	}
	return NULL;
}

/*
 * Mark the accessed, updated, or changed times in an snode
 * with the current time.
 */
void
smark(sp, flag)
	register struct snode *sp;
	register int flag;
{
	time_t t = hrestime.tv_sec;

	/* added for enchanced security */
	if ((mac_installed) && (sp->s_secflag & D_NOSPECMACDATA))
		return;
	sp->s_flag |= flag;
	if (flag & SACC)
		sp->s_atime = t;
	if (flag & SUPD)
		sp->s_mtime = t;
	if (flag & SCHG)
		sp->s_ctime = t;
	return;
}

/*
 * SPECFS file system one-time initialization.  Called at system startup time.
 */
int
specinit(vswp, fstype)
	struct vfssw *vswp;
	int fstype;
{
	dev_t dev;

	/*
	 * Associate vfs and vnode operations.
	 */
	vswp->vsw_vfsops = &spec_vfsops;
	if ((dev = getudev()) == -1)
		dev = 0;
	specdev = makedevice(dev, 0);
	return 0;
}

/*
 * Create an snode for a character special device.
 */
struct vnode *
makectty(ovp)
	vnode_t *ovp;
{
	struct	modctl	*modctlp;
	dev_t	dev;
	register vnode_t *vp;

	if (vp = makespecvp((dev = ovp->v_rdev), VCHR)) {
		VTOS(vp)->s_count++;
		VTOS(VTOS(vp)->s_commonvp)->s_count++;

		/*
		 * If the device driver associated with this vnode is loadable,
		 * increment its reference count.
		 * We need to do this here because this extra reference will
		 * result in an additional call to spec_close().
		 */
		if(modctlp = mod_shadowcsw[getmajor(dev)])	{
			MOD_HOLD(modctlp);
		}
	}

	return vp;
}

/* XENIX Support */
/*
 * XENIX rdchk() support.
 */
int
spec_rdchk(vp, cr, rvalp)
	struct vnode *vp;
	struct cred *cr;
	int *rvalp;
{
	dev_t dev;
	int error;

	if (vp->v_type != VCHR || vp->v_op != &spec_vnodeops)
		return ENOTTY;
	dev = VTOS(vp)->s_dev;
	if (cdevsw[getmajor(dev)].d_str)
		error = strioctl(vp, FIORDCHK, 0, 0, K_TO_K, cr, rvalp);
	else
		error =
		  (*cdevsw[getmajor(dev)].d_ioctl)(dev, FIORDCHK, 0, 0, cr, rvalp);
	return error;
}

/*
 * This routine is called from spec_sync.  While traversing the hash list,
 * the routine can go to sleep.  It therefore locks the hash table before
 * initiating the traversal.
 */
void
spec_flush()

{
	register struct snode **spp, *sp;
	register struct vnode *vp;
	
	SPECTBLOCK();
	for (spp = spectable; spp < &spectable[SPECTBSIZE]; spp++) {
		for (sp = *spp; sp != NULL; sp = sp->s_next) {
			vp = STOV(sp);
			/*
			 * Don't bother sync'ing a vp if it's
			 * part of a virtual swap device.
			 */
			if (IS_SWAPVP(vp))
				continue;
			if (vp->v_type == VBLK && vp->v_pages)
				(void) VOP_PUTPAGE(vp, 0, 0, B_ASYNC, sys_cred);
		}
	}
	SPECTBUNLOCK();
}
/* End XENIX Support */
