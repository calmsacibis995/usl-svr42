/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/sfs/sfs_subr.c	1.16.2.7"
#ident	"$Header: $"

#include <acc/dac/acl.h>
#include <fs/buf.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_tables.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

int 			sfs_updlock;	/* synchronize between sfs_update() and sfs_unmount() */
extern struct fshead	sfs_fshead;	/* sfs/ufs filesystem head for dynamic inodes */

/*
 * Invalidate file system.  Called when a corrupt file system is
 * detected instead of panic'ing.
 */
void
sfs_fsinvalid(vfsp)
struct vfs	*vfsp;	/* mounted file system vfs structure to invalidate */
{
	struct sfs_vfs *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
	struct fs *fsp = getfs(vfsp);
	
	if (vfsp == rootvfs) {
		cmn_err(CE_PANIC, "Root file system corrupt\n");
	}

	sfs_vfsp->vfs_flags |= SFS_FSINVALID;
	cmn_err(CE_WARN, "Invalidating corrupt file system %s. \n",
		fsp->fs_fsmnt);
	cmn_err(CE_WARN, "Unmount immediately and fix. \n");
	return;
}

/*
 * sfs_update performs the sfs part of `sync'.  It goes through the disk
 * queues to initiate sandbagged IO; goes through the inodes to write
 * modified nodes; and it goes through the mount table to initiate
 * the writing of the modified super blocks.
 */
void
sfs_update()
{
	register struct vfs *vfsp;
	struct fs *fs;

	while (sfs_updlock) {
		(void) sleep((caddr_t)&sfs_updlock, PINOD);
	}
	sfs_updlock++;

	/*
	 * Write back modified superblocks.
	 * Consistency check that the superblock of
	 * each file system is still in the buffer cache.
	 */
	for (vfsp = rootvfs; vfsp != NULL; vfsp = vfsp->vfs_next)
		if (vfsp->vfs_op == &sfs_vfsops || 
		    vfsp->vfs_op == &ufs_vfsops) {
			fs = getfs(vfsp);
			if (fs->fs_fmod == 0)
				continue;
			if (fs->fs_ronly != 0) {
				cmn_err(CE_WARN, "fs = %s sfs_update: read-only filesystem modified\n",
					fs->fs_fsmnt);
				sfs_fsinvalid(vfsp);
				continue;
			}
			fs->fs_fmod = 0;
			fs->fs_time = hrestime.tv_sec;
			sfs_sbupdate(vfsp);
		}

	sfs_flushi(0);

	/*
	 * Force stale buffer cache information to be flushed,
	 * for all devices.  This should cause any remaining control
	 * information (e.g., cg and inode info) to be flushed back.
	 */
	bflush((dev_t)NODEV);
	sfs_updlock = 0;
	wakeprocs((caddr_t)&sfs_updlock, PRMPT);
	return;
}

/*
 * Flush inode cache.  If flag is set (by fsflush()) or file is a swap
 * file, don't sync pages, just update inode.
 */
void
sfs_flushi(flag)
	short flag;
{
	register struct inode *ip;
	register struct inode *nip;
	register union ihead *hip;
	register struct vnode *vp;
	register int cheap = flag & SYNC_ATTR;

	/*
	 * See if we can shrink the inode pool
	 */
	 if (sfs_fshead.f_flag & DEALLOC)
		fs_cleanone(&sfs_fshead, 0, 0);


	/*
	 * Write back each (modified) inode,
	 * but don't sync back pages if vnode is
	 * part of the virtual swap device.
	 */


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
			/* 
			 * XXX - Should this check to see if the beginning
			 * of the hash chain was changed (e.g., a new insert)?
			 */
			nip = ip->i_forw;

			vp = ITOV(ip);
			/*
			 * Skip locked inodes.
			 * Skip inodes w/ no pages and no inode changes.
			 * Skip inodes from read only vfs's.
			 */
			if ((ip->i_flag & (IRWLOCKED | ILOCKED)) ||
			    ((vp->v_pages == NULL) &&
			    ((ip->i_flag & (IMOD | IACC | IUPD | ICHG)) == 0)) ||
			    (vp->v_vfsp == NULL) ||
			    ((vp->v_vfsp->vfs_flag & VFS_RDONLY) != 0))
				continue;
			sfs_ilock(ip);
			VN_HOLD(vp);
	
			/*
			 * If this is an inode sync for file system hardening
			 * or this is a full sync but file is a swap file,
			 * don't sync pages but make sure the inode is up
			 * to date. In other cases, push everything out.
			 */
			if (cheap || IS_SWAPVP(vp)) {
				IUPDAT(ip, IUP_DELAY);
			} else {
				(void) sfs_syncip(ip, B_ASYNC, IUP_DELAY);
			}

			if(ip->i_forw != nip)
                		nip = hip->ih_chain[0];

			sfs_iput(ip);
		}
	}
	return;
}


/*
 * Flush all the pages associated with an inode using the given flags,
 * then force inode information to be written back using the given flags.
 */
int
sfs_syncip(ip, flags, mode)
	register struct inode *ip;
	int flags;
	enum iupmode mode;
{
	int error;
	register struct vnode *vp = ITOV(ip);

