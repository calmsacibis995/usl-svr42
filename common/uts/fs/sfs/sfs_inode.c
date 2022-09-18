/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/sfs/sfs_inode.c	1.15.6.18"
#ident	"$Header: $"

#include <acc/dac/acl.h>
#include <acc/mac/covert.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dnlc.h>
#include <fs/fs_subr.h>
#include <fs/fsinode.h>
#include <fs/mode.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/mod/moddefs.h>

#ifdef QUOTA
#include <fs/sfs/sfs_quota.h>
#endif

/* control structure for covert channel limiter */
STATIC ccevent_t cc_spec_unlink = { CC_SPEC_UNLINK, CCBITS_SPEC_UNLINK };

/*
 * Inode hashing 
 */

#if	((INOHSZ&(INOHSZ-1)) == 0)
#define	INOHASH(dev, ino)	(((dev)+(ino))&(INOHSZ-1))
#else
#define	INOHASH(dev, ino)	(((unsigned)((dev)+(ino)))%INOHSZ)
#endif
union ihead sfs_ihead[INOHSZ];	/* SFS/UFS hash table */

extern int sfs_ninode;		/* # of slots in inode table, from sfs.cf */

extern int sfs_ifree();
void sfs_idrop(), sfs_iinactive(), sfs_iupdat(), sfs_ilock();
void sfs_iunlock(), sfs_remque(), sfs_insque();
struct fshead	sfs_fshead;

STATIC mode_t		sfs_daccess();
STATIC void		sfs_ihinit();
STATIC struct ipool	sfs_ifreelist;
STATIC int		sfs_cleanup();
STATIC long		sfs_indirtrunc();

struct timeval	sfs_iuniqtime;

STATIC int sfs_load(void);

int sfs_fsflags = 0;	/* initialize vswp->vsw_flags */

MOD_FS_WRAPPER(sfs, sfs_load, NULL, "Loadable SFS FS Type");

STATIC int
sfs_load(void)
{
	sfs_ihinit();

	/* Initialize ipool data */
	sfs_fshead.f_freelist = &sfs_ifreelist;
	sfs_fshead.f_inode_cleanup = sfs_cleanup;
	sfs_fshead.f_maxpages = 1;
	sfs_fshead.f_isize = sizeof (struct inode);
	sfs_fshead.f_max = sfs_ninode;

	fs_ipoolinit(&sfs_fshead);
	return 0;
}

/*
 * Initialize the vfs structure
 */

int
sfsinit(vswp, fstype)
	struct vfssw *vswp;
	int fstype;
{
	sfs_ihinit();
	
	/* Associate vfs and vnode operations */

	vswp->vsw_vfsops = &sfs_vfsops;

	/* Initialize ipool data */
	sfs_fshead.f_freelist = &sfs_ifreelist;
	sfs_fshead.f_inode_cleanup = sfs_cleanup;
	sfs_fshead.f_maxpages = 1;
	sfs_fshead.f_isize = sizeof (struct inode);
	sfs_fshead.f_max = sfs_ninode;

	fs_ipoolinit(&sfs_fshead);
	return 0;
}


/*
 * Initialize hash links for inodes
 */
STATIC void
sfs_ihinit()
{
	register int i;
	register union  ihead *ih = sfs_ihead;


	for (i = INOHSZ; --i >= 0; ih++) {
		ih->ih_head[0] = ih;
		ih->ih_head[1] = ih;
	}

	sfs_ifreelist.i_ff = sfs_ifreelist.i_fb = &sfs_ifreelist;

	return;
}

STATIC int
sfs_cleanup(ip)
struct inode *ip;
{
	struct vnode	*vp;

	if (IPOOL_TOVP(ip) == NULL) {
		/*
		 * This inode was never used and its vnode contents were
		 * never initialized.
		 */
		RM_IFREELIST(ip, &sfs_fshead);
		return(0);
	}

	vp = ITOV(ip);
	ASSERT(IPOOL_TOVP(ip) == vp);
	ASSERT(vp->v_op == &sfs_vnodeops);


	if (vp->v_count > 0)
		return(1);

	if (ip->i_flag & (IRWLOCKED | ILOCKED | IREF))
		return(1);

/*
 * IREF must be or'd in so that the state of the other flags
 * is not destroyed.  In particular, sfs_syncip which is called
 * in a few lines, looks at other flags to see if the inode
 * needs to be written to the buffer cache; thus these flags
 * must be preserved.
 */
	ip->i_flag |= IREF;
	ILOCK(ip);
	RM_IFREELIST(ip, &sfs_fshead);

	if ((sfs_syncip(ip, B_INVAL, IUP_DELAY) != 0) ||
			(ip->i_flag & IWANT))  {
		VN_HOLD(vp);
		sfs_idrop(ip);
		return(1);
	}

	ASSERT(vp->v_pages == NULL);
	remque(ip);
	if (ip->i_map)
		sfs_freemap(ip);

	if (ITOI_SEC(ip) != NULL) {
		kmem_free(ITOI_SEC(ip), sizeof(union i_secure));
		ITOI_SEC(ip) = NULL;
	}
	

	IUNLOCK(ip);
	return(0);

}



/*
 * Look up an inode by device, inumber.  If it is in core (in the
 * inode structure), honor the locking protocol.  If it is not in
 * core, read it in from the specified device after freeing any pages.
 * In all cases, a pointer to a locked inode structure is returned.
 */
int
sfs_iget(vfsp, fs, ino, ipp, cr)
	register struct vfs *vfsp;
	register struct fs *fs;
	ino_t ino;
	struct inode **ipp;
	struct cred *cr;
{
	struct inode *ip;
	register union  ihead *ih;
	register struct buf *bp;
	register struct dinode *dp;
	register struct vnode *vp;
	register struct inode *iq;
	int error = 0;

	sysinfo.iget++;
	*ipp = NULL;
	/*
	 * Lookup inode in cache.
	 */
loop:
	ASSERT(getfs(vfsp) == fs);
	ih = &sfs_ihead[INOHASH(vfsp->vfs_dev, ino)];
	for (ip = ih->ih_chain[0]; ip != (struct inode *)ih; ip = ip->i_forw) {
		if (ino == ip->i_number && vfsp->vfs_dev == ip->i_dev) {
			/*
			 * Found it - check for locks.
			 */
			if ((ip->i_flag & (IRWLOCKED | ILOCKED)) &&
			    ip->i_owner != curproc->p_slot) {
				ip->i_flag |= IWANT;
				(void) sleep((caddr_t)ip, PINOD);
				goto loop;
			}
			/*
			 * If inode is on free list, remove it.
			 */
			if ((ip->i_flag & IREF) == 0) {
				ASSERT((ip)->i_freef != NULL);
				ASSERT((ip)->i_freeb != NULL);	
				RM_IFREELIST(ip, &sfs_fshead);
			}
			ASSERT(ITOV(ip) != NULL);
			ASSERT(ITOV(ip)->v_op == &sfs_vnodeops);
			ASSERT(ITOV(ip)->v_data == (caddr_t)ip);
			/*
			 * Lock the inode and mark it referenced and return it.
			 */
			ip->i_flag |= IREF;
			sfs_ilock(ip);
			VN_HOLD(ITOV(ip));
			*ipp = ip;

			return error;
		}
	}

