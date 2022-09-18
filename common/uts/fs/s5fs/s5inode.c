/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/s5fs/s5inode.c	1.15.3.12"
#ident	"$Header: $"

#include <util/types.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <util/cmn_err.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <fs/file.h>
#include <mem/kmem.h>
#include <io/open.h>
#include <util/param.h>
#include <fs/stat.h>
#include <mem/swap.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <svc/systm.h>
#include <util/var.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/mode.h>
#include <fs/dnlc.h>
#include <proc/proc.h>	/* XXX -- needed for user-context kludge in ILOCK */
#include <proc/disp.h>	/* XXX */
#include <fs/s5fs/s5param.h>
#include <fs/s5fs/s5dir.h>
#include <fs/s5fs/s5filsys.h>
#include <fs/s5fs/s5ino.h>
#include <fs/s5fs/s5inode.h>
#include <fs/s5fs/s5macros.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <fs/fs_subr.h>
#include <fs/fsinode.h>
#include <util/mod/moddefs.h>

/*
 * inode hashing.
 */

#define ihash(X)	(&hinode[(int) (X) & (NHINO-1)])
struct	hinode	hinode[NHINO];		/* S5 inode hash table */

STATIC void ipfree();
void iupdat(), iunhash(), tloop();
int itrunc();
extern struct vfsops s5_vfsops;

extern long int ninode;
struct fshead	s5fshead;

STATIC struct ipool	s5ifreelist;
STATIC int		s5_cleanup();
STATIC void		inoinit();


STATIC int s5_load(void);
int s5_fsflags = 0;	/* to initialize vswp->vsw_flags */

MOD_FS_WRAPPER(s5, s5_load, NULL, "Loadable s5 FS Type");

STATIC int
s5_load(void)
{
	
	inoinit();

	bzero((caddr_t)&s5fshead, sizeof(s5fshead));
	s5fshead.f_freelist = &s5ifreelist;
	s5fshead.f_inode_cleanup = s5_cleanup;
	s5fshead.f_maxpages = 1;
	s5fshead.f_isize = sizeof (struct inode);
	s5fshead.f_max = ninode;

	fs_ipoolinit(&s5fshead);
	return 0;
}

/*
 * Allocate and initialize inodes.
 */
STATIC
void
inoinit()
{
	int i;

	for (i = 0; i < NHINO; i++) {
		hinode[i].i_forw = 
			hinode[i].i_back = 
				(inode_t *) &hinode[i];
	}

	s5ifreelist.i_ff = s5ifreelist.i_fb = &s5ifreelist;
}

STATIC int
s5_cleanup(ip)
struct inode *ip;
{
	struct	vnode	*vp;
	int	skip;

	if (IPOOL_TOVP(ip) == NULL) {
		/*
		 * This inode was never used and its vnode contents were never 
		 * initialized.
		 */

		RM_IFREELIST(ip, &s5fshead);
		return(0);
	}

	vp = ITOV(ip);
	ASSERT(IPOOL_TOVP(ip) == vp);
	ASSERT(vp->v_op == &s5vnodeops);

	if (vp->v_count > 0)
		return(1);

	if (ip->i_flag & (IRWLOCKED|ILOCKED))
		return(1);

	/*
	 * This should be an ILOCK
	 */
	ip->i_flag = ILOCKED;
	ip->i_owner = curproc->p_slot; 		/* XXX -- recursive ilocks */
	ip->i_nilocks = 1;			/* XXX -- recursive ilocks */

	RM_IFREELIST(ip, &s5fshead);

	/*
	 * When the inode was put on the free list in s5inactive(),
	 * we did an asynchronous syncip() there.  Here we call
	 * syncip() to synchronously wait for any pages that are
	 * still in transit, to invalidate all the pages on the vp,
	 * and finally to write back the inode to disk.  Since
	 * syncip() may sleep, someone may find and try to acquire
	 * the inode in the meantime; if so we put it back on the
	 * free list.
	 */
	if ((vp->v_vfsp && syncip(ip, B_INVAL, 0) != 0)
	  || (ip->i_flag & IWANT)) {
		ipfree(ip);
		IUNLOCK(ip);
		return (1);
	}

	ASSERT(vp->v_pages == NULL); 
	iunhash(ip);