	if (ip->i_fs == NULL)
		return (0);			/* not active */
	if (vp->v_pages == NULL || vp->v_type == VCHR)
		error = 0;
	else
		error = VOP_PUTPAGE(vp, 0, 0, flags, sys_cred);
	if (ip->i_flag & (IUPD | IACC | ICHG | IMOD)) {
		sfs_iupdat(ip, mode);
	}
	return (error);
}

/*
 *   sfs_aclget - Get the File's extended ACLs
 *
 *   Input:     pointer to an inode
 *              pointer to kernel buffer for storing default ACL
 *		defaults flag: if set, get default entries only
 */
int
sfs_aclget(ip, aclp, defaults)
register struct inode   *ip;
register struct acl     *aclp;
int			defaults;
{
        register struct aclhdr  *ahdrp;         /* ACL header ptr */
        register uint           bsize;
        struct buf             	*bp = NULL;
        struct fs               *fsp = ip->i_fs;
        struct acl              *src_aclp;      /* ptr to source ACL */
        daddr_t                 aclblk;
        long                    dentries;       /* default entries */
        long                    aentries;       /* non-default entries */
        long                    bentries;       /* entries in current buffer */
        long                    b_aentries;     /* non-default entries in */
                                                /*    current buffer      */
        long                    b_dentries = 0; /* default entries in */
                                                /*    current buffer  */
        long                    entries;        /* total entries in ACL */
        uint                    err = 0;

	ASSERT(!UFSIP(ip));
        entries = ip->i_aclcnt;
        dentries = ip->i_daclcnt;
        aentries = entries - dentries;          /* non-default entries */

        /* Set up to copy ACL entries from the secure inode first */
        bentries = (entries > NACLI) ? NACLI : entries;
        src_aclp = ip->i_acl;
        aclblk = ip->i_aclblk;

        /*
         * Loop while default entries remain to be copied.
         * At entry to loop, we'll copy any inode based entries.
         * If any extended ACL blocks exist, read them and
         * copy them to temp buffer also.
         */
        while (entries > 0) {
                if (bentries > aentries) {      /* buffer has defaults */
                        b_dentries = bentries - aentries;
                        b_aentries = aentries;
                } else                          /* only non-defaults */
                        b_aentries = bentries;
                aentries -= b_aentries;
                dentries -= b_dentries;

                if (b_aentries > 0) {           /* non-defaults in buffer? */
			if (!defaults) {
				/* copy non-defaults */
				bcopy((caddr_t)src_aclp, (caddr_t)aclp,
					b_aentries * sizeof(struct acl));
				aclp += b_aentries;

			}
                        src_aclp += b_aentries; /* then skip past them */
		}

                if (b_dentries > 0) {           /* defaults in buffer? */
                                                /* then copy them */
                        bcopy((caddr_t)src_aclp, (caddr_t)aclp,
                        	b_dentries * sizeof(struct acl));
                        src_aclp += b_dentries;
                        aclp += b_dentries;
                }

                entries -= bentries;
                if (bp)                /* release last block if necessary */
                        brelse(bp);

                /* Read the Entries from the ACL Blocks  */
                if (aclblk) {
                        ASSERT(entries > 0);
                        bsize = fragroundup(fsp, entries * sizeof(struct acl) +
                                        sizeof (struct aclhdr));
                        if (bsize > fsp->fs_bsize)
                                bsize = fsp->fs_bsize;
                        bp = pbread(ip->i_dev, NSPF(fsp) * aclblk, bsize);
			if (bp->b_flags & B_ERROR) {
				err = bp->b_error ? bp->b_error : EIO;
                                brelse(bp);
                                return (err);
                        }

                        /* point at block header */
                        ahdrp = (struct aclhdr *)(bp->b_un.b_addr);
                        /* point at actual ACL entries in the fragment */
                        src_aclp = (struct acl *)((caddr_t)ahdrp +
                                                sizeof(struct aclhdr));
                        bentries = ahdrp->a_size;
                        aclblk = ahdrp->a_nxtblk;
                }       /* end "if (aclblk)" */
        }       /* end "while(entries)" */

        return (err);
}

/*
 *      sfs_aclstore - Store a given ACL buffer on the file
 *              This routine sets an ACL exactly from the given
 *              buffer, without the overhead of computing file
 *              modes, and determining whether all base entries
 *              should be stored with the ACL or not.
 *
 *      Input:   Pointer to the file's inode
 *               Pointer to buffer of ACL entries
 *               Number of ACL entries to save
 *               number of default ACL entries
 *
 */
int
sfs_aclstore(ip, aclp, nentries, dentries, cr)
register struct inode   *ip;
register struct acl     *aclp;
register int            nentries;
register long           dentries;
struct cred		*cr;
{
        register struct aclhdr  *ahdrp;
        register uint           bsize;
        struct vnode            *vp = ITOV(ip);
        struct vfs              *vfsp = vp->v_vfsp;
        struct sfs_vfs          *sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
        struct buf             	*bp = NULL;
        struct buf             	*lbp = NULL;
        struct fs               *fsp = ip->i_fs;
        struct aclhdr           *lahdrp=NULL;
        daddr_t                 aclblk = (daddr_t)0;
        daddr_t                 laclblk=0;
        long                    acls;           /* working count */
        uint                    lbsize;
        uint                    err = 0;