	ip = (struct inode *)fs_iget(&sfs_fshead, vfsp);
	if (ip == NULL) {
		cmn_err(CE_WARN, "sfs_iget - inode table overflow");
		syserr.inodeovf++;
		return (ENFILE);
	}

	/* At this point, we have an inode. We need to perform		*/
	/* some more file system specific clean up actions -- which	*/
	/* include: free unrequired storage, quota buffers, etc. 	*/
	/* First: make sure that the required inode has not already	*/
	/* appeared on the hash chain.					*/

	/* 
	 * Check if this is an inode from a new allocated inode group 
	 * and if so initialize the vnode and hash pointers
	 */
	if (IPOOL_TOVP(ip) == NULL) {
		IPOOL_TOVP(ip) = ITOVADDR(ip);
		ip->i_forw = ip->i_back = ip;
	}

	vp = ITOV(ip);

	if ((ip->i_mode != 0) && (vp->v_pages != NULL)) 
		sysinfo.sfsipage++;
	else
		sysinfo.sfsinopage++;

	/*
	 * IREF must be or'd in so that the state of the other flags
	 * is not destroyed.  In particular, sfs_syncip which is called
	 * in a few lines, looks at other flags to see if the inode
	 * needs to be written to the buffer cache; thus these flags
	 * must be preserved.
	 */
	ip->i_flag |= IREF;
	ILOCK(ip);
	if ((sfs_syncip(ip, B_INVAL, IUP_DELAY) != 0) || (ip->i_flag & IWANT)) {
		VN_HOLD(vp);
		sfs_idrop(ip);
		goto loop;
	}

	for (iq = ih->ih_chain[0]; 
	     iq != (struct inode *)ih; iq = iq->i_forw) {
		if (ino == iq->i_number && vfsp->vfs_dev == iq->i_dev) {
			VN_HOLD(vp);
			sfs_idrop(ip);
			goto loop;
		}
	}

	remque(ip);
	insque(ip, ih);
	if (ip->i_map)
		sfs_freemap(ip);


	ip->i_dev = vfsp->vfs_dev;
	ip->i_devvp = ((struct sfs_vfs *)vfsp->vfs_data)->vfs_devvp;
	ip->i_number = ino;

	vp->v_op = &sfs_vnodeops;
	vp->v_data = (caddr_t)ip;
	vp->v_flag = 0;
	vp->v_macflag = 0;

	if(!UFSVFSP(vfsp)) {
		if (ITOI_SEC(ip) == NULL)
			ITOI_SEC(ip) = kmem_zalloc(sizeof(union i_secure), KM_SLEEP);
	} else {
		if (ITOI_SEC(ip) != NULL) {
			kmem_free((ITOI_SEC(ip)), sizeof(union i_secure));
			ITOI_SEC(ip) = NULL;
		}
	}
	
	ASSERT(vp->v_pages == NULL);
	ASSERT(vp->v_count == 0);

	/*
	 * Move the inode on the chain for its new (ino, dev) pair
	 */

	ip->i_diroff = 0;
	ip->i_dirofflid = cr->cr_lid;
	ip->i_fs = fs;
	ip->i_nextr = 0;
	ip->i_vcode = 0;
	ip->i_opencnt = 0;
				/* just added for sanity */
	ip->i_map = NULL;
	ip->i_dquot = NULL;
	ip->i_mapcnt = 0;

	bp = bread(ip->i_dev, (daddr_t)fragstoblks(fs, itod(fs, ino)),
	    (int)fs->fs_bsize);
	/*
	 * Check I/O errors and get vcode
	 */
	if ((error = (bp->b_flags & B_ERROR) ? EIO : 0) != 0) {

		brelse(bp);
		/*
		 * The inode doesn't contain anything useful, so it
		 * would be misleading to leave it on its hash chain.
		 * `sfs_iput' will take care of putting it back on the
		 * free list.
		 */
		remque(ip);
		/*
		 * We also lose its inumber, just in case (as sfs_iput
		 * doesn't do that any more) - but as it isn't on its
		 * hash chain, I doubt if this is really necessary.
		 * (probably the two methods are interchangable)
		 */
		ip->i_number = 0;
		sfs_iunlock(ip);
		sfs_iinactive(ip, cr);
		return (error);
	}
	dp = (struct dinode *)bp->b_un.b_addr;
	dp += itoo(fs, ino);
	ip->i_ic = dp->di_ic;			/* structure assignment */
	if (IFTOVT(ip->i_mode) == VREG
	  && ((error = fs_vcode(vp, &ip->i_vcode)) != 0)) {
		remque(ip);
		ip->i_number = 0;
		sfs_iunlock(ip);
		sfs_iinactive(ip, cr);
		return (error);
	}
	if (!UFSVFSP(vfsp)) {
		ASSERT(ITOI_SEC(ip) != NULL);
		ip->is_ic = (dp + 1)->di_ic;	/* alternate inode */
		vp->v_macflag |= VMAC_SUPPORT;
		vp->v_lid = ip->i_lid;
		if(ip->i_sflags & ISD_MLD)
			vp->v_macflag |= VMAC_ISMLD;
	}
        /* the following is a sanity check so that mount */
        /* doesn't succeed on a corrupted file system with a good superblock */
        /* This assumes that all root ufs inodes have EFT_MAGIC set, and */
        /* that no one will overwrite the rest of the inode with garbage */
        /* but leave EFT_MAGIC set... */
        /* This "fix" gets around a panic on unmount of a bad file system */
        /* by not allowing it to be mounted in the first place. */