	if (ip->i_map)
		s5freemap(ip);

	ip->i_nilocks = 0;
	ip->i_flag &= ~ILOCKED;
	return (0);
}


/*
 * Look up an inode by vfs and i-number.  If it's in core, honor
 * the locking protocol.  If it's not in core, read it in from the
 * associated device.  In all cases, a pointer to a locked inode
 * structure is returned.
 */
int
iget(vfsp, ino, ipp)
	register struct vfs *vfsp;
	register int ino;
	struct inode **ipp;
{
	register struct hinode *hip;
	register struct vnode *vp;
	struct inode *ip;
	struct inode *iq;
	int error;

	sysinfo.iget++;
	*ipp = NULL;
loop:
	hip = ihash(ino);
	for (ip = hip->i_forw; ip != (struct inode *) hip; ip = ip->i_forw) {
		/* We assume that an inode on an hash chain does 	*/
		/* have its vnode and other pointers sane!		*/
		vp = ITOV(ip);
		ASSERT(vp == &(ip->i_vnode));
		ASSERT(vp->v_op == &s5vnodeops);
		if (ino == ip->i_number && vfsp == vp->v_vfsp) {
			/* found the inode */
			if ((ip->i_flag & (IRWLOCKED|ILOCKED))
			  && ip->i_owner != curproc->p_slot) {	/* XXX */
				ip->i_flag |= IWANT;
				sleep((caddr_t) ip, PINOD);
				goto loop;
			}
			if (vp->v_count == 0) {
				/*
				 * Remove from freelist.
				 */
				ASSERT(ip->av_back->av_forw == ip);
				ASSERT(ip->av_forw->av_back == ip);
				RM_IFREELIST(ip, &s5fshead);
			} 
			VN_HOLD(vp);

			/* -- This should be an ILOCK -- swlocks do not get set */
			ip->i_owner = curproc->p_slot;	/* XXX */
			ip->i_nilocks++;		/* XXX */
			ip->i_flag |= ILOCKED;

			*ipp = ip;
			return 0;
		}
	}

	ip = (struct inode *)fs_iget(&s5fshead, vfsp);
	if (ip == NULL) {
		cmn_err(CE_WARN, "iget - inode table overflow");
		syserr.inodeovf++;
		return ENFILE;
	}

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
		sysinfo.s5ipage++;
	else
		sysinfo.s5inopage++;

	/* -- This  should be an  ILOCK */
	ip->i_flag = ILOCKED;
	ip->i_owner = curproc->p_slot; 		/* XXX -- recursive ilocks */
	ip->i_nilocks = 1;			/* XXX -- recursive ilocks */

	/*
	 * When the inode was put on the free list in s5inactive(),
	 * we did an asynchronous syncip() there.  Here we call
	 * syncip() to synchronously wait for any pages that are
	 * still in transit, to invalidate all the pages on the vp,
	 * and finally to write back the inode to disk.  Since
	 * syncip() may sleep, someone may find and try to acquire
	 * the inode in the meantime; if so we put it back on the
	 * free list and loop around to find another free inode.
	 */
	if ((vp->v_vfsp && syncip(ip, B_INVAL, 0) != 0)
	  || (ip->i_flag & IWANT)) {
		ipfree(ip);
		IUNLOCK(ip);
		goto loop;
	}

	hip = ihash(ino);
	for (iq = hip->i_forw; iq != (struct inode *)hip; iq = (struct inode *)iq->i_forw) {
		register struct vnode *qvp;

		qvp = ITOV(iq);
		if (ino == iq->i_number && vfsp == qvp->v_vfsp) {
			ipfree(ip);
			IUNLOCK(ip);
			goto loop;
		}
	}


	ASSERT(vp->v_pages == NULL);
	ASSERT(vp->v_count == 0);

	/*
	 * Remove from old hash chain and insert into new one.
	 */
	ip->i_back->i_forw = ip->i_forw;
	ip->i_forw->i_back = ip->i_back;
	hip->i_forw->i_back = ip;
	ip->i_forw = hip->i_forw;
	hip->i_forw = ip;
	ip->i_back = (struct inode *) hip;

	if (ip->i_map)
		s5freemap(ip);