	ASSERT(!UFSIP(ip));
        ip->i_aclblk = aclblk;

        /* Copy Entries into Secure Inode */

        ip->i_aclcnt = nentries;
        ip->i_daclcnt = dentries;
        acls = (nentries > NACLI) ? NACLI : nentries;
        nentries -= acls;
        bcopy((caddr_t)aclp, (caddr_t)&ip->i_acl, acls * sizeof(struct acl));
        aclp += acls;

        /* Allocate any necessary ACL blocks, one at a time */

        while (nentries) {
                bsize = fragroundup(fsp, (nentries * sizeof(struct acl)) +
                                        sizeof(struct aclhdr));
                if (bsize > fsp->fs_bsize)
                        bsize = fsp->fs_bsize;

                if ((err = sfs_alloc(ip, (daddr_t)0, bsize, &aclblk, cr)) != 0)
                /* No Space on Disk, Deallocate All Blocks */
                        goto dealloc;

                bp = pbread(ip->i_dev, NSPF(fsp) * aclblk, bsize);
		if (bp->b_flags & B_ERROR) {
                        err = bp->b_error ? bp->b_error : EIO;
                        goto dealloc;
		}

                /* point at ACL block header in correct fragment */
                ahdrp = (struct aclhdr *)(bp->b_un.b_addr);
                ahdrp->a_ino = (ino_t)ip->i_number;
                acls = (bsize - sizeof(struct aclhdr)) / sizeof(struct acl);
                ahdrp->a_size = nentries > acls ? acls : nentries;

                /* Copy ACL Entries out to disk block buffer */
                bcopy((caddr_t)aclp, (caddr_t)ahdrp + sizeof(struct aclhdr),
                        ahdrp->a_size * sizeof(struct acl));

                nentries -= ahdrp->a_size;
                aclp += ahdrp->a_size;
                /*
                 * ACL Block Copied Successfully. Set up correct
                 * back-chaining of ACL blocks for recovery purposes
                 */
                if (lbp) {
                        /* set previous blk's next block ptr to current block */                        lahdrp->a_nxtblk = aclblk;
                        /* set current blk's prev. block ptr to last block */
                        ahdrp->a_lstblk = laclblk;
                        bwrite(lbp);
                } else  {
                        /*
                         * This is the first (and possibly only) ACL block
                         * make it's previous block ptr zero, and
                         * set inode ACL block ptr to block number.
                         */
                        ahdrp->a_lstblk = (daddr_t)0;
                        ip->i_aclblk = aclblk;
                }
                lbp = bp;
                lahdrp = ahdrp;
                laclblk = aclblk;
                lbsize = bsize;
        }       /* end "while (nentries)" */

        if (aclblk) {
                /* write out last block (if it exists) */
                ahdrp->a_nxtblk = (daddr_t)0;
                bwrite(bp);
        }
        return (0);

dealloc:
	ip->i_aclcnt = 0;
	ip->i_daclcnt = 0;
	ip->i_aclblk = (daddr_t)0;

        /* release all buffers & deallocate all blocks if an error occurred */
        if (bp) {
                sfs_free(ip, aclblk, bsize);
                brelse(bp);
                aclblk = (daddr_t)0;
                /* if this is the only ACL block, nothing  else to do */
       	}
        if (lbp) {
                if (lahdrp->a_lstblk != (daddr_t)0) {
                        /*
                         * If not the 1st ACL block, setup
                         * to free all remaining blocks.
                         */
                        aclblk = lahdrp->a_lstblk;
                        /* get count of entries successfully stored in blks */
                        nentries = ip->i_aclcnt - nentries - NACLI -
                                        lahdrp->a_size;
                }
                sfs_free(ip, laclblk, lbsize);
                brelse(lbp);
        }

	if ((sfs_vfsp->vfs_flags & SFS_FSINVALID) != 0)
		return (err);

        while (aclblk) {
                ASSERT(nentries > 0);
                bsize = fragroundup(fsp, (nentries * sizeof(struct acl)) +
                                        sizeof(struct aclhdr));
                if (bsize > fsp->fs_bsize)
                        bsize = fsp->fs_bsize;
                bp = pbread(ip->i_dev, NSPF(fsp) * aclblk, bsize);
		if (bp->b_flags & B_ERROR) {
                        sfs_free(ip, aclblk, bsize);
                        brelse(bp);
                        return (err);
                }
                ahdrp = (struct aclhdr *)(bp->b_un.b_addr);
                laclblk = ahdrp->a_lstblk;
                nentries -= ahdrp->a_size;
                sfs_free(ip, aclblk, bsize);
                brelse(bp);
		if ((sfs_vfsp->vfs_flags & SFS_FSINVALID) != 0)
			return (err);
                aclblk = laclblk;
        }
        return (err);
}