        if (ip->i_eftflag != (u_long)EFT_MAGIC && ino == (ino_t)SFSROOTINO) {
                brelse(bp);
                remque(ip);
                ip->i_number = 0;
                sfs_iunlock(ip);
                sfs_iinactive(ip);
                return(EINVAL);
        } else
	if (ip->i_eftflag != EFT_MAGIC) {
		register int ftype;

		ip->i_mode = ip->i_smode;
		ip->i_uid = ip->i_suid;
		ip->i_gid = ip->i_sgid;
		ftype = ip->i_mode & IFMT;
		if (ftype == IFBLK || ftype == IFCHR)
			ip->i_rdev = expdev(ip->i_oldrdev);
		ip->i_eftflag = (u_long)EFT_MAGIC;
	}
	/*
	 * Fill in the rest.
	 */
	vp->v_count = 1;
	vp->v_vfsp = vfsp;
	vp->v_stream = NULL;
	vp->v_pages = NULL;
	vp->v_filocks = NULL;
	vp->v_type = IFTOVT(ip->i_mode);
	vp->v_rdev = ip->i_rdev;
	if (ino == (ino_t)SFSROOTINO) {
		vp->v_flag |= VROOT;
	}
	brelse(bp);
#ifdef QUOTA
	if (ip->i_mode != 0)
		ip->i_dquot = sfs_getinoquota(ip, cr);
#endif
	*ipp = ip;
	return error;
}

/*
 * Unlock inode and vrele associated vnode
 */
void
sfs_iput(ip)
	register struct inode *ip;
{

	ASSERT(ip->i_flag & ILOCKED);
	sfs_iunlock(ip);
	ITIMES(ip);
	VN_RELE(ITOV(ip));
	return;
}

/*
 * Check that inode is not locked and release associated vnode.
 */
void
sfs_irele(ip)
	register struct inode *ip;
{

	ASSERT(!(ip->i_flag & ILOCKED));
	ITIMES(ip);
	VN_RELE(ITOV(ip));
	return;
}

/*
 * Drop inode without going through the normal
 * chain of unlocking and releasing.
 */
void
sfs_idrop(ip)
	register struct inode *ip;
{

	ASSERT(ip->i_freef == NULL);
	ASSERT(ip->i_freeb == NULL);

	ASSERT(ip->i_flag & ILOCKED);
	sfs_iunlock(ip);
	if (--ITOV(ip)->v_count == 0)
		sfs_ipfree(ip);

}

/*
 * Insert the inode in to the freelist without going through the normal
 * chain of unlocking and releasing.
 */
sfs_ipfree(ip)
	register struct inode *ip;
{

	ASSERT(ip->i_freef == NULL);
	ASSERT(ip->i_freeb == NULL);

	/*
	 * Clear all inode flags except IMOD.  IMOD is preserved so that
	 * when the inode is recycled, sfs_syncip can look at the IMOD
	 * flag and know whether the inode needs to be written to the
	 * buffer cache.  Note that the IUPD, ICHG, and IACC bits will be
	 * 0 by now, since the inode was sfs_iupdat'd at VOP_INACTIVE time.
	 * sfs_iupdat always clears the IUPD, ICHG, and IACC bits, even
	 * on a lazy update.
	 */
	ip->i_flag &= IMOD;

	/*
	 *  if inode is invalid or there is no page associated with
	 *  this inode, put the inode in the front of the free list
	 *  Otherwise, put the inode back on the end of the free list.
	 */
	if (ITOV(ip)->v_pages == NULL || ip->i_mode == 0)
		fs_iret(&sfs_fshead, (ipool_t *)ip, 1);
	else
		fs_iret(&sfs_fshead, (ipool_t *)ip, 0);

}

/*
 * Vnode is no longer referenced, write the inode out
 * and if necessary, truncate and deallocate the file.
 */
void
sfs_iinactive(ip, cr)
	register struct inode *ip;
	register struct cred *cr;
{
        struct  aclhdr  *ahdrp;
        struct  buf    	*bp;
        struct  fs      *fsp = ip->i_fs;
        struct  vnode   *vp = ITOV(ip);
        struct  vfs     *vfsp = vp->v_vfsp;
        struct  sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
        long            aclcnt = 0;
        uint            bsize;
        daddr_t         aclblk = (daddr_t)0;
        daddr_t         nxt_aclblk;
	mode_t mode;
	int		removed = 0;

        /* if inode has already been freed, just return */
	if (IS_IPFREE(ip))
		return;
	/*
         * Mark iinactive in progress.	This allow VOP_PUTPAGE to abort
	 * a concurrent attempt to flush a page due to pageout/fsflush.
	*/
	ASSERT((ip->i_flag & IINACTIVE) == 0);
	ip->i_flag |= IINACTIVE;

	ASSERT((ip->i_flag & (IREF|ILOCKED)) == IREF);
	ASSERT(ip->i_freeb == 0);
	ASSERT(ip->i_freef == 0);
	if ((ip->i_fs->fs_ronly == 0) &&
		((sfs_vfsp->vfs_flags & SFS_FSINVALID) == 0)) {
		sfs_ilock(ip);
		if (ip->i_nlink <= 0) {
			ip->i_gen++;
			vp->v_flag &= ~VISSWAP;	/* sfs_stickyhack */
			if ((sfs_itrunc(ip, (u_long)0, cr) != 0) &&
				((sfs_vfsp->vfs_flags & SFS_FSINVALID) != 0)) {
				/* 
				 * file system invalidated during itrunc.  
				 * skip freeing file blocks & acl blocks
				 */
				sfs_iunlock(ip);
				goto badinactive;
			}
			mode = ip->i_mode;
			ip->i_mode = 0;
			ip->i_rdev = 0;
			ip->i_oldrdev = 0;
			ip->i_flag |= IUPD|ICHG;
			ip->i_eftflag = 0;
			if (!UFSIP(ip)) {

				if (MAC_ACCESS(MACEQUAL, ip->i_lid, cr->cr_lid))
					cc_limiter(&cc_spec_unlink, cr);

				/*
				 * The inode level is not cleared.
				 * It is used in determining a covert
				 * channel event in sfs_dirmakeinode().
				 *
				 * ip->i_lid = (lid_t)0;
				 */

				ip->i_sflags = 0;

                        	if (ip->i_aclcnt > NACLI) {
                                	aclcnt = ip->i_aclcnt - NACLI;
                                	aclblk = ip->i_aclblk;
                        	}
                        	while (aclcnt) {
                                	ASSERT(aclblk != (daddr_t) 0);
                                	bsize = fragroundup(fsp, aclcnt *
                                                	sizeof(struct acl) +
                                                	sizeof(struct aclhdr));
                                	if (bsize > fsp->fs_bsize)
                                        	bsize = fsp->fs_bsize;
                                	bp = pbread(ip->i_dev, NSPF(fsp) * 
							aclblk, bsize);
					if (bp->b_flags & B_ERROR) {
						brelse(bp);
						break;
					}
                                	ahdrp = (struct aclhdr *)
						(bp->b_un.b_addr);
                                	aclcnt -= (aclcnt > ahdrp->a_size) ?
                                                	ahdrp->a_size : aclcnt;
                                	nxt_aclblk = ahdrp->a_nxtblk;
                                	ASSERT ((ahdrp->a_size * 
						sizeof(struct acl) +
                                      		sizeof(struct aclhdr)) 
						<= fsp->fs_bsize);
                                	brelse(bp);
                                	if (((sfs_free(ip, aclblk, bsize) != 0))
						&& ((sfs_vfsp->vfs_flags & 
						SFS_FSINVALID) != 0)) {
						sfs_iunlock(ip);
						goto badinactive;
					}
					
                                	aclblk = nxt_aclblk;
                        	}	/* end "while (aclcnt) " */
                        	ip->i_aclblk = (daddr_t)0;
                        	ip->i_aclcnt = ip->i_daclcnt = 0;
			}	/* end "if (!UFSIP(ip))" */

			/*
			 * Set the removed flag will do sfs_ifree after the
			 * inode is updated.
			 */
			removed = 1;

		} else if (!IS_SWAPVP(ITOV(ip))) {
			/*
			 * Do an async write (B_ASYNC) of the pages
			 * on the vnode and put the pages on the free
			 * list when we are done (B_FREE).  This action
			 * will cause all the pages to be written back
			 * for the file now and will allow sfs_update() to
			 * skip over inodes that are on the free list.
			 */
			(void) sfs_syncip(ip, B_FREE | B_ASYNC, IUP_LAZY);
			sfs_iunlock(ip);
			goto badinactive;
		}
#ifdef QUOTA
		if (UFSIP(ip)) {
			(void) sfs_chkiq((struct sfs_vfs *)
				(ITOV(ip))->v_vfsp->vfs_data,
		    		ip, (uid_t)ip->i_uid, 0, cr);
			sfs_dqrele(ip->i_dquot, cr);
		}
		ip->i_dquot = NULL;
#endif
		IUPDAT(ip, IUP_DELAY)	
		if (removed)
			(void) sfs_ifree(ip, ip->i_number, mode);
		sfs_iunlock(ip);
	}

badinactive:

	sfs_ipfree(ip);
	return;
}