	/*
	 * Fill in the rest.
	 */
	vp->v_flag = 0;
	vp->v_count = 1;		/* VN_HOLD */
	vp->v_lid = (lid_t)0;		
	vp->v_vfsmountedhere = NULL;
	vp->v_op = &s5vnodeops;
	vp->v_vfsp = vfsp;
	vp->v_stream = NULL;
	vp->v_pages = NULL;
	vp->v_data = (caddr_t)ip;
	vp->v_filocks = NULL;
	ip->i_vcode = 0;
	ip->i_mapcnt = 0;
	ip->i_dev = vfsp->vfs_dev;
	if ((error = iread(ip, ino))
	  || ((vp->v_type = IFTOVT((int)ip->i_mode)) == VREG
	    && ((error = fs_vcode(vp, &ip->i_vcode)) != 0))) {
		iunhash(ip);
		ipfree(ip);
		vp->v_vfsp = NULL;
		vp->v_data = NULL; 
		vp->v_count = 0;
		IUNLOCK(ip);
	} else {
		vp->v_rdev = ip->i_rdev;
		*ipp = ip;
	}
	return error;
}

/*
 * Decrement reference count of an inode structure.
 * On the last reference, write the inode out and if necessary,
 * truncate and deallocate the file.
 */
void
iput(ip)
	register struct inode *ip;
{
	struct vnode *vp = ITOV(ip);

	ASSERT(ip->i_flag & ILOCKED);
	ASSERT(vp->v_count > 0);
	ITIMES(ip);
	IUNLOCK(ip);
	VN_RELE(vp);
}

/* ARGSUSED */
void
iinactive(ip, cr)
	register struct inode *ip;
	struct cred *cr;
{
 	/* if inode has already been freed, just return */
	if (IS_IPFREE(ip))
		return;

        /*
         * Mark iinactive in progress.	This allow VOP_PUTPAGE to abort
	 * a concurrent attempt to flush a page due to pageout/fsflush.
	 */
	ASSERT((ip->i_flag & IINACTIVE) == 0);
	ip->i_flag |= IINACTIVE;

	/* itruc may take some time, so preempt */
	/* PREEMPT(); this is the wrong place, may corrupt the inode */
	ILOCK(ip);
	if (ip->i_nlink <= 0) {
		ip->i_gen++;
		(void) itrunc(ip);
		ip->i_flag |= IUPD|ICHG;
		ifree(ip);
	} else if (!IS_SWAPVP(ITOV(ip))) {
		/*
		 * Do an async write (B_ASYNC) of the pages
		 * on the vnode and put the pages on the free
		 * list when we are done (B_FREE).  This action
		 * will cause all the pages to be written back
		 * for the file now and will allow update() to
		 * skip over inodes that are on the free list.
		 */
		(void) syncip(ip, B_FREE | B_ASYNC, 0);
	}

	if (ip->i_flag & (IACC|IUPD|ICHG|IMOD)) {
		if ((ip->i_flag & IUPD)
		  && (ip->i_mode & IFMT) == IFREG && ip->i_map)
			s5freemap(ip);
		/*
		 * Only call iupdat if an ifree has not been done; this
		 * avoids a race whereby an ifree could put an inode on
		 * the freelist, the inode could be allocated, and then
		 * the iupdat could put outdated information into the
		 * disk inode.
		 */
		iupdat(ip);
	}

        /* Clear the IINACTIVE flag */
	ip->i_flag &= ~IINACTIVE;

	ASSERT((ITOV(ip))->v_count == 0);
	ipfree(ip);

	IUNLOCK(ip);
	PREEMPT();
}

/*
 * Purge any cached inodes on the given VFS.  If "force" is 0,
 * -1 is returned if an active inode (other than the filesystem root)
 * is found, otherwise 0.  If "force" is non-zero, the search
 * doesn't stop if an active inode is encountered.
 */
int
iflush(vfsp, force)
	register struct vfs *vfsp;
	int force;
{
	register struct inode *ip;
	register struct inode *nip;
	register struct vnode *vp, *rvp;
	register int i;
	register struct hinode *hip;
	dev_t dev;

	rvp = S5VFS(vfsp)->vfs_root;
	dev = vfsp->vfs_dev;
	ASSERT(rvp != NULL);

	/*
	 * This search runs through the hash chains (rather
	 * than the entire inode table) so that we examine
	 * inodes that we know are currently valid.
	 */

	for (hip=hinode; hip < &hinode[NHINO]; hip++) {
		for (ip = hip->i_forw; ip != (struct inode *)hip; ip = nip) {

			ASSERT(ITOV(ip) == &(ip->i_vnode));
			ASSERT(ITOV(ip)->v_op == &s5vnodeops);

			/* The next ip should be checked each time it sleeps.
			 * If it has changed it should be set to the begining of
			 * the hash list.
			 * The inode pool should be locked so that it does not 
			 * get released while it was sleeping.
			 */
			nip = ip->i_forw;
			if (ip->i_dev == dev) {
				vp = ITOV(ip);
				if (vp == rvp) {
					if (vp->v_count > 1 && force == 0)
						return -1;
					ILOCK(ip);
					(void) syncip(ip, B_INVAL, 1);
					if (ip->i_forw != nip)
						nip = hip->i_forw;
					IUNLOCK(ip);
					continue;
				}
				if (vp->v_count == 0) {
					if (vp->v_vfsp == 0)
						continue;
					if ((ip->i_flag & IRWLOCKED)
					  || (ip->i_flag & ILOCKED)) {
						if (force)
							continue;
						return -1;
					}
					/*
					 * Thoroughly dispose of this inode.  Flush
					 * any associated pages and remove it from
					 * its hash chain.
					 */
					ILOCK(ip);	/* Won't sleep */
					if (ip->i_map)
						s5freemap(ip);
					(void) syncip(ip, B_INVAL, 1);
					if (ip->i_forw != nip)
						nip = hip->i_forw;
					if (ip->i_flag & IWANT) {
						IUNLOCK(ip);
						if (force) 
							continue;
						return -1;
					}
					IUNLOCK(ip);
					iunhash(ip);
				} else if (force == 0)
					return -1;
			}
		}
	}
	return 0;
}


/*
 * Processing a remount.
 *
 * Assumptions:
 *	- remount only valid on a read-only fs
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
s5iremount(vfsp)
	struct vfs *vfsp;
{
	register struct inode *ip;
	register struct inode *nip;
	register struct	hinode *hip;
	register struct vnode *vp;
	register struct s5vfs *s5vfsp;
	struct buf *bp;
	struct dinode *dp;
	struct filsys *fs;
	dev_t dev;
	int i;
	
	/*
	 * File system is mounted read-only at present.
	 */
	fs = getfs(vfsp);
	ASSERT(fs->s_ronly);

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
        for (hip=hinode; hip < &hinode[NHINO]; hip++) {
                for (ip = hip->i_forw; ip != (struct inode *)hip; ip = nip) {

			ASSERT(ITOV(ip) == &(ip->i_vnode));
			ASSERT(ITOV(ip)->v_op == &s5vnodeops);

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
			s5vfsp = S5VFS(vp->v_vfsp);

			/*
			 * Invalidate pages.
			 */
			if (vp->v_pages && vp->v_type != VCHR)
				VOP_PUTPAGE(vp, 0, 0, B_INVAL, sys_cred);

			/*
			 * Invalidate inodes unless in use.
			 */
			if (vp->v_count || (ip->i_flag & IWANT)) {
				/*
				 * Compare incore inode with disk inode.
				 * Don't continue if they are different.
				 * Make an exception for the access time.
				 */
				i = VBSIZE(vp);
				bp = bread(ip->i_dev, FsITOD(s5vfsp, ip->i_number), i);
				if (bp->b_flags & B_ERROR) {
					brelse(bp);
					IUNLOCK(ip);
					return EINVAL;
				}
				dp = (struct dinode *)bp->b_un.b_addr;
				dp += FsITOO(s5vfsp, ip->i_number);
				dp->di_atime = ip->i_atime;
				if (s5icmp(ip, dp)) {
					cmn_err(CE_WARN, "s5iremount: incore and disk copies of inode %d are not in sync\n", ip->i_number);
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
					s5freemap(ip);
				ip->i_flag &= ~(IUPD|IACC|ICHG|IMOD);
				iunhash(ip);
			}

			IUNLOCK(ip);

			if (ip->i_forw != nip)
				nip = hip->i_forw;
		}
	}

	return 0;
}