/*
 * Inode update routine.  This checks accessed and update flags on an inode
 * structure, and if any are on, updates the inode with the (unique) current
 * time.  The inode is copied to the buffer cache depending on the value of
 * iuptype:
 *
 *	If iuptype is IUP_SYNC, then the data is copied to the buffer cache
 *	and then bwrite is invoked to release the buffer.
 *
 *	If iuptype is IUP_DELAY, then the data is copied to the buffer cache
 *	and then btwrite is invoked to release the buffer.
 *
 *	If iuptype is IUP_LAZY, then blookup is used to look in the buffer
 *	cache to see if the buffer is present and available.  If the buffer
 *	is present, then the inode is copied to the buffer and the buffer is
 *	released via btwrite.  If the buffer is not present, then no work is
 *	done.
 *
 * Note that IUP_LAZY should not be used when the incore copy of the inode
 * is about to be destroyed.
 *
 * This gets called at least once during each FS hardening interval, since
 * sfs_sync calls sfs_flushi(SYNCATTR) which in turns calls
 * sfs_iupdat(IUP_DELAY) if the inode has changed.
 */
void
sfs_iupdat(ip, mode)
	register struct inode *ip;
	enum iupmode mode;
{
	register struct buf *bp;
	register struct fs *fp;
	struct dinode *dp;
	struct buf *(*lookup)();

	fp = ip->i_fs;
	if ((ip->i_flag & (IUPD|IACC|ICHG|IMOD)) != 0) {
		if (fp->fs_ronly)
			return;
	/*
	 * Note that we explicitly turn on the IMOD bit for now (ITIMES).
	 * If the blookup below returns NULL, then the IMOD bit will remain
	 * on in the inode so that future calls to sfs_iupdat will
 	 * continue to try to copy the inode to the buffer cache.
	 * However, if blookup finds the buffer (or bread is called),
	 * and the inode data is copied to the buffer cache, then the
	 * IMOD bit is cleared when the buffer is released via btwrite
	 * or bwrite.
	 */
		ITIMES(ip);
		ip->i_smode = ip->i_mode;
		if (ip->i_uid > 0xffff)
			ip->i_suid = UID_NOBODY;
		else
			ip->i_suid = ip->i_uid;
		if (ip->i_gid > 0xffff)
			ip->i_sgid = GID_NOBODY;
		else
			ip->i_sgid = ip->i_gid;
		if (ip->i_stamp == 0)
			ip->i_stamp = lbolt;
		if (mode == IUP_LAZY)
			lookup = blookup;
		else
			lookup = bread;
		bp = (*lookup)(ip->i_dev, (daddr_t)fragstoblks(fp,
			itod(fp, ip->i_number)), (int)fp->fs_bsize);
		if (bp == NULL)
			return;
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return;
		}
		dp = (struct dinode *)bp->b_un.b_addr + itoo(fp, ip->i_number);
		dp->di_ic = ip->i_ic;		/* structure assignment */
		if (!UFSIP(ip))
			(dp + 1)->di_ic = ip->is_ic;	/* alternate inode */
	/*
	 * If this inode is a sym link, and the target is being kept in
	 * the i_db array, make sure we zero it out in the disk inode
	 * structure before writing the inode to disk.
	 */
		if (((ip->i_mode & IFMT) == IFLNK) &&
				(ip->i_db[1] != 0) &&
				(ip->i_size <= SHORTSYMLINK)) {
			bzero((char *)&dp->di_db[1], ip->i_size);
		}
		if (mode == IUP_SYNC)
			bwrite(bp);
		else
			btwrite(bp, ip->i_stamp);
		ip->i_flag &= ~IMOD;
		ip->i_stamp = 0;
	}
	return;
}

#define	SINGLE	0	/* index of single indirect block */
#define	DOUBLE	1	/* index of double indirect block */
#define	TRIPLE	2	/* index of triple indirect block */

/*
 * Release blocks associated with the inode ip and
 * stored in the indirect block bn.  Blocks are free'd
 * in LIFO order up to (but not including) lastbn.  If
 * level is greater than SINGLE, the block is an indirect
 * block and recursive calls to sfs_indirtrunc must be used to
 * cleanse other indirect blocks.
 *
 * N.B.: triple indirect blocks are untested.
 */