/*
 * Put an in-core inode on the free list.
 */
STATIC void
ipfree(ip)
	register struct inode *ip;
{
	ASSERT(ip->av_forw == NULL);
	ASSERT(ip->av_back == NULL);

	if (ip->i_mode == 0 || ITOV(ip)->v_pages == NULL)
		fs_iret(&s5fshead, (ipool_t *)ip, 1);
	else 
		fs_iret(&s5fshead, (ipool_t *)ip, 0);	
}

/*
 * Remove an inode from its hash list.
 */
void
iunhash(ip)
	register struct inode *ip;
{
	ip->i_back->i_forw = ip->i_forw;
	ip->i_forw->i_back = ip->i_back;
	ip->i_forw = ip->i_back = ip;
}

/*
 * Update times on inode.
 */
void
iuptimes(ip)
	register struct inode *ip;
{
	if (ip->i_flag & (IACC|IUPD|ICHG)) {
		ip->i_flag |= IMOD;
		if (ip->i_flag & IACC)
			ip->i_atime = hrestime.tv_sec;
		if (ip->i_flag & IUPD) 
			ip->i_mtime = hrestime.tv_sec;
		if (ip->i_flag & ICHG) 
			ip->i_ctime = hrestime.tv_sec;
		ip->i_flag &= ~(IACC|IUPD|ICHG);
	}
}

/*
 * Free all the disk blocks associated with the specified inode
 * structure.  The blocks of the file are removed in reverse order.
 * This FILO algorithm will tend to maintain a contiguous free list
 * much longer than FIFO.
 *
 * Update inode first with zero size and block addrs to ensure sanity.
 * Save blocks addrs locally to free.
 */
int
itrunc(ip)
	register struct inode *ip;
{
	register int i, type;
	register struct vnode *vp = ITOV(ip);
	register struct vfs *vfsp;
	register daddr_t bn;
	daddr_t save[NADDR];

	ASSERT(ip->i_flag & ILOCKED);
	type = ip->i_mode & IFMT;
	if (type != IFREG && type != IFDIR && type != IFLNK)
		return 0;

	/*
	 * If file is currently in use for swap, disallow truncate-down.
	 */
	if (ip->i_size > 0 && IS_SWAPVP(vp))
		return EBUSY;

	if ((ip->i_mode & IFMT) == IFREG && ip->i_map)
		s5freemap(ip);

	/*
	 * Update the pages associated with the file.
	 */
	pvn_vptrunc(vp, 0, (u_int) 0);

	vfsp = vp->v_vfsp;
	ip->i_size = 0;
	for (i = NADDR - 1; i >= 0; i--) {
		save[i] = ip->i_addr[i];
		ip->i_addr[i] = 0;
	}
	ip->i_flag |= IUPD|ICHG|ISYN;
	iupdat(ip);

	for (i = NADDR - 1; i >= 0; i--) {
		if ((bn = save[i]) == 0)
			continue;

		switch (i) {

		default:
			blkfree(vfsp, bn);
			break;

		case NADDR-3:
			tloop(vfsp, bn, 0, 0);
			break;

		case NADDR-2:
			tloop(vfsp, bn, 1, 0);
			break;

		case NADDR-1:
			tloop(vfsp, bn, 1, 1);
		}
	}
	return 0;
}

void
tloop(vfsp, bn, f1, f2)
	register struct vfs *vfsp;
	daddr_t bn;
{
	dev_t dev;
	register i;
	register struct buf *bp;
	register daddr_t *bap;
	register daddr_t nb;
	struct s5vfs *s5vfsp = S5VFS(vfsp);

	dev = vfsp->vfs_dev;
	bp = NULL;
	for (i = s5vfsp->vfs_nindir-1; i >= 0; i--) {
		if (bp == NULL) {
			bp = bread(dev, bn, vfsp->vfs_bsize);
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				return;
			}
			bap = bp->b_un.b_daddr;
		}
		nb = bap[i];
		if (nb == (daddr_t)0)
			continue;
		/*
		 * Move following 2 lines out of "if" so that buffer
		 * guaranteed to be released before calling mfree, thus
		 * avoiding the rare deadlock whereby we would have a
		 * buffer locked here but couldn't get the super block lock,
		 * and someone in alloc would have the super block lock and
		 * would not be able to get the buffer lock that is locked
		 * here.
		 */
		brelse(bp);
		bp = NULL;
		if (f1)
			tloop(vfsp, nb, f2, 0);
		else
			blkfree(vfsp, nb);
	}
	if (bp != NULL)
		brelse(bp);
	blkfree(vfsp, bn);
}

/*
 * Lock an inode.
 */
void
ilock(ip)
	register struct inode *ip;
{
	ILOCK(ip);
}

/*
 * Unlock an inode.
 */
void
iunlock(ip)
	register struct inode *ip;
{
	IUNLOCK(ip);
}


int
s5init(vswp, fstype)
	struct vfssw *vswp;
	int fstype;
{
	inoinit();
	vswp->vsw_vfsops = &s5_vfsops;

	bzero((caddr_t)&s5fshead, sizeof(s5fshead));
	s5fshead.f_freelist = &s5ifreelist;
	s5fshead.f_inode_cleanup = s5_cleanup;
	s5fshead.f_maxpages = 1;
	s5fshead.f_isize = sizeof (struct inode);
	s5fshead.f_max = ninode;

	fs_ipoolinit(&s5fshead);
	return 0;
}

#define	TST_GROUP	3
#define	TST_OTHER	6

/*
 * Check mode permission on inode.  Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the read-only status of the file system
 * is checked.  Also in WRITE, prototype text segments cannot be
 * written.  The mode is shifted to select the owner/group/other
 * fields.
 */
int
iaccess(ip, mode, cr)
	register struct inode *ip;
	register int mode;
	register struct cred *cr;
{
	struct vnode *vp = ITOV(ip);
	register int	denied_mode, lshift;
		 int	i;

	if ((mode & IWRITE) && (vp->v_vfsp->vfs_flag & VFS_RDONLY))
		return EROFS;
	if (cr->cr_uid == ip->i_uid)
		lshift = 0;			/* TST OWNER */
	else if (groupmember(ip->i_gid, cr)) {
		mode >>= TST_GROUP;
		lshift = TST_GROUP;
	}
	else {
		mode >>= TST_OTHER;
		lshift = TST_OTHER;
	}
	if ((i = (ip->i_mode & mode)) == mode) {
		return 0;
	}
	denied_mode = (mode & (~i));
	denied_mode <<= lshift;
	if ((denied_mode & (IREAD | IEXEC)) && pm_denied(cr, P_DACREAD))
		return (EACCES);
	if ((denied_mode & IWRITE) && pm_denied(cr, P_DACWRITE))
		return (EACCES);
	return (0);
}

/*
 * Flush all the pages associated with an inode using the given flags,
 * then force inode information to be written back.
 */
int
syncip(ip, flags, flags2)
	register struct inode *ip;
	int flags;
	int flags2;
{
	int error;
	register struct vnode *vp = ITOV(ip);

	if (vp->v_pages == NULL || vp->v_type == VCHR)
		error = 0;
	else
		error = VOP_PUTPAGE(vp, 0, 0, flags, sys_cred);
	if (ip->i_flag & (IUPD|IACC|ICHG|IMOD)) {
		if (((flags & B_ASYNC) == 0) && (flags2 == 0))
			ip->i_flag |= ISYN;
		iupdat(ip);
	}
	return error;
}

void
inull(vfsp)
	register struct vfs *vfsp;
{
	register struct inode *ip;
	register struct hinode *hip;
	register struct vnode *vp, *rvp;
	register int i;
	dev_t dev = vfsp->vfs_dev;

	rvp = S5VFS(vfsp)->vfs_root;
	ASSERT(rvp != NULL);

	for (hip = hinode; hip < &hinode[NHINO]; hip++) {
		for (ip = hip->i_forw; ip != (struct inode *)hip; 
		     ip = (struct inode *)ip->i_forw) {

			ASSERT(ITOV(ip) == &(ip->i_vnode));
			ASSERT(ITOV(ip)->v_op == &s5vnodeops);

			if (ip->i_dev == dev) {
				vp = ITOV(ip);
				if (vp == rvp)
					continue;
				vp->v_vfsp = 0;
			}
		}
	}
}