STATIC long
sfs_indirtrunc(ip, bn, lastbn, level)
	register struct inode *ip;
	daddr_t bn, lastbn;
	int level;
{
	register int i;
	struct buf *bp, *copy;
	register daddr_t *bap;
	register struct fs *fs = ip->i_fs;
	daddr_t nb, last;
	long factor;
	int blocksreleased = 0, nblocks;
	int err;

	/*
	 * Calculate index in current block of last
	 * block to be kept.  -1 indicates the entire
	 * block so we need not calculate the index.
	 */
	factor = 1;
	for (i = SINGLE; i < level; i++)
		factor *= NINDIR(fs);
	last = lastbn;
	if (lastbn > 0)
		last /= factor;
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Get buffer of block pointers, zero those
	 * entries corresponding to blocks to be free'd,
	 * and update on disk copy first.
	 */
	copy = ngeteblk(fs->fs_bsize);
	bp = bread(ip->i_dev, (daddr_t)fragstoblks(fs, bn),
		(int)fs->fs_bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(copy);
		brelse(bp);
		return (0);
	}
	bap = bp->b_un.b_daddr;
	bcopy((caddr_t)bap, (caddr_t)copy->b_un.b_daddr, (u_int)fs->fs_bsize);
	bzero((caddr_t)&bap[last + 1],
	  (u_int)(NINDIR(fs) - (last + 1)) * sizeof (daddr_t));
	bwrite(bp);
	bp = copy, bap = bp->b_un.b_daddr;

	/*
	 * Recursively free totally unused blocks.
	 */
	for (i = NINDIR(fs) - 1; i > last; i--) {
		nb = bap[i];
		if (nb == 0)
			continue;
		if (level > SINGLE)
			blocksreleased +=
			    sfs_indirtrunc(ip, nb, (daddr_t)-1, level - 1);
		if ((err = sfs_free(ip, nb, (off_t)fs->fs_bsize)) != 0)
			return (err);
		blocksreleased += nblocks;
	}

	/*
	 * Recursively free last partial block.
	 */
	if (level > SINGLE && lastbn >= 0) {
		last = lastbn % factor;
		nb = bap[i];
		if (nb != 0)
			blocksreleased += sfs_indirtrunc(ip, nb, last, level - 1);
	}
	brelse(bp);
	return (blocksreleased);
}

/*
 * Truncate the inode ip to at most length size.
 * Free affected disk blocks -- the blocks of the
 * file are removed in reverse order.
 */
int
sfs_itrunc(oip, length, cr)
	register struct inode *oip;
	int length;
	struct cred *cr;
{
	register struct fs *fs = oip->i_fs;
	register struct inode *ip;
	register daddr_t lastblock;
	register off_t bsize;
	register int offset;
	int off;
	daddr_t bn, lastiblock[NIADDR];
	int level;
	long nblocks, blocksreleased = 0;
	register int i;
	struct inode tip;
	daddr_t llbn;
	int err;

	/*
	 * We only allow truncation of regular files and directories
	 * to arbritary lengths here.  In addition, we allow symbolic
	 * links to be truncated only to zero length.  Other inode
	 * types cannot have their length set here disk blocks are
	 * being dealt with - especially device inodes where
	 * ip->i_rdev is actually being stored in ip->i_db[1]!
	 */
	i = oip->i_mode & IFMT;
	if (i != IFREG && i != IFDIR && i != IFLNK)
		return (0);
	else if (i == IFLNK) {
		if (length != 0)
			return (EINVAL);
		if ((oip->i_size <= SHORTSYMLINK) && (oip->i_db[1] != 0))
			bzero((char *)&oip->i_db[1], oip->i_size);
	}
	if (length == oip->i_size) {
	/* update ctime & mtime to please POSIX tests */
		oip->i_flag |= ICHG | IUPD;
		return (0);
	}

	if (((oip->i_mode &IFMT) == IFREG) && oip->i_map)
		sfs_freemap(oip);
	
	offset = blkoff(fs, length);
	llbn = lblkno(fs, length - 1);

	if (length > oip->i_size) {

		/*
		 * Trunc up case.  BMAPALLOC will insure that the right blocks
		 * are allocated.  This includes extending the old frag to a
		 * full block (if needed) in addition to doing any work
		 * needed for allocating the last block.
		 */
		if (offset == 0)
			err = BMAPALLOC(oip, llbn, (int)fs->fs_bsize);
		else
			err = BMAPALLOC(oip, llbn, offset);

		if (err == 0) {
			oip->i_size = length;
			oip->i_flag |= ICHG;
			ITIMES(oip);
		}
		return (err);
	}

	/* Truncate-down case. */

	/*
	 * If file is currently in use for swap, disallow truncate-down.
	 */
	if (IS_SWAPVP(ITOV(oip)))
		return EBUSY;

	/*
	 * Update the pages of the file.  If the file is not being
	 * truncated to a block boundary, the contents of the pages
	 * following the end of the file must be zero'ed up to the
	 * block boundary in case they ever become accessible again
	 * because of subsequent file growth.  If the block size is
	 * smaller than the page size, the rest of the page must be
	 * zero'ed.
	 */
	off = fs->fs_bsize < PAGESIZE ? length & PAGEOFFSET : offset;
	if (off == 0)
		pvn_vptrunc(ITOV(oip), (u_int)length, (u_int)0);
	else {
		int err;

		/*
		 * Make sure that the last block is properly allocated.
		 * We only really have to do this if the last block is
		 * actually allocated since sfs_bmap will now handle the case
		 * of an fragment which has no block allocated.  Just to
		 * be sure, we do it now independent of current allocation.
		 */
		err = BMAPALLOC(oip, llbn, offset);
		if (err)
			return (err);
		bsize = llbn >= NDADDR? fs->fs_bsize : fragroundup(fs, offset);
		pvn_vptrunc(ITOV(oip), (u_int)length,
			(u_int)(MAX(PAGESIZE, bsize) - off));
	}

	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep.  Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	lastblock = lblkno(fs, length + fs->fs_bsize - 1) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR(fs);
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR(fs) * NINDIR(fs);
	nblocks = btodb(fs->fs_bsize);

	/*
	 * Update file and block pointers
	 * on disk before we start freeing blocks.
	 * If we crash before free'ing blocks below,
	 * the blocks will be returned to the free list.
	 * lastiblock values are also normalized to -1
	 * for calls to sfs_indirtrunc below.
	 */
	tip = *oip;			/* structure copy */
	ip = &tip;

	for (level = TRIPLE; level >= SINGLE; level--)
		if (lastiblock[level] < 0) {
			oip->i_ib[level] = 0;
			lastiblock[level] = -1;
		}
	for (i = NDADDR - 1; i > lastblock; i--)
		oip->i_db[i] = 0;

	oip->i_size = length;
	oip->i_flag |= ICHG|IUPD;
	sfs_iupdat(oip, IUP_SYNC);		/* do sync inode update */

	/*
	 * Indirect blocks first.
	 */
	for (level = TRIPLE; level >= SINGLE; level--) {
		bn = ip->i_ib[level];
		if (bn != 0) {
			blocksreleased +=
			    sfs_indirtrunc(ip, bn, lastiblock[level], level);
			if (lastiblock[level] < 0) {
				ip->i_ib[level] = 0;
				err = sfs_free(ip, bn, (off_t)fs->fs_bsize);
				if (err != 0)
					return (err);
				blocksreleased += nblocks;
			}
		}
		if (lastiblock[level] >= 0)
			goto done;
	}

	/*
	 * All whole direct blocks or frags.
	 */
	for (i = NDADDR - 1; i > lastblock; i--) {
		bn = ip->i_db[i];
		if (bn == 0)
			continue;
		ip->i_db[i] = 0;
		bsize = (off_t)blksize(fs, ip, i);
		err = sfs_free(ip, bn, bsize);
		if (err != 0)
			return (err);
		blocksreleased += btodb(bsize);
	}
	if (lastblock < 0)
		goto done;

	/*
	 * Finally, look for a change in size of the
	 * last direct block; release any frags.
	 */
	bn = ip->i_db[lastblock];
	if (bn != 0) {
		off_t oldspace, newspace;

		/*
		 * Calculate amount of space we're giving
		 * back as old block size minus new block size.
		 */
		oldspace = blksize(fs, ip, lastblock);
		ip->i_size = length;
		newspace = blksize(fs, ip, lastblock);
		ASSERT(newspace != 0);
		if (oldspace - newspace > 0) {
			/*
			 * Block number of space to be free'd is
			 * the old block # plus the number of frags
			 * required for the storage we're keeping.
			 */
			bn += numfrags(fs, newspace);
			err = sfs_free(ip, bn, oldspace - newspace);
			if (err != 0)
				return (err);
			blocksreleased += btodb(oldspace - newspace);
		}
	}
done:
/* BEGIN PARANOIA */
	for (level = SINGLE; level <= TRIPLE; level++)
		ASSERT(ip->i_ib[level] == oip->i_ib[level]);
	for (i = 0; i < NDADDR; i++)
		ASSERT(ip->i_db[i] == oip->i_db[i]);
/* END PARANOIA */
	oip->i_blocks -= blocksreleased;
	if (oip->i_blocks < 0) {		/* sanity */
		cmn_err(CE_NOTE, "sfs_itrunc: %s/%d new size = %d, blocks = %d\n",
		    fs->fs_fsmnt, oip->i_number, oip->i_size, oip->i_blocks);
		oip->i_blocks = 0;
	}
	oip->i_flag |= ICHG;
#ifdef QUOTA
	(void) sfs_chkdq(oip, -blocksreleased, 0, cr);
#endif
	return (0);
}

/*
 * Remove any inodes in the inode cache belonging to dev.
 * The inodes are written out to disk using bdwrite() 
 * via sfs_syncip().
 *
 * There should not be any active ones, return error if any are found but
 * still invalidate others (N.B.: this is a user error, not a system error).
 *
 * This is called from umount1()/sfs_vfsops.c when dev is being unmounted.
 * 
 *
 */
#ifndef	QUOTA
/* ARGSUSED */
#endif
int
sfs_iflush(vfsp, iq, cr)
	struct vfs *vfsp;
	struct inode *iq;
	struct cred *cr;
{
	register struct inode *ip;
	register struct inode *nip;
	register union	ihead *hip;
	register open = 0;
	register struct vnode *vp, *rvp;
	dev_t dev;
	
	open = 0;
	dev = vfsp->vfs_dev;
	rvp = ((struct sfs_vfs *)(vfsp->vfs_data))->vfs_root;


	/*
	 * This search runs through the hash chains (rather
	 * than the entire inode table) so that we examine
	 * inodes that we know are currently valid.
	 */

        for (hip=sfs_ihead; hip < &sfs_ihead[INOHSZ]; hip++) {
                for (ip = hip->ih_chain[0]; ip != (struct inode *)hip; ip = nip) {

			ASSERT(ITOV(ip) == &(ip->i_vnode));
			ASSERT(ITOV(ip)->v_op == &sfs_vnodeops);

			/* The next ip should be checked each time it sleeps.
			 * If it has changed it should be set to the begining of
			 * the hash list.
			 * The inode pool does not have to be be locked because
			 * the inode is locked, this should prevent the pool from
			 * being released while it was sleeping.
			 */

			nip = ip->i_forw;
#ifdef QUOTA
			if (ip != iq && ip->i_dev == dev) {
#else
			if (ip->i_dev == dev) {
#endif
				vp = ITOV(ip);
				if (vp == rvp) {
					if (vp->v_count > 1) {
						open = -1;
						continue;
					}
#ifdef QUOTA
					/* 
					 * we could check the ip here,
					 * but it might not be initialized.
					 * we can check the vfsp since this
					 * ip->i_dev must match the
					 * vfsp->vfs_dev.
					 */
					if (UFSVFSP(vfsp))
						sfs_dqrele(ip->i_dquot, cr);
					ip->i_dquot = NULL;
#endif
					ILOCK(ip);
					/* skip i_map; only applicable to VREG */
					(void) sfs_syncip(ip, B_INVAL,
						IUP_DELAY);

					if (ip->i_forw != nip)
                				nip = hip->ih_chain[0];

					IUNLOCK(ip);
					continue;
				}
				if (ip->i_flag & IREF) {
					open = -1;
					continue;
				}
				/*
				 * as IREF == 0, the inode was on the free
				 * list already, just leave it there, it will
				 * fall off the bottom eventually. We could
				 * perhaps move it to the head of the free
				 * list, but as umounts are done so
				 * infrequently, we would gain very little,
				 * while making the code bigger.
				 */
#ifdef QUOTA
				/* 
				 * we could check the ip here,
				 * but it might not be initialized.
				 * we can check the vfsp since this
				 * ip->i_dev must match the
				 * vfsp->vfs_dev.
				 */
				if (UFSVFSP(vfsp))
					sfs_dqrele(ip->i_dquot, cr);
				ip->i_dquot = NULL;
#endif
				ILOCK(ip);
				if (ip->i_map)
					sfs_freemap(ip);
				(void) sfs_syncip(ip, B_INVAL, IUP_DELAY);
				if (ip->i_forw != nip)
                			nip = hip->ih_chain[0];
				if (ip->i_flag & IWANT) {
					IUNLOCK(ip);
					open = -1;
					continue;
				}
				IUNLOCK(ip);
				remque(ip);
			}
		}
	}
	return (open);
}

/*
 * Processing a remount.
 *
 * Assumptions:
 *	- remount only valid on a read-only fs
 *	- quotas not on for read-only fs
 *
 * Processing:
 *	- invalidate pages associated with inodes
 *	- invalidate inodes that are not in use
 *	- check inuse incore inodes with disk counterparts
 *
 * Return:
 *	- 0 on success
 *	- EINVAL if any active incore inode is not in sync with
 *		disk counterpart
 */
int
sfs_iremount(vfsp)
	struct vfs *vfsp;
{
	register struct inode *ip;
	register struct inode *nip;
	register union	ihead *hip;
	register struct vnode *vp;
	struct buf *bp;
	struct dinode *dp;
	struct fs *fs;
	dev_t dev;
	
	/*
	 * File system is mounted read-only at present.
	 */
	fs = getfs(vfsp);
	ASSERT(fs->fs_ronly);

#ifdef QUOTA
	/*
	 * Therefore, quotas are not yet enabled, either.
	 */
	ASSERT((((struct sfs_vfs *)(vfsp->vfs_data))->vfs_qflags & MQ_ENABLED)
		== 0);
#endif

	/*
	 * Clear dnlc entries for this file system to minimize
	 * active inodes.
	 */
	dnlc_purge_vfsp(vfsp, 0);

	/*
	 * Invalidate buffer cache entries for this file system
	 * so that the disk inodes are read in for comparison
	 * with the incore copies (active inodes only).
	 */
	dev = vfsp->vfs_dev;
	binval(dev);

	/*
	 * This search runs through the hash chains (rather
	 * than the entire inode table) so that we examine
	 * inodes that we know are currently valid.
	 */
        for (hip=sfs_ihead; hip < &sfs_ihead[INOHSZ]; hip++) {
                for (ip = hip->ih_chain[0]; ip != (struct inode *)hip; ip = nip) {

			ASSERT(ITOV(ip) == &(ip->i_vnode));
			ASSERT(ITOV(ip)->v_op == &sfs_vnodeops);

			/*
			 * The next ip should be checked each time it sleeps.
			 * If it has changed it should be set to the begining of
			 * the hash list.
			 * The inode pool does not have to be be locked because
			 * the inode is locked, this should prevent the pool
			 * from being released while it was sleeping.
			 */
			nip = ip->i_forw;

			/*
			 * Process inodes on a particular fs only.
			 */
			if (ip->i_dev != dev)
				continue;

			ILOCK(ip);

			vp = ITOV(ip);

			/*
			 * Invalidate pages.
			 */
			if (vp->v_pages && vp->v_type != VCHR)
				VOP_PUTPAGE(vp, 0, 0, B_INVAL, sys_cred);

			/*
			 * Invalidate inodes unless in use.
			 */
			if (vp->v_count || (ip->i_flag & (IREF|IWANT))) {
				/*
				 * Compare incore inode with disk inode.
				 * Don't continue if they are different.
				 * Make an exception for the access time.
				 */
#ifdef	DEBUG
				cmn_err(CE_CONT, "sfs_iremount: active inode %d, v_count = %d, i_flag = 0x%x%s%s\n",
				ip->i_number, vp->v_count,
				ip->i_flag,
				(ip->i_flag & IREF) ? " IREF" : "",
				(ip->i_flag & IWANT) ? " IWANT" : "");
#endif

				bp = bread(ip->i_dev, (daddr_t)fragstoblks(fs, itod(fs, ip->i_number)), (int)fs->fs_bsize);
				if (bp->b_flags & B_ERROR) {
					cmn_err(CE_NOTE, "sfs_iremount: cannot read disk inode %d\n", ip->i_number);
					brelse(bp);
					IUNLOCK(ip);
					return EINVAL;
				}
				dp = (struct dinode *)bp->b_un.b_addr;
				dp += itoo(fs, ip->i_number);
				dp->di_atime = ip->i_atime;
				if (bcmp((caddr_t)&dp->di_ic,
					 (caddr_t)&ip->i_ic,
					 sizeof(struct dinode))
				|| (!UFSVFSP(vfsp) &&
				    bcmp((caddr_t)&(dp+1)->di_ic,
					 (caddr_t)&ip->is_ic,
					 sizeof(struct dinode)))) {
					cmn_err(CE_NOTE, "sfs_iremount: incore and disk copies of inode %d are not in sync\n", ip->i_number);
					brelse(bp);
					IUNLOCK(ip);
					return EINVAL;
				}
				brelse(bp);

			} else {
				/*
				 * Invalidate incore inode.
				 */
				if (vp->v_type == VREG && ip->i_map)
					sfs_freemap(ip);
				ip->i_flag &= ~(IUPD|IACC|ICHG|IMOD);
				remque(ip);
			}

			IUNLOCK(ip);

			if (ip->i_forw != nip)
				nip = hip->ih_chain[0];
		}
	}

	return 0;
}

/*
 * Lock an inode.
 */
void
sfs_ilock(ip)
	register struct inode *ip;
{

	ILOCK(ip);
	return;
}

/*
 * Unlock an inode.
 */
void
sfs_iunlock(ip)
	register struct inode *ip;
{

	IUNLOCK(ip);
	return;
}

/*
 * Check mode permission on inode.  Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the read-only status of the file system
 * is checked.  The mode is shifted to select the owner/group/other
 * fields.  A privileged process is granted all permissions except
 * writing to read-only file systems.
 */
int
sfs_iaccess(ip, mode, cr)
	register struct inode *ip;
	register mode_t mode;
	register struct cred *cr;
{
	register mode_t denied_mode;

	if (mode & IWRITE) {
		/*
		 * Disallow write attempts on read-only
		 * file systems, unless the file is a block
		 * or character device or a FIFO.
		 */
		if (ip->i_fs->fs_ronly != 0) {
			if ((ip->i_mode & IFMT) != IFCHR &&
			    (ip->i_mode & IFMT) != IFBLK &&
			    (ip->i_mode & IFMT) != IFIFO) {
				return (EROFS);
			}
		}
	}

	/*
         *      Perform Discretionary Access Check
         */
        if ((denied_mode = sfs_daccess(ip, mode, cr)) == 0)
                return (0);
	if ((long)denied_mode < 0)
		/* it is assumed sfs_daccess returns -1 for EIO */
		return (EIO);
	else 	{
                if ((denied_mode & (IREAD | IEXEC)) && pm_denied(cr, P_DACREAD))
                        return (EACCES);
                if ((denied_mode & IWRITE) && pm_denied(cr, P_DACWRITE))
                        return (EACCES);
                return (0);
        }

}

/*
 *      sfs_daccess -   check discretionary access permissions
 *
 *      processing -    if effective uid == owner of file, use file owner bits
 *                      if no ACL entries & effective gid == owning group,
 *                              use file group bits.
 *                      scan ACL looking for a matching user or group,
 *                              and use matching entry permissions.
 *                              Use total permissions of all matching group
 *                              entries, until ACL is exhausted.
 *                      file group bits mask permissions allowed by ACL.
 *                      if not owner, owning group, or matching entry in ACL,
 *                              use file other bits.
 *
 *      output -        0 if permission granted, otherwise denied permissions.
 *			-1 indicates we received an I/O error from pbread().
 *
 *			NOTE: sfs_iaccess expects the denied permissions
 *			to be contained in the 1st octal permissions bit
 *			(e.g. 0400, 0200) for comparison with IREAD,IWRITE,
 *			or IEXEC prior to privilege checking.
 *			So we must make sure to shift the denied access
 *			to the correct position before returning it.
 *
 */
STATIC mode_t
sfs_daccess(ip, mode, cr)
	register struct inode   *ip;
	mode_t                   mode;
	register struct cred    *cr;
{
        register struct acl     *aclp;
        register mode_t         workmode = 0;
        register mode_t         reqmode;
        register mode_t         basemode;
        register int            cnt, i;
        struct buf             	*bp = NULL;
        struct aclhdr           *ahdrp;
        struct fs               *fsp = ip->i_fs;
        daddr_t                 aclblk;
        long                    bsize;
        long                    entries = 0;
        int                     idmatch = 0;


        /*
         *      check if effective uid == owner of file
         */

        if (cr->cr_uid == ip->i_uid) {
                if ((workmode = (ip->i_mode & mode)) == mode)
                        return 0;
                else
                        return (mode & ~workmode);
        }
        mode >>= 3;
        /*
         *      If there's no ACL, check only the group &
         *      other permissions (Just like the "Good Old Days")
         */
	if ((UFSIP(ip)) || dac_installed == 0 ||
	    ((entries = (long)(ip->i_aclcnt - ip->i_daclcnt))==0)) {
                if (groupmember(ip->i_gid, cr)) {
                        if ((workmode = (ip->i_mode & mode)) == mode)
                                return 0;
                        else
                                return ((mode & ~workmode) << 3);
                } else
                        goto other_ret;

	} 

        ASSERT(entries > 0);

        /*      set up requested & base permissions */
        reqmode = (mode >> 3) & 07;
        basemode = (ip->i_mode >> 3) & 07;

        /*
         *      Check the ACL for a matching entry
         *      first, the entries in the inode are checked
         */

        aclp = ip->i_acl;
        cnt = (entries > NACLI) ? NACLI : entries;
        entries -= cnt;
        aclblk = ip->i_aclblk;

        while (cnt) {
                for (i = cnt; i > 0; i--, aclp++) {
                        switch (aclp->a_type) {
                        case USER:
                                if (cr->cr_uid == aclp->a_id) {
                                        if ((workmode = ((aclp->a_perm & 
						reqmode) & basemode)) == 
						reqmode) {
                                        /*
                                         *      Matching USER entry found,
                                         *      access granted
                                         */
                                                if (bp)
                                                        brelse(bp);
                                                return 0;
                                        } else {
                                        /*
                                         *      Matching USER entry found,
                                         *      access not granted
                                         */
                                                if (bp)
                                                        brelse(bp);
                                                return ((reqmode & ~workmode)
								<< 6);
                                        }
                                }
                                break;
                        case GROUP_OBJ:
                                if (groupmember(ip->i_gid, cr)) {
                                        if ((workmode |= (aclp->a_perm
                                                & reqmode)) == reqmode)
                                                goto match;
                                        else
                                                idmatch++;
                                }
                                break;
                        case GROUP:
                                if (groupmember(aclp->a_id, cr)) {
                                        if ((workmode |= (aclp->a_perm
                                                & reqmode)) == reqmode)
                                                goto match;
                                        else
                                                idmatch++;
                                }
                                break;
                        }       /* end switch statement */
                }       /* end for statement */

                /*
                 *      next, check entries in each disk block
                 */

                if (bp)
                        brelse(bp);
                if (entries) {
                        ASSERT(aclblk != 0);
                        /* compute size of all remaining ACL entries */
                        bsize = fragroundup(fsp, entries * sizeof(struct acl) +
                                        sizeof(struct aclhdr));
                        /* if bigger than a logical block, just read a block */
                        if (bsize > fsp->fs_bsize)
                                bsize = fsp->fs_bsize;
                        bp = pbread(ip->i_dev, NSPF(fsp) * aclblk, bsize);
			if (bp->b_flags & B_ERROR) {
                                brelse(bp);
                                return ((mode_t)-1);
                        }
                        ahdrp = (struct aclhdr *)(bp->b_un.b_addr);
                        cnt =  (entries > ahdrp->a_size) ?
                                                ahdrp->a_size : entries;
                        entries -= cnt;
                        aclblk = ahdrp->a_nxtblk;

                        aclp = (struct acl *)(ahdrp + sizeof(ahdrp));
                }
                else
                        cnt = 0;
        }       /* end while statement */
        if (idmatch)
                /*
                 *      Matching GROUP or GROUP_OBJ entries
                 *      were found, but  did not grant the access.
                 */
                return ((reqmode & ~workmode) << 6);

other_ret:

        /*
         *      Not the file owner, and either
         *      no ACL, or no match in ACL.
         *      Now, check the file permissions.
         */

        mode >>= 3;
        if ((workmode = (ip->i_mode & mode)) == mode)
                return 0;
        else
                return ((mode & ~workmode) << 6);

match:
        /*
         *      Access granted by GROUP_OBJ or GROUP ACL entry or entries
         */

        if (bp)
                brelse(bp);

        /*
         *      File Group Class Bits mask access,
         *      so determine whether matched entries
         *      should really grant access.
         */
        if ((workmode & basemode) == reqmode)
                return 0;
        else
                return ((reqmode & ~(workmode & basemode)) << 6);
}


/*
 * Remove from old hash chain and insert into new one.
 */

void
sfs_remque(ip)
struct inode *ip;
{
	ip->i_back->i_forw = ip->i_forw;
	ip->i_forw->i_back = ip->i_back;
	ip->i_forw = ip->i_back = ip;
	return;
}

void
sfs_insque(ip, hip)
struct inode *ip;
union ihead *hip;
{
	hip->ih_chain[0]->i_back = ip;
	ip->i_forw = hip->ih_chain[0];
	hip->ih_chain[0] = ip;
	ip->i_back = (struct inode *) hip;
	return;
}
