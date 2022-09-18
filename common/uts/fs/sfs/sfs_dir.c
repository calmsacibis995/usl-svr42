/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/sfs/sfs_dir.c	1.42.3.10"
#ident	"$Header: $"

/*
 * Directory manipulation routines.
 */

#include <acc/dac/acl.h>
#include <acc/mac/covert.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/dnlc.h>
#include <fs/fbuf.h>
#include <fs/fs_subr.h>
#include <fs/mode.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_fsdir.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysinfo.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifdef QUOTA
#include <fs/sfs/sfs_quota.h>
#endif

/* control structures for covert channel limiter */
STATIC ccevent_t cc_alloc_inode = { CC_ALLOC_INODE, CCBITS_ALLOC_INODE };
STATIC ccevent_t cc_spec_diroff = { CC_SPEC_DIROFF, CCBITS_SPEC_DIROFF };
STATIC ccevent_t cc_spec_dirrm = { CC_SPEC_DIRRM, CCBITS_SPEC_DIRRM };

/*
 * A virgin directory.
 */
STATIC struct dirtemplate sfs_mastertemplate = {
	0, 12, 1, ".",
	0, DIRBLKSIZ - 12, 2, ".."
};

#define DOT	0x01
#define DOTDOT	0x02

/*
 * The SFS_DIRSIZ macro gives the minimum record length which will hold
 * a directory entry given the name length.  This requires the amount
 * of space for the name with a terminating null byte, rounded up to a 4
 * byte boundary.
 */
#define SFS_DIRSIZ(len) \
    ((sizeof (struct direct) - (SFS_MAXNAMLEN + 1)) + (((len) + 1 + 3) &~ 3))

STATIC int sfs_dirchk = 0;
STATIC void sfs_dirbad();
STATIC ino_t sfs_dirpref();
/*
 * Look for a given name in a directory.  On successful return, *ipp
 * will point to the (locked) inode.
 */
int
sfs_dirlook(dp, namep, ipp, cr)
	register struct inode *dp;
	register char *namep;
	register struct inode **ipp;
	struct cred *cr;
{
	struct inode *ip;
	struct vnode *vp;
	struct fbuf *fbp = NULL;	/* a buffer of directory entries */
	register struct direct *ep;	/* the current directory entry */
	struct vnode *dnlc_lookup();
	int entryoffsetinblock;		/* offset of ep in addr's buffer */
	int numdirpasses;		/* strategy for directory search */
	off_t endsearch;		/* offset to end directory search */
	int namlen = strlen(namep);	/* length of name */
	off_t offset;
	int error;
	register int i;

	/*
	 * Check accessibility of directory.
	 */
	if ((dp->i_mode & IFMT) != IFDIR)
		return (ENOTDIR);
	if (error = sfs_iaccess(dp, IEXEC, cr))
		return (error);

	/*
	 * Null component name is synonym for directory being searched.
	 */
	if (*namep == '\0') {
		VN_HOLD(ITOV(dp));
		*ipp = dp;
		ILOCK(dp);
		return 0;
	}

	/*
	 * Check the directory name lookup cache.
	 */
	if (vp = dnlc_lookup(ITOV(dp), namep, NOCRED)) {
		ip = VTOI(vp);
		VN_HOLD(vp);
		sfs_ilock(ip);
		*ipp = ip;
		return (0);
	}

	sfs_ilock(dp);
	if (dp->i_diroff > dp->i_size) {
		dp->i_diroff = 0;
	}
	if (dp->i_diroff == 0) {
		offset = 0;
		numdirpasses = 1;
	} else {
		offset = dp->i_diroff;
		entryoffsetinblock = blkoff(dp->i_fs, offset);
		if (entryoffsetinblock != 0) {
			error = sfs_blkatoff(dp, offset, (char **)0, &fbp);
			if (error) {
				goto bad;
			}
		}
		numdirpasses = 2;
	}
	endsearch = roundup(dp->i_size, DIRBLKSIZ);

searchloop:
	while (offset < endsearch) {
		/*
		 * If offset is on a block boundary,
		 * read the next directory block.
		 * Release previous if it exists.
		 */
		if (blkoff(dp->i_fs, offset) == 0) {
			if (fbp != NULL)
				fbrelse(fbp, S_OTHER);
			error = sfs_blkatoff(dp, offset, (char **)0, &fbp);
			if (error) {
				goto bad;
			}
			entryoffsetinblock = 0;
		}

		/*
		 * Get pointer to next entry.
		 * Full validation checks are slow, so we only check
		 * enough to insure forward progress through the
		 * directory. Complete checks can be run by patching
		 * "sfs_dirchk" to be true.
		 */
		ep = (struct direct *)(fbp->fb_addr + entryoffsetinblock);
		if (ep->d_reclen == 0 ||
		    sfs_dirchk && sfs_dirmangled(dp, ep, entryoffsetinblock, offset) ) {
			i = DIRBLKSIZ - (entryoffsetinblock & (DIRBLKSIZ - 1));
			offset += i;
			entryoffsetinblock += i;
			continue;
		}

		/*
		 * Check for a name match.
		 * We must get the target inode before unlocking
		 * the directory to insure that the inode will not be removed
		 * before we get it.  We prevent deadlock by always fetching
		 * inodes from the root, moving down the directory tree. Thus
		 * when following backward pointers ".." we must unlock the
		 * parent directory before getting the requested directory.
		 * There is a potential race condition here if both the current
		 * and parent directories are removed before the `sfs_iget'
		 * for the inode associated with ".." returns.  We hope that
		 * this occurs infrequently since we can't avoid this race
		 * condition without implementing a sophisticated deadlock
		 * detection algorithm. Note also that this simple deadlock
		 * detection scheme will not work if the file system has any
		 * hard links other than ".." that point backwards in the
		 * directory structure.
		 * See comments at head of file about deadlocks.
		 */
		if (ep->d_ino && ep->d_namlen == namlen &&
		    *namep == *ep->d_name &&	/* fast chk 1st chr */
		    bcmp(namep, ep->d_name, (int)ep->d_namlen) == 0) {
			u_long ep_ino;

			/*
			 * We have to release the fbp early ehere to avoid
			 * a possible deadlock situation where we have the
			 * fbp and want the directory inode and someone doing
			 * a sfs_direnter has the directory inode and wants the
			 * fbp.  XXX - is this still needed?
			 */
			ep_ino = ep->d_ino;
			fbrelse(fbp, S_OTHER);
			fbp = NULL;
			dp->i_diroff = offset;
			if (MAC_ACCESS(MACEQUAL, dp->i_dirofflid, cr->cr_lid)) {
				dp->i_dirofflid = cr->cr_lid;
				cc_limiter(&cc_spec_diroff, cr);
			}
			/* are we looking for parent directory? */
			if (namlen == 2 && namep[0] == '.' && namep[1] == '.') {
				sfs_iunlock(dp);	/* race to get inode */
				if (error = sfs_iget((ITOV(dp))->v_vfsp,
				    dp->i_fs, ep_ino, ipp, cr))
					goto bad2;
			/* Not looking for "..". Looking for "."? */
			} else if (dp->i_number == ep_ino) {
				VN_HOLD(ITOV(dp));	/* want ourself, "." */
				*ipp = dp;
			/* Not looking for ".." or ".". Just get inode. */
			} else {
				error = sfs_iget((ITOV(dp))->v_vfsp, dp->i_fs,
					ep_ino, ipp, cr);
				sfs_iunlock(dp);
				if (error) {
					goto bad2;
				}
			}
			ip = *ipp;
			dnlc_enter(ITOV(dp), namep, ITOV(ip), NOCRED);
			return (0);
		}
		offset += ep->d_reclen;
		entryoffsetinblock += ep->d_reclen;
	}
	/*
	 * If we started in the middle of the directory and failed
	 * to find our target, we must check the beginning as well.
	 */
	if (numdirpasses == 2) {
		numdirpasses--;
		offset = 0;
		endsearch = dp->i_diroff;
		goto searchloop;
	}
	error = ENOENT;
bad:
	sfs_iunlock(dp);
bad2:
	if (fbp)
		fbrelse(fbp, S_OTHER);
	return (error);
}

/*
 * If "dircheckforname" fails to find an entry with the given name, this
 * structure holds state for "sfs_direnter" as to where there is space to put
 * an entry with that name.
 * If "dircheckforname" finds an entry with the given name, this structure
 * holds state for "dirrename" and "sfs_dirremove" as to where the entry is.
 * "status" indicates what "dircheckforname" found:
 *	NONE		name not found, large enough free slot not found,
 *			can't make large enough free slot by compacting entries
 *	COMPACT		name not found, large enough free slot not found,
 *			can make large enough free slot by compacting entries
 *	FOUND		name not found, large enough free slot found
 *	EXIST		name found
 * If "dircheckforname" fails due to an error, this structure is not filled in.
 *
 * After "dircheckforname" succeeds the values are:
 *	status	offset		size		fbp, ep
 *	------	------		----		-------
 *	NONE	end of dir	needed		not valid
 *	COMPACT	start of area	of area		not valid
 *	FOUND	start of entry	of ent		not valid
 *	EXIST	start if entry	of prev ent	valid
 *
 * "endoff" is set to 0 if the an entry with the given name is found, or if no
 * free slot could be found or made; this means that the directory should not
 * be truncated.  If the entry was found, the search terminates so
 * "dircheckforname" didn't find out where the last valid entry in the
 * directory was, so it doesn't know where to cut the directory off; if no free
 * slot could be found or made, the directory has to be extended to make room
 * for the new entry, so there's nothing to cut off.
 * Otherwise, "endoff" is set to the larger of the offset of the last
 * non-empty entry in the directory, or the offset at which the new entry will
 * be placed, whichever is larger.  This is used by "diraddentry"; if a new
 * entry is to be added to the directory, any complete directory blocks at the
 * end of the directory that contain no non-empty entries are lopped off the
 * end, thus shrinking the directory dynamically.
 *
 * On success, "sfs_dirprepareentry" makes "fbp" and "ep" valid.
 */
struct slot {
	enum	{NONE, COMPACT, FOUND, EXIST} status;
	off_t	offset;		/* offset of area with free space */
	int	size;		/* size of area at slotoffset */
	struct	fbuf *fbp;	/* dir buf where slot is */
	struct direct *ep;	/* pointer to slot */
	off_t	endoff;		/* last useful location found in search */
};

/*
 * Write a new directory entry.
 * The directory must not have been removed and must be writable.
 * We distinguish five operations that build a new entry:  creating a file
 * (DE_CREATE), making a directory (DE_MKDIR), making a Multi-Level directory
 * (DE_MKMLD), renaming (DE_RENAME) or linking (DE_LINK).  There are five
 * possible cases to consider:
 *
 *	Name
 *	found	op			action
 *	-----	---------------------	--------------------------------------
 *	no	DE_CREATE, DE_MKDIR,	create file according to vap and enter
 *		or DE_MKMLD
 *	no	DE_LINK or DE_RENAME	enter the file sip
 *	yes	DE_CREATE, DE_MKDIR,	error EEXIST *ipp = found file
 *		or DE_MKMLD
 *	yes	DE_LINK			error EEXIST
 *	yes	DE_RENAME		remove existing file, enter new file
 */
int
sfs_direnter(tdp, namep, op, sdp, sip, vap, ipp, cr)
	register struct inode *tdp;	/* target directory to make entry in */
	register char *namep;		/* name of entry */
	enum de_op op;			/* entry operation */
	register struct inode *sdp;	/* source inode parent if rename */
	struct inode *sip;		/* source inode if link/rename */
	struct vattr *vap;		/* attributes if new inode needed */
	struct inode **ipp;		/* return entered inode (locked) here */
	struct cred *cr;		/* user credentials */
{
	struct inode *tip;		/* inode of (existing) target file */
	struct slot slot;		/* slot info to pass around */
	register int namlen;		/* length of name */
	register int err;		/* error number */
	register char *s;

	ASSERT(!((UFSIP(tdp)) && (op == DE_MKMLD)));

	/* don't allow '/' characters in pathname component */
	for (s=namep,namlen=0; *s; s++,namlen++)
		if (*s == '/')
			return(EACCES);
	ASSERT(namlen != 0);
	/*
	 * If name is "." or ".." then if this is a create look it up
	 * and return EEXIST.  Rename or link TO "." or ".." is forbidden.
	 */
	if (namep[0] == '.' &&
	    (namlen == 1 || (namlen == 2 && namep[1] == '.')) ) {
		if (op == DE_RENAME) {
			return (EINVAL);	/* *SIGH* should be ENOTEMPTY */
		}
		if (ipp) {
			if (err = sfs_dirlook(tdp, namep, ipp, cr))
				return (err);
		}
		return (EEXIST);
	}
	slot.status = NONE;
	slot.fbp = NULL;

	/*
	 * For mkdir and mkmld, ensure that we won't be exceeding the maximum 
	 * link count of the parent directory.
	 */
	if ((op == DE_MKDIR || op == DE_MKMLD)
	&&  tdp->i_nlink >= MAXLINK)
		return (EMLINK);

	/*
	 * For link and rename lock the source entry and check the link count
	 * to see if it has been removed while it was unlocked.  If not, we
	 * increment the link count and force the inode to disk to make sure
	 * that it is there before any directory entry that points to it.
	 */
	if (op == DE_LINK || op == DE_RENAME) {
		sfs_ilock(sip);
		if (sip->i_nlink == 0) {
			sfs_iunlock(sip);
			return (ENOENT);
		}
		if (sip->i_nlink >= MAXLINK) {
			sfs_iunlock(sip);
			return (EMLINK);
		}
		sip->i_nlink++;
		sip->i_flag |= ICHG;
		sfs_iupdat(sip, IUP_SYNC);
		sfs_iunlock(sip);
	}
	sfs_ilock(tdp);
	/*
	 * If target directory has not been removed, then we can consider
	 * allowing file to be created.
	 */
	if (tdp->i_nlink == 0) {
		err = ENOENT;
		goto out;
	}
	/*
	 * Check accessibility of directory.
	 */
	if ((tdp->i_mode & IFMT) != IFDIR) {
		err = ENOTDIR;
		goto out;
	}
	/*
	 * Execute access is required to search the directory.
	 */
	if (err = sfs_iaccess(tdp, IEXEC, cr))
		goto out;
	/*
	 * If this is a rename of a directory and the parent is
	 * different (".." must be changed), then the source
	 * directory must not be in the directory hierarchy
	 * above the target, as this would orphan everything
	 * below the source directory.  Also the user must have
	 * write permission in the source so as to be able to
	 * change "..".
	 */
	if (op == DE_RENAME && (sip->i_mode & IFMT) == IFDIR && sdp != tdp
	  && (((err = sfs_iaccess(sip, IWRITE, cr)) != 0)
	   || ((err = sfs_dircheckpath(sip, tdp, cr)) != 0)))
			goto out;
	/*
	 * Search for the entry.
	 */
	if (err = sfs_dircheckforname(tdp, namep, namlen, &slot, &tip, cr))
		goto out;

	/*
	 * MAC write checks are necessary in sfs dependent code
	 * because the time between lookup and VOP_CREATE at
	 * independent level is rather long.  The vnode is released
	 * after lookup, allowing the file to be created or
	 * removed prior to getting to VOP_CREATE. The MAC check
	 * at fs independent level is, therefore, not sufficient.
	 */
	if (tip) {
		struct vnode *vp = ITOV(tip);

		switch (op) {

		case DE_MKDIR:
		case DE_MKMLD:
			if (vp->v_type != VCHR
			&&  vp->v_type != VBLK
			&&  (err = MAC_VACCESS(vp, VWRITE, cr))) {
				sfs_iput(tip);
				goto out;
			}
			/* fall thru */
		case DE_CREATE:	/* MAC check handled in sfs_create() */
			if (ipp) {
				*ipp = tip;
				err = EEXIST;
			} else
				sfs_iput(tip);
			break;

		case DE_RENAME:
			err = sfs_dirrename(sdp, sip, tdp, namep,
			    tip, &slot, cr);
			sfs_iput(tip);
			break;

		case DE_LINK:
			/*
			 * Can't link to an existing file.
			 */
			sfs_iput(tip);
			err = EEXIST;
			break;
		}
	} else {
		if (err = sfs_iaccess(tdp, IWRITE, cr))
			goto out;
		if (op == DE_CREATE || op == DE_MKDIR || op == DE_MKMLD) {
			/*
			 * Ideally, check MAC write permission in directory
			 * only if object existed at fs independent
			 * level prior to calling dependent create call.
			 * If object did not exist at independent level, then
			 * the MAC write check on the directory has already
			 * been done at fs independent level.  Same comments
			 * apply to fs range checks.
			 *
			 * Note that applying the MAC checks
			 * after the DAC check is not a problem
			 * in this case.
			 */
			ASSERT(vap != NULL);
			if (vap->va_type != VLNK) {/* not through vn_create() */
				if (err = MAC_VACCESS(ITOV(tdp), VWRITE, cr))
					goto out;
				/*
				 * Level of object is that of the calling
				 * process.  Make sure that this level is
				 * within the fs range.
				 * The MAC equality checks are added for
				 * performance, i.e., if the level is that
				 * of the floor or ceiling,
				 * there is no need to do domination checks.
				 */
				if (MAC_ACCESS(MACEQUAL,
					ITOV(tdp)->v_vfsp->vfs_macfloor,
					cr->cr_lid)
				&&  MAC_ACCESS(MACEQUAL,
					ITOV(tdp)->v_vfsp->vfs_macceiling,
					cr->cr_lid)
				&&  (MAC_ACCESS(MACDOM,
					ITOV(tdp)->v_vfsp->vfs_macceiling,
					cr->cr_lid)
				  || MAC_ACCESS(MACDOM,
					cr->cr_lid,
					ITOV(tdp)->v_vfsp->vfs_macfloor))
				&&  pm_denied(cr, P_FSYSRANGE)) {
					err = EACCES;
					goto out;
				}
			}
			/*
			 * Make new inode and directory entry as required.
			 */
			if (err=sfs_dirmakeinode(tdp,&sip,vap, op, cr))
				goto out;
		}
		if (err = sfs_diraddentry(tdp, namep, namlen, &slot, sip, 
					sdp, cr, op)) {
			if (op == DE_CREATE ||op == DE_MKDIR ||op == DE_MKMLD) {
				/*
				 * Unmake the inode we just made.
				 */
				if ((sip->i_mode & IFMT) == IFDIR)
					tdp->i_nlink--;
				sip->i_nlink = 0;
				sip->i_flag |= ICHG;
				sfs_irele(sip);
				sip = NULL;
			}
		} else if (ipp) {
			sfs_ilock(sip);
			*ipp = sip;
		} else if (op == DE_CREATE || op == DE_MKDIR ||op == DE_MKMLD) {
			sfs_irele(sip);
		}
	}

out:
	if (slot.fbp)
		fbrelse(slot.fbp, S_OTHER);
	if (err && (op == DE_LINK || op == DE_RENAME)) {
		/*
		 * Undo bumped link count.
		 */
		sip->i_nlink--;
		sip->i_flag |= ICHG;
	}
	sfs_iunlock(tdp);

	return (err);
}

/*
 * Check for the existence of a name in a directory, or else of an empty
 * slot in which an entry may be made.  If the requested name is found,
 * then on return *ipp points at the (locked) inode and *offp contains
 * its offset in the directory.  If the name is not found, then *ipp
 * will be NULL and *slotp will contain information about a directory slot in
 * which an entry may be made (either an empty slot, or the first position
 * past the end of the directory).
 * The target directory inode (tdp) is supplied locked.
 *
 * This may not be used on "." or "..", but aliases of "." are ok.
 */
STATIC int
sfs_dircheckforname(tdp, namep, namlen, slotp, ipp, cr)
	register struct inode *tdp;	/* inode of directory being checked */
	char *namep;			/* name we're checking for */
	register int namlen;		/* length of name */
	register struct slot *slotp;	/* slot structure */
	struct inode **ipp;		/* return inode if we find one */
	struct cred *cr;		/* user credentials */
{
	int dirsize;			/* size of the directory */
	struct fbuf *fbp;		/* pointer to directory block */
	register int entryoffsetinblk;	/* offset of ep in fbp's buffer */
	int slotfreespace;		/* free space in block */
	register struct direct *ep;	/* directory entry */
	register off_t offset;		/* offset in the directory */
	register off_t last_offset;	/* last offset */
	off_t enduseful;		/* pointer past last used dir slot */
	int i;				/* length of mangled entry */
	int needed;
	int err;

	fbp = NULL;
	entryoffsetinblk = 0;
	needed = SFS_DIRSIZ(namlen);
	/*
	 * No point in using i_diroff since we must search whole directory
	 */
	dirsize = roundup(tdp->i_size, DIRBLKSIZ);
	enduseful = 0;
	offset = last_offset = 0;
	while (offset < dirsize) {
		/*
		 * If offset is on a block boundary,
		 * read the next directory block.
		 * Release previous if it exists.
		 */
		if (blkoff(tdp->i_fs, offset) == 0) {
			if (fbp != NULL)
				fbrelse(fbp, S_OTHER);
			err = sfs_blkatoff(tdp, offset, (char **)0, &fbp);
			if (err) {
				return (err);
			}
			entryoffsetinblk = 0;
		}
		/*
		 * If still looking for a slot, and at a DIRBLKSIZ
		 * boundary, have to start looking for free space
		 * again.
		 */
		if (slotp->status == NONE &&
		    (entryoffsetinblk&(DIRBLKSIZ-1)) == 0) {
			slotp->offset = -1;
			slotfreespace = 0;
		}
		/*
		 * Get pointer to next entry.
		 * Since we are going to do some entry manipulation
		 * we call sfs_dirmangled to do more thorough checks.
		 */
		ep = (struct direct *)(fbp->fb_addr + entryoffsetinblk);
		if (ep->d_reclen == 0 ||
		    sfs_dirmangled(tdp, ep, entryoffsetinblk, offset) ) {
			i = DIRBLKSIZ - (entryoffsetinblk & (DIRBLKSIZ - 1));
			offset += i;
			entryoffsetinblk += i;
			continue;
		}
		/*
		 * If an appropriate sized slot has not yet been found,
		 * check to see if one is available. Also accumulate space
		 * in the current block so that we can determine if
		 * compaction is viable.
		 */
		if (slotp->status != FOUND) {
			int size = ep->d_reclen;

			if (ep->d_ino != 0)
				size -= SFS_DIRSIZ(ep->d_namlen);
			if (size > 0) {
				if (size >= needed) {
					slotp->status = FOUND;
					slotp->offset = offset;
					slotp->size = ep->d_reclen;
				} else if (slotp->status == NONE) {
					slotfreespace += size;
					if (slotp->offset == -1)
						slotp->offset = offset;
					if (slotfreespace >= needed) {
						slotp->status = COMPACT;
						slotp->size =
						    offset + ep->d_reclen -
						    slotp->offset;
					}
				}
			}
		}
		/*
		 * Check for a name match.
		 */
		if (ep->d_ino && ep->d_namlen == namlen &&
		    *namep == *ep->d_name &&	/* fast chk 1st char */
		    bcmp(namep, ep->d_name, namlen) == 0) {
			tdp->i_diroff = offset;
			if (tdp->i_number == ep->d_ino) {
				*ipp = tdp;	/* we want ourself, ie "." */
				VN_HOLD(ITOV(tdp));
			} else {
				err = sfs_iget((ITOV(tdp))->v_vfsp, tdp->i_fs,
				    ep->d_ino, ipp, cr);
				if (err) {
					fbrelse(fbp, S_OTHER);
					return (err);
				}
			}
			slotp->status = EXIST;
			slotp->offset = offset;
			slotp->size = offset - last_offset;
			slotp->fbp = fbp;
			slotp->ep = ep;
			slotp->endoff = 0;
			return (0);
		}
		last_offset = offset;
		offset += ep->d_reclen;
		entryoffsetinblk += ep->d_reclen;
		if (ep->d_ino)
			enduseful = offset;
	}
	if (fbp) {
		fbrelse(fbp, S_OTHER);
	}
	if (slotp->status == NONE) {
		/*
		 * We didn't find a slot; the new directory entry should be put
		 * at the end of the directory.  Return an indication of where
		 * this is, and set "endoff" to zero; since we're going to have
		 * to extend the directory, we're certainly not going to
		 * trucate it.
		 */
		slotp->offset = dirsize;
		slotp->size = DIRBLKSIZ;
		slotp->endoff = 0;
	} else {
		/*
		 * We found a slot, and will return an indication of where that
		 * slot is, as any new directory entry will be put there.
		 * Since that slot will become a useful entry, if the last
		 * useful entry we found was before this one, update the offset
		 * of the last useful entry.
		 */
		if (enduseful < slotp->offset + slotp->size)
			enduseful = slotp->offset + slotp->size;
		slotp->endoff = roundup(enduseful, DIRBLKSIZ);
	}
	*ipp = (struct inode *)NULL;
	return (0);
}

/*
 * Rename the entry in the directory tdp so that it points to
 * sip instead of tip.
 */
STATIC int
sfs_dirrename(sdp, sip, tdp, namep, tip, slotp, cr)
	register struct inode *sdp;	/* parent directory of source */
	register struct inode *sip;	/* source inode */
	register struct inode *tdp;	/* parent directory of target */
	char *namep;			/* entry we are trying to change */
	struct inode *tip;		/* locked target inode */
	struct slot *slotp;		/* slot for entry */
	struct cred *cr;		/* credentials */
{
	int error= 0;
	int doingdirectory;
	int dotflag;
	
	/*
	 * Check that everything is on the same filesystem.
	 */
	if (((ITOV(tip))->v_vfsp != (ITOV(tdp))->v_vfsp) ||
	    ((ITOV(tip))->v_vfsp != (ITOV(sip))->v_vfsp))
		return (EXDEV);		/* XXX archaic */
	/*
	 * Short circuit rename of something to itself.
	 */
	if (sip->i_number == tip->i_number)
		return (ESAME);		/* special KLUDGE error code */
	/*
	 * Must have write permission to rewrite target entry.
	 */
	if (error = sfs_iaccess(tdp, IWRITE, cr))
		return (error);
	/*
	 * If the parent directory is "sticky", then the user must own
	 * either the parent directory or the destination of the rename,
	 * or else must have permission to write the destination.
	 * Otherwise the destination may not be changed (except with
	 * privilege).  This implements append-only directories.
	 */
	if ((tdp->i_mode & ISVTX) && cr->cr_uid != tdp->i_uid
	    && cr->cr_uid != tip->i_uid && pm_denied(cr, P_OWNER)
	    && (error = sfs_iaccess(tip, IWRITE, cr)))
		return (error);

	/*
	 * Ensure source and target are compatible (both directories
	 * or both not directories).  If target is a directory it must
	 * be empty and have no links to it; in addition it must not
	 * be a mount point.
	 */
	doingdirectory = ((sip->i_mode & IFMT) == IFDIR);
	if ((tip->i_mode & IFMT) == IFDIR) {
		if (!doingdirectory)
			return (EISDIR);
		if (ITOV(tip)->v_vfsmountedhere)
			return (EBUSY);
		if (!sfs_dirempty(tip, tdp->i_number, cr, &dotflag) || tip->i_nlink > 2)
			return (EEXIST);	/* SIGH should be ENOTEMPTY */
	} else if (doingdirectory)
		return (ENOTDIR);

	/*
	 * Rewrite the inode pointer for target name entry
	 * from the target inode (ip) to the source inode (sip).
	 * This prevents the target entry from disappearing
	 * during a crash. Mark the directory inode to reflect the changes.
	 */
	dnlc_remove(ITOV(tdp), namep);
	slotp->ep->d_ino = sip->i_number;
	dnlc_enter(ITOV(tdp), namep, ITOV(sip), NOCRED);
	error = fbwrite(slotp->fbp);
	slotp->fbp = NULL;
	if (error)
		return (error);
	tdp->i_flag |= IUPD|ICHG;
	/*
	 * Decrement the link count of the target inode.
	 * Fix the ".." entry in sip to point to dp.
	 * This is done after the new entry is on the disk.
	 */
	tip->i_nlink--;
	tip->i_flag |= ICHG;
	if (doingdirectory) {
		/*
		 * Decrement target link count once more if it was a directory.
		 */
		if (--tip->i_nlink != 0) {
			cmn_err(CE_WARN, "sfs_direnter: target directory link count");
			sfs_fsinvalid(ITOV(tip)->v_vfsp);
			return (EIO);
		}
		error = sfs_itrunc(tip, (u_long)0, cr);
		if (error)
			return (error);
		/*
		 * Renaming a directory with the parent different
		 * requires that ".." be rewritten.  The window is
		 * still there for ".." to be inconsistent, but this
		 * is unavoidable, and a lot shorter than when it was
		 * done in a user process.  We decrement the link
		 * count in the new parent as appropriate to reflect
		 * the just-removed target.  If the parent is the
		 * same, this is appropriate since the original
		 * directory is going away.  If the new parent is
		 * different, dirfixdotdot() will bump the link count
		 * back.
		 */
		tdp->i_nlink--;
		tdp->i_flag |= ICHG;
		if (sdp != tdp)
			error = sfs_dirfixdotdot(sip, sdp, tdp);
	}
	return (error);
}

/*
 * Fix the ".." entry of the child directory so that it points
 * to the new parent directory instead of the old one.  Routine
 * assumes that dp is a directory and that all the inodes are on
 * the same file system.
 */
STATIC int
sfs_dirfixdotdot(dp, opdp, npdp)
	register struct inode *dp;	/* child directory */
	register struct inode *opdp;	/* old parent directory */
	register struct inode *npdp;	/* new parent directory */
{
	struct fbuf *fbp;
	struct dirtemplate *dirp;
	int error;

	sfs_ilock(dp);
	/*
	 * Check whether this is an ex-directory.
	 */
	if (dp->i_nlink == 0 || dp->i_size < sizeof (struct dirtemplate)) {
		sfs_iunlock(dp);
		return (0);
	}
	error = sfs_blkatoff(dp, (off_t)0, (char **)&dirp, &fbp);
	if (error)
		goto bad;
	if (dirp->dotdot_ino == npdp->i_number)	/* Just a no-op. */
		goto bad;
	if (dirp->dotdot_namlen != 2 ||
	    dirp->dotdot_name[0] != '.' ||
	    dirp->dotdot_name[1] != '.') {	/* Sanity check. */
		sfs_dirbad(dp, "mangled .. entry", (off_t)0);
		error = ENOTDIR;
		goto bad;
	}

	/*
	 * Increment the link count in the new parent inode and force it out.
	 */
	npdp->i_nlink++;
	npdp->i_flag |= ICHG;
	sfs_iupdat(npdp, IUP_SYNC);

	/*
	 * Rewrite the child ".." entry and force it out.
	 */
	dnlc_remove(ITOV(dp), "..");
	dirp->dotdot_ino = npdp->i_number;
	dnlc_enter(ITOV(dp), "..", ITOV(npdp), NOCRED);
	error = fbwrite(fbp);
	fbp = NULL;
	if (error)
		goto bad;
	sfs_iunlock(dp);

	/*
	 * Decrement the link count of the old parent inode and force
	 * it out.  If opdp is NULL, then this is a new directory link;
	 * it has no parent, so we need not do anything.
	 */
	if (opdp != NULL) {
		sfs_ilock(opdp);
		if (opdp->i_nlink != 0) {
			opdp->i_nlink--;
			opdp->i_flag |= ICHG;
			sfs_iupdat(opdp, IUP_SYNC);
		}
		sfs_iunlock(opdp);
	}
	return (0);

bad:
	if (fbp)
		fbrelse(fbp, S_OTHER);
	sfs_iunlock(dp);
	return (error);
}

/*
 * Enter the file sip in the directory tdp with name namep.
 */
STATIC int
sfs_diraddentry(tdp, namep, namlen, slotp, sip, sdp, cr, op)
	struct inode *tdp;
	char *namep;
	int namlen;
	struct slot *slotp;
	struct inode *sip;
	struct inode *sdp;
	struct cred *cr;
	enum de_op op;
{
	int error;

	/*
	 * Prepare a new entry.  If the caller has not supplied an
	 * existing inode, make a new one.
	 */
	error = sfs_dirprepareentry(tdp, slotp, cr);
	if (error)
		return (error);
	/*
	 * Check inode to be linked to see if it is in the
	 * same filesystem.
	 */
	if ((ITOV(tdp))->v_vfsp != (ITOV(sip))->v_vfsp) {
		error = EXDEV;
		goto bad;
	}
	if ((sip->i_mode & IFMT) == IFDIR && op == DE_RENAME) {
		error = sfs_dirfixdotdot(sip, sdp, tdp);
		if (error)
			goto bad;
	}

	/*
	 * Fill in entry data.
	 */
	slotp->ep->d_namlen = namlen;
	(void) strncpy(slotp->ep->d_name, namep, (size_t)((namlen + 4) & ~3));
	slotp->ep->d_ino = sip->i_number;
	dnlc_enter(ITOV(tdp), namep, ITOV(sip), NOCRED);

	/*
	 * Write out the directory entry.
	 */
	if (op == DE_MKDIR || op == DE_MKMLD)
		error = fbwrite(slotp->fbp);
	else {
		fbrelse(slotp->fbp, S_WRITE);
		error = 0;
	}
	slotp->fbp = NULL;
	if (error)
		return (error);		/* XXX - already fixed dotdot? */

	/*
	 * Mark the directory inode to reflect the changes.
	 * Truncate the directory to chop off blocks of empty entries.
	 */
	tdp->i_flag |= IUPD|ICHG;
	tdp->i_diroff = 0;
	if (slotp->endoff && slotp->endoff < tdp->i_size)
		error = sfs_itrunc(tdp, (u_long)slotp->endoff, cr);
	return (error);

bad:
	/*
	 * Clear out entry prepared by dirprepareent.
	 */
	slotp->ep->d_ino = 0;
	(void) fbwrite(slotp->fbp);	/* XXX - is this right? */
	slotp->fbp = NULL;
	return (error);
}

/*
 * Prepare a directory slot to receive an entry.
 */
STATIC
sfs_dirprepareentry(dp, slotp, cr)
	register struct inode *dp;	/* directory we are working in */
	register struct slot *slotp;	/* available slot info */
	struct cred *cr;		/* user credentials */
{
	register u_short slotfreespace;
	register u_short dsize;
	register int loc;
	register struct direct *ep, *nep;
	char *dirbuf;
	off_t entryend;
	int err;

	/*
	 * If we didn't find a slot, then indicate that the
	 * new slot belongs at the end of the directory.
	 * If we found a slot, then the new entry can be
	 * put at slotp->offset.
	 */
	entryend = slotp->offset + slotp->size;
	if (slotp->status == NONE) {
		if (slotp->offset & (DIRBLKSIZ - 1)) {
			cmn_err(CE_WARN, "sfs_dirprepareentry: new block");
			sfs_fsinvalid(ITOV(dp)->v_vfsp);
			return (EIO);
		}
		ASSERT(DIRBLKSIZ <= dp->i_fs->fs_fsize);
		/*
		 * Allocate the new block.
		 */
		err = BMAPALLOC(dp, (daddr_t)lblkno(dp->i_fs, slotp->offset),
		    (int)(blkoff(dp->i_fs, slotp->offset) + DIRBLKSIZ));
		if (err)
			return (err);
		dp->i_size = entryend;
		dp->i_flag |= IUPD|ICHG;
	} else if (entryend > dp->i_size) {
		/*
		 * Adjust directory size, if needed. This should never
		 * push the size past a new multiple of DIRBLKSIZ.
		 * This is an artifact of the old (4.2BSD) way of initializing
		 * directory sizes to be less than DIRBLKSIZ.
		 */
		dp->i_size = roundup(entryend, DIRBLKSIZ);
		dp->i_flag |= IUPD|ICHG;
	}

	/*
	 * Get the block containing the space for the new directory entry.
	 */
	err = sfs_blkatoff(dp, slotp->offset, (char **)&slotp->ep, &slotp->fbp);
	if (err) {
		return (err);
	}

	ep = slotp->ep;
	switch (slotp->status) {
	case NONE:
		/*
		 * No space in the directory. slotp->offset will be on a
		 * directory block boundary and we will write the new entry
		 * into a fresh block.
		 */
		ep->d_reclen = DIRBLKSIZ;
		break;

	case FOUND:
	case COMPACT:
		/*
		 * Found space for the new entry
		 * in the range slotp->offset to slotp->offset + slotp->size
		 * in the directory.  To use this space, we have to compact
		 * the entries located there, by copying them together towards
		 * the beginning of the block, leaving the free space in
		 * one usable chunk at the end.
		 */
		dirbuf = (char *)ep;
		dsize = SFS_DIRSIZ(ep->d_namlen);
		slotfreespace = ep->d_reclen - dsize;
		for (loc = ep->d_reclen; loc < slotp->size; ) {
			nep = (struct direct *)(dirbuf + loc);
			if (ep->d_ino) {
				/* trim the existing slot */
				ep->d_reclen = dsize;
				ep = (struct direct *)((char *)ep + dsize);
			} else {
				/* overwrite; nothing there; header is ours */
				slotfreespace += dsize;
			}
			dsize = SFS_DIRSIZ(nep->d_namlen);
			slotfreespace += nep->d_reclen - dsize;
			loc += nep->d_reclen;
			bcopy((caddr_t)nep, (caddr_t)ep, (unsigned)dsize);
		}
		/*
		 * Update the pointer fields in the previous entry (if any).
		 * At this point, ep is the last entry in the range
		 * slotp->offset to slotp->offset + slotp->size.
		 * Slotfreespace is the now unallocated space after the
		 * ep entry that resulted from copying entries above.
		 */
		if (ep->d_ino == 0) {
			ep->d_reclen = slotfreespace + dsize;
		} else {
			ep->d_reclen = dsize;
			ep = (struct direct *)((char *)ep + dsize);
			ep->d_reclen = slotfreespace;
		}
		break;

	default:
		cmn_err(CE_WARN, "sfs_dirprepareentry: invalid slot status");
		sfs_fsinvalid(ITOV(dp)->v_vfsp);
		return (EIO);
	}
	slotp->ep = ep;
	return (0);
}

/*
 * Allocate and initialize a new inode that will go into directory tdp.
 */
STATIC int
sfs_dirmakeinode(tdp, ipp, vap, op, cr)
	struct inode *tdp;
	struct inode **ipp;
	register struct vattr *vap;
	enum de_op op;
	struct cred *cr;
{
	struct inode *ip;
	register enum vtype type;
	int imode;			/* mode and format as in inode */
	ino_t ipref;
	int error;
	struct vnode *tvp = ITOV(tdp);
	lid_t nlid;
	int eff_dir = 0;
	int iskipped;

	ASSERT(vap != NULL);
	ASSERT(op == DE_CREATE || op == DE_MKDIR || op == DE_MKMLD);
	ASSERT((vap->va_mask & (AT_TYPE|AT_MODE)) == (AT_TYPE|AT_MODE));

	/*
	 * Allocate a new inode.
	 */
	type = vap->va_type;

	if (type == VLNK)
		nlid = tvp->v_lid;
	else
		nlid = cr->cr_lid;

	if (type == VDIR) {
		ipref = sfs_dirpref(tdp->i_fs);
	} else {
		ipref = tdp->i_number;
	}
	imode = MAKEIMODE(type, vap->va_mode);
	error = sfs_ialloc(tdp, ipref, imode, &ip, cr, &iskipped);
	if (error)
		return (error);
#ifdef QUOTA
	ASSERT(ip->i_dquot == NULL);
#endif
	ip->i_mode = imode;
	ip->i_ic.ic_eftflag = (ulong)EFT_MAGIC;
	if (type == VBLK || type == VCHR || type == VXNAM) {
		(ITOV(ip))->v_rdev = ip->i_rdev = vap->va_rdev;
	}
	if (!UFSIP(tdp)) {
		/*
		 * Covert channel check.  If inode was previously used at a
		 * different level, record as potential covert channel
		 */
		if (ip->i_lid != cr->cr_lid) {
			if (iskipped)
				iskipped = find_msb(iskipped);
			cc_alloc_inode.cc_bits = (long)iskipped + 2;
			cc_limiter(&cc_alloc_inode, cr);
		}
		(ITOV(ip))->v_lid = ip->i_lid = nlid; /* MAC */
		(ITOV(ip))->v_macflag = VMAC_SUPPORT;
	}
	(ITOV(ip))->v_type = type;
	if (type == VDIR && (op == DE_MKDIR || op == DE_MKMLD)) {
		ip->i_nlink = 2;      /*anticipating a call to dirmakedirect */
		if (op == DE_MKMLD && !(UFSIP(tdp))) {
			ip->i_sflags |= ISD_MLD; /*indicate MLD*/
			(ITOV(ip))->v_macflag |= VMAC_ISMLD;
		}
	} else {
		ip->i_nlink = 1;
	}
	ip->i_uid = cr->cr_uid;
	/*
         * To determine the group-id of the created file:
	 *  1) If this is a creation of an effective directory by
	 *     virtue of an automatic creation, i.e., process is
	 *     in virtual MLD mode, just take the DAC information
	 *     from the parent directory (MLD).
	 *  2) If the gid is set in the attribute list (non-Sun & pre-4.0
	 *     clients are not likely to set the gid), then use it if
	 *     the process is privileged, belongs to the target group,
	 *     or the group is the same as the parent directory.
	 *  3) If the filesystem was not mounted with the Old-BSD-compatible
	 *     GRPID option, and the directory's set-gid bit is clear,
	 *     then use the process's gid.
	 *  4) Otherwise, set the group-id to the gid of the parent directory.
	 */
	if (op == DE_MKDIR
	&&  !UFSIP(tdp)
	&&  !(cr->cr_flags & CR_MLDREAL)
	&&  tdp->i_sflags & ISD_MLD) {
		/*
		 * If there are stored ACL entries on parent dir,
		 * allocate space to store the ACL entries,
		 * store ACL entries on inode, and free the space.
		 */
		if (tdp->i_aclcnt && dac_installed) {
			struct acl *tmpaclp;	/* tmp storage for ACL */
			int tmpsize;		/* size of tmp storage */

			tmpsize = tdp->i_aclcnt * sizeof(struct acl);
			tmpaclp = (struct acl *)kmem_alloc(tmpsize, KM_SLEEP);
		  	if ((error = sfs_aclget(tdp, tmpaclp, 0))
		  	||  (error = sfs_aclstore(ip, tmpaclp,
				tdp->i_aclcnt, tdp->i_daclcnt, cr))) {
				kmem_free(tmpaclp, tmpsize);

				/* Throw away inode just allocated */
				ip->i_nlink = 0;
				ip->i_flag |= ICHG;
				sfs_iput(ip);
				return (error);
			}
			kmem_free(tmpaclp, tmpsize);
		} else {
			ip->i_aclcnt = 0;
			ip->i_daclcnt = 0;
			ip->i_aclblk = (daddr_t)0;
		}

		ip->i_uid = tdp->i_uid;
		ip->i_gid = tdp->i_gid;
		ip->i_mode = tdp->i_mode;

		eff_dir = 1;

	} else if ((vap->va_mask & AT_GID) &&
	    ((vap->va_gid == tdp->i_gid) || groupmember(vap->va_gid, cr) ||
	    (!pm_denied(cr, P_OWNER)))) {
		/*
		 * XXX - is this only the case when a 4.0 NFS client, or a
		 * client derived from that code, makes a call over the wire?
		 */
		ip->i_gid = vap->va_gid;
	} else {
		if (tdp->i_mode & ISGID)
			ip->i_gid = tdp->i_gid;
		else
			ip->i_gid = cr->cr_gid;
	}
	/*
	 * If we're creating a directory, and the parent directory has the
	 * set-GID bit set, set it on the new directory.
	 * Otherwise, if the user is neither privileged nor a member of the
	 * file's new group, clear the file's set-GID bit.
	 */
	if (tdp->i_mode & ISGID && type == VDIR)
		ip->i_mode |= ISGID;
	else {
		if ((ip->i_mode & ISGID) && !groupmember((uid_t)ip->i_gid, cr)
		    && pm_denied(cr, P_OWNER))
			ip->i_mode &= ~ISGID;
	}
#ifdef QUOTA
	ip->i_dquot = sfs_getinoquota(ip, cr);
#endif QUOTA

	/* 
	 * If the parent directory has default ACL entries, call
	 * sfs_dirdefacl to apply the defaults to the new file
	 */
	if (!UFSIP(tdp) && !eff_dir) {
		/*
		 * Defaults are not propagated if:
		 *	- the parent has no defaults
		 *	- object is a symbolic link
		 *	- the DAC module is not installed
		 */
		if (tdp->i_daclcnt && type != VLNK && dac_installed) {
		   	if (error = sfs_dirdefacl(tdp, ip, cr)) {
				/* Throw away inode just allocated */
				ip->i_nlink = 0;
				ip->i_flag |= ICHG;
				sfs_iput(ip);
				return (error);
			}
		} else {
			/* otherwise clear all inode ACL info */
			ip->i_aclcnt = 0;
			ip->i_daclcnt = 0;
			ip->i_aclblk = (daddr_t)0;
		}
	}

	ip->i_flag |= IACC|IUPD|ICHG;

	/*
	 * Make sure inode goes to disk before directory data and entries
	 * pointing to it.
	 * Then unlock it, since nothing points to it yet.
	 */
	sfs_iupdat(ip, IUP_SYNC);

	if (op == DE_MKDIR || op == DE_MKMLD) {
		error = sfs_dirmakedirect(ip, tdp, cr);
	}
	if (error) {
		/* Throw away inode we just allocated. */
		ip->i_nlink = 0;
		ip->i_flag |= ICHG;
		sfs_iput(ip);
	} else {
		sfs_iunlock(ip);
		*ipp = ip;
	}
	return (error);
}

/*
 * Add an ACL to a newly created file based on the default
 * ACL of the parent directory.  If the new file is a directory,
 * the default entries are propagated as defaults in addition to
 * the non-defaults that are applied.
 */
STATIC int
sfs_dirdefacl(tdp, ip, cr)
	register struct inode   *tdp;           /* ptr to parent directory inode */
	register struct inode   *ip;            /* ptr to new inode */
	struct cred             *cr;            /* caller's credentials */
{
        register struct acl     *srcaclp;       /* source ACL */
        register struct acl     *tgtaclp;       /* target ACL */
        register long   dentries;               /* # parent default entries */
        register long   i;
        struct vnode    *vp = ITOV(ip);         /* new vnode */
        struct acl      *tmpaclp;               /* parent default ACL buffer*/
        struct acl      *newaclp;               /* new file ACL buffer */
        struct acl      *user_1p = NULL;        /* 1st USER entry */
        struct acl      *user_np = NULL;        /* nth USER entry */
        struct acl      *group_1p = NULL;       /* 1st GROUP entry */
        struct acl      *group_np = NULL;       /* nth GROUP entry */
        struct acl      base_grp = {GROUP_OBJ, (uid_t)0, (ushort)0};
        long            entries;                /* # entries in ACL buffer */
        long            bufsize;                /* entries in temp ACL buffer */
	long            groups;
        long            group_obj = 0;
        long            users;
	mode_t		type = ip->i_mode & IFMT;	/* file type */
        mode_t          ownclass = ip->i_mode & 0700;   /* owner class bits */
        mode_t          grpclass = ip->i_mode & 070;    /* group class bits */
        mode_t          othclass = ip->i_mode & 07;     /* other class bits */
        int             error;
	int		tmpsize;
	
	ASSERT(!UFSIP(tdp));
        ILOCK(tdp);
        dentries = tdp->i_daclcnt;	
        ASSERT(dentries > 0);
	tmpsize = dentries + 1;         /* room for GROUP_OBJ if necessary */
        entries = dentries;             /* entries for new file's ACL */
	tmpaclp = (struct acl *)kmem_alloc(tmpsize * sizeof(struct acl),
                                                KM_SLEEP);
        /* get default ACL from parent directory inode */
        error = sfs_aclget(tdp, tmpaclp, 1);	/* get defaults only */
        IUNLOCK(tdp);
        if (error) {
		kmem_free(tmpaclp, tmpsize * sizeof(struct acl));
                return (error);
        }
        /* propagate defaults if this is a new directory */
        if (vp->v_type == VDIR) {
                bufsize = tmpsize + dentries;
                entries += dentries;
                newaclp = (struct acl *)kmem_alloc(bufsize * sizeof(struct acl),                                                KM_SLEEP);
                tgtaclp = newaclp + dentries + 1;
                bcopy((caddr_t)tmpaclp, (caddr_t)tgtaclp, 
			dentries * sizeof(struct acl));
        } else {
		bufsize = tmpsize;
                newaclp = (struct acl *)kmem_alloc(bufsize * sizeof(struct acl),                        			KM_SLEEP);
                tgtaclp = newaclp + dentries + 1;
        }
        /*
         * scan defaults, looking for base entries,
         * and use them to set permission bits.
         * set defaults to access-related entries (turn off DEFAULT flag)
         */
        srcaclp = tmpaclp;
        for (i = 0; i < dentries; i++, srcaclp++) {
                srcaclp->a_type &= ~ACL_DEFAULT;
                switch (srcaclp->a_type) {
                case USER_OBJ:
                        ownclass &= ((long)srcaclp->a_perm & 07) << 6;
                        entries--;      /* don't store USER_OBJ in ACL */
                        break;
                case USER:
                        if (user_1p == NULL)
                                user_1p = srcaclp;
                        user_np = srcaclp + 1;
                        break;
                case GROUP_OBJ:
                        group_obj++;
                        /*
                         * if additional USER or GROUP entries
                         * we're gonna store GROUP_OBJ in the ACL.
                         */
                        if ((user_1p != NULL) ||
                                ((srcaclp + 1)->a_type == GROUP)) {
                                group_1p = srcaclp;
                                group_np = srcaclp + 1;
                        } else
                                entries--;
                        break;
                case GROUP:
                        if (group_1p == NULL)
                                group_1p = srcaclp;
                        group_np = srcaclp + 1;
                        break;
                case CLASS_OBJ:
                        grpclass &= ((long)srcaclp->a_perm & 07) << 3;
			entries--;	/* don't store CLASS_OBJ in ACL */
			break;
                case OTHER_OBJ:
                        othclass &= (long)srcaclp->a_perm & 07;
                        entries--;      /* don't store OTHER_OBJ in ACL */
                        break;
                }       /* end switch */
        }       /* end for */
        groups = (long)(group_np - group_1p);
        users = (long)(user_np - user_1p);
        if (groups > 0) {
                /* if default had additional groups, copy them first */
                tgtaclp -= groups;
                bcopy((caddr_t)group_1p, (caddr_t)tgtaclp, groups * sizeof(struct acl));
        }
        /*
         * if no default GROUP_OBJ were specified, and
         * there were default additional USER or GROUP entries,
	 * we've gotta build a GROUP_OBJ entry
         */
        if (((groups > 0) || (users > 0)) && (group_obj == 0)) {
                base_grp.a_perm = (ushort)grpclass >> 3;
                tgtaclp--;
                bcopy((caddr_t)&base_grp, (caddr_t)tgtaclp, sizeof(struct acl));
                entries++;
        }
        if (users > 0) {
                /* if default had additional users, copy them next */
                tgtaclp -= users;
                bcopy((caddr_t)user_1p, (caddr_t)tgtaclp, users * sizeof (struct acl));
        }

        if ((vp->v_type == VDIR) && (entries > acl_getmax()) && 
		pm_denied(cr, P_FILESYS))
		error = ENOSPC;
	else {
        	ILOCK(ip);
        	/* now, set the ACL on the disk inode & disk blocks */
        	error = sfs_aclstore(ip, tgtaclp, entries,
                                (vp->v_type == VDIR) ? dentries : 0, cr);
        	/* set the permission bits */
        	ip->i_mode = type | ownclass | grpclass | othclass;
        	IUNLOCK(ip);
	}
	kmem_free(tmpaclp, tmpsize * sizeof(struct acl));
        kmem_free(newaclp, bufsize * sizeof(struct acl));
        return (error);
}


/*
 * Write a prototype directory into the empty inode ip, whose parent is dp.
 */
STATIC int
sfs_dirmakedirect(ip, dp, cr)
	register struct inode *ip;		/* new directory */
	register struct inode *dp;		/* parent directory */
	register struct cred *cr;		/* user credentials */
{
	int error;
	register struct dirtemplate *dirp;
	struct fbuf *fbp;

	/*
	 * Allocate space for the directory we're creating.
	 */
	error = BMAPALLOC(ip, (daddr_t)0, DIRBLKSIZ);
	if (error)
		return (error);
	ASSERT(DIRBLKSIZ <= ip->i_fs->fs_fsize);
	ip->i_size = DIRBLKSIZ;
	ip->i_flag |= IUPD|ICHG;
	/*
	 * Update the tdp link count and write out the change.
	 * This reflects the ".." entry we'll soon write.
	 */
	dp->i_nlink++;
	dp->i_flag |= ICHG;
	sfs_iupdat(dp, IUP_SYNC);
	/*
	 * Initialize directory with "."
	 * and ".." from static template.
	 */
	error = fbread(ITOV(ip), 0, (u_int)ip->i_fs->fs_fsize, S_OTHER, &fbp);
	if (error)
		return (error);
	dirp = (struct dirtemplate *)fbp->fb_addr;
	/*
	 * Now initialize the directory we're creating
	 * with the "." and ".." entries.
	 */
	*dirp = sfs_mastertemplate;			/* structure assignment */
	dirp->dot_ino = ip->i_number;
	dirp->dotdot_ino = dp->i_number;
	error = fbwrite(fbp);
	return (error);
}

/*
 * Delete a directory entry.  If oip is nonzero the entry is checked
 * to make sure it still reflects oip.
 */
int
sfs_dirremove(dp, namep, oip, cdir, op, cr)
	register struct inode *dp;
	char *namep;
	struct inode *oip;
	struct vnode *cdir;
	enum dr_op op;
	struct cred *cr;
{
	register struct direct *ep;
	struct direct *pep;
	struct inode *ip;
	int namlen;
	struct slot slot;
	int error = 0;
	int dotflag;
	
	namlen = strlen(namep);
	if (namlen == 0) {
		cmn_err(CE_WARN, "sfs_dirremove: directory name length zero");
		sfs_fsinvalid(ITOV(dp)->v_vfsp);
		return (EIO);
	}
	/*
	 * return error when removing . and ..
	 */
	if (namep[0] == '.') {
		if (namlen == 1)
			return (EINVAL);
		else if (namlen == 2 && namep[1] == '.')
			{
			return (EEXIST);	/* SIGH should be ENOTEMPTY */
			}
	}

	ip = NULL;
	slot.fbp = NULL;
	sfs_ilock(dp);

	/*
	 * Check accessibility of directory.
	 */
	if ((dp->i_mode & IFMT) != IFDIR) {
		error = ENOTDIR;
		goto out;
	}

	/*
	 * Execute access is required to search the directory.
	 * Access for write is interpreted as allowing
	 * deletion of files in the directory.
	 */
	if (error = sfs_iaccess(dp, IEXEC|IWRITE, cr))
		goto out;

	slot.status = FOUND;	/* don't need to look for empty slot */
	if (error = sfs_dircheckforname(dp, namep, namlen, &slot, &ip, cr))
		goto out;
	if (ip == NULL) {
		error = ENOENT;
		goto out;
	}
	if (oip && oip != ip) {
		error = ENOENT;
		goto out;
	}
	/*
	 * There used to be a check here to make sure you are not removing a
	 * mounted on dir.  This was no longer correct because sfs_iget() does
	 * not cross mount points anymore so the the i_dev fields in the inodes
	 * pointed to by ip and dp will never be different.  There does need
	 * to be a check here though, to eliminate the race between mount and
	 * rmdir (It can also be a race between mount and unlink, if your
	 * kernel allows you to unlink a directory.)
	 */
	if (ITOV(ip)->v_vfsmountedhere != NULL) {
		error = EBUSY;
		goto out;
	}
	/*
	 * If the parent directory is "sticky", then the user must
	 * own the parent directory or the file in it, or else must
	 * have permission to write the file.  Otherwise it may not
	 * be deleted (except by a privileged user).  This implements
	 * append-only directories.
	 */
	if ((dp->i_mode & ISVTX) && cr->cr_uid != dp->i_uid
	    && cr->cr_uid != ip->i_uid && pm_denied(cr, P_OWNER)
	    && (error = sfs_iaccess(ip, IWRITE, cr)))
		goto out;
	if (op == DR_RMDIR) {
		/*
		 * For rmdir(2), some special checks are required.
	 	 * (a) Don't remove any alias of the parent (e.g. ".").
	 	 * (b) Don't remove the current directory.
		 * (c) Make sure the entry is (still) a directory.
		 * (d) Make sure the directory is empty.
	 	 */

		if (dp == ip || ITOV(ip) == cdir)
			error = EINVAL;
		else if ((ip->i_mode & IFMT) != IFDIR)
			error = ENOTDIR;
		else if (!sfs_dirempty(ip, dp->i_number, cr, &dotflag))
		{
			if (MAC_ACCESS(MACEQUAL, ip->i_lid, cr->cr_lid))
				cc_limiter(&cc_spec_dirrm, cr);
			error = EEXIST;	/* SIGH should be ENOTEMPTY */
		}
		if (error)
			goto out;
	} else if (op == DR_REMOVE)  {
		/*
		 * unlink(2) requires a different check:
		 * Allow only a privileged user to unlink a directory.
		 */
		struct vnode *vp = ITOV(ip);
		if (vp->v_type == VDIR && pm_denied(cr, P_FILESYS)) {
			error = EPERM;
			goto out;
		}
	}
	/*
	 * Remove the cache'd entry, if any.
	 */
	dnlc_remove(ITOV(dp), namep);
	/*
	 * If the entry isn't the first in the directory, we must reclaim
	 * the space of the now empty record by adding the record size
	 * to the size of the previous entry.
	 */
	ep = slot.ep;
	if ((slot.offset & (DIRBLKSIZ - 1)) == 0) {
		/*
		 * First entry in block: set d_ino to zero.
		 */
		ep->d_ino = 0;
	} else {
		/*
		 * Collapse new free space into previous entry.
		 */
		pep = (struct direct *)((char *)ep - slot.size);
		pep->d_reclen += ep->d_reclen;
	}
	/*
	 * Clear out the entry's name.
	 */
	bzero(ep->d_name, ep->d_namlen);

	error = fbwrite(slot.fbp);
	slot.fbp = NULL;
	dp->i_flag |= IUPD|ICHG;
	ip->i_flag |= ICHG;
	if (error)
		goto out;
	/*
	 * Now dispose of the inode.
	 */
	if (ip->i_nlink > 0) {
		if (op == DR_RMDIR && (ip->i_mode & IFMT) == IFDIR) {
			/*
			 * decrement by 2 because we're trashing the "."
			 * entry as well as removing the entry in dp.
			 * Clear the inode, but there may be other hard
			 * links so don't free the inode.
			 * Decrement the dp linkcount because we're
			 * trashing the ".." entry.
			 */
			if (dotflag &DOT) {
				ip->i_nlink -=2;
				dnlc_remove(ITOV(ip), ".");
			} else {
				ip->i_nlink--;
			}		
			if (dotflag & DOTDOT) {
				dp->i_nlink--;
				dnlc_remove(ITOV(ip), "..");
			}
			if (ITOV(ip)->v_count > 1 &&
				ip->i_nlink <= 0)
				sfs_rdwri(UIO_WRITE, ip,
				(caddr_t)&sfs_mastertemplate,
				min(sizeof(sfs_mastertemplate),
				ip->i_size), (off_t)0,
				UIO_SYSSPACE, (int *)0,
				cr);
		} else {
			ip->i_nlink--;
		}
	}
	if (ip->i_nlink < 0)
		ip->i_nlink = 0;
out:
	if (ip)
		sfs_iput(ip);
	if (slot.fbp)
		fbrelse(slot.fbp, S_OTHER);
	sfs_iunlock(dp);
	return (error);
}

/*
 * Return buffer with contents of block "offset"
 * from the beginning of directory "ip".  If "res"
 * is non-zero, fill it in with a pointer to the
 * remaining space in the directory.
 */
STATIC int
sfs_blkatoff(ip, offset, res, fbpp)
	struct inode *ip;
	off_t offset;
	char **res;
	struct fbuf **fbpp;
{
	register struct fs *fs;
	struct fbuf *fbp;
	daddr_t lbn;
	u_int bsize;
	int err;

	fs = ip->i_fs;
	lbn = lblkno(fs, offset);
	bsize = blksize(fs, ip, lbn);
	sysinfo.dirblk++;
	err = fbread(ITOV(ip), (long)(offset & fs->fs_bmask), bsize, S_OTHER,
	    &fbp);
	if (err) {
		*fbpp = (struct fbuf *)NULL;
		return (err);
	}
	if (res)
		*res = fbp->fb_addr + blkoff(fs, offset);
	*fbpp = fbp;
	return (0);
}

/*
 * Do consistency checking:
 *	record length must be multiple of 4
 *	entry must fit in rest of its DIRBLKSIZ block
 *	record must be large enough to contain entry
 *	name is not longer than SFS_MAXNAMLEN
 * if sfs_dirchk is on:
 *	name must be as long as advertised, and null terminated
 * NOTE: record length must not be zero (should be checked previously).
 */
STATIC int
sfs_dirmangled(dp, ep, entryoffsetinblock, offset)
	register struct inode *dp;
	register struct direct *ep;
	int entryoffsetinblock;
	off_t offset;
{
	register int i;

	i = DIRBLKSIZ - (entryoffsetinblock & (DIRBLKSIZ - 1));
	if ((ep->d_reclen & 0x3) != 0 || (int)ep->d_reclen > i ||
	    (u_int)ep->d_reclen < SFS_DIRSIZ(ep->d_namlen) ||
	    ep->d_namlen > SFS_MAXNAMLEN ||
	    sfs_dirchk && sfs_dirbadname(ep->d_name, (int)ep->d_namlen)) {
		sfs_dirbad(dp, "mangled entry", offset);
		return (1);
	}
	return (0);
}

STATIC void
sfs_dirbad(ip, how, offset)
	struct inode *ip;
	char *how;
	off_t offset;
{

	cmn_err(CE_NOTE, "%s: bad dir ino %d at offset %d: %s\n",
	    ip->i_fs->fs_fsmnt, ip->i_number, offset, how);
	return;
}

STATIC int
sfs_dirbadname(sp, l)
	register char *sp;
	register int l;
{

	while (l--) {			/* check for nulls */
		if (*sp++ == '\0') {
			return (1);
		}
	}
	return (*sp);			/* check for terminating null */
}

/*
 * Check if a directory is empty or not.
 *
 * Using a struct dirtemplate here is not precisely
 * what we want, but better than using a struct direct.
 *
 * N.B.: does not handle corrupted directories.
 */
STATIC int
sfs_dirempty(ip, parentino, cr, dotflagp)
	register struct inode *ip;
	ino_t parentino;
	struct cred *cr;
	int *dotflagp;
{
	register off_t off;
	struct dirtemplate dbuf;
	register struct direct *dp = (struct direct *)&dbuf;
	int err, count;
#define	MINDIRSIZ (sizeof (struct dirtemplate) / 2)

	*dotflagp = 0;	
	for (off = 0; off < ip->i_size; off += dp->d_reclen) {
		err = sfs_rdwri(UIO_READ, ip, (caddr_t)dp, (int)MINDIRSIZ,
		    off, UIO_SYSSPACE, &count, cr);
		/*
		 * Since we read MINDIRSIZ, residual must
		 * be 0 unless we're at end of file.
		 */
		if (err || count != 0 || dp->d_reclen == 0)
			return (0);
		/* skip empty entries */
		if (dp->d_ino == 0)
			continue;
		/* accept only "." and ".." */
		if (dp->d_namlen > 2)
			return (0);
		if (dp->d_name[0] != '.')
			return (0);
		/*
		 * At this point d_namlen must be 1 or 2.
		 * 1 implies ".", 2 implies ".." if second
		 * char is also "."
		 */
		if (dp->d_namlen == 1) {
			*dotflagp |= DOT;
			continue;
		}	
		/* don't check parentino, link will change it */
		if (dp->d_name[1] == '.') {
			*dotflagp |= DOTDOT;
			continue;
		}	
		return (0);
	}
	return (1);
}

#define	RENAME_IN_PROGRESS	0x01
#define	RENAME_WAITING		0x02

/*
 * Check if source directory is in the path of the target directory.
 * Target is supplied locked, source is unlocked.
 * The target is always relocked before returning.
 */
STATIC int
sfs_dircheckpath(source, target, cr)
	struct inode *source;
	struct inode *target;
	struct cred *cr;
{
	struct fbuf *fbp;
	struct dirtemplate *dirp;
	register struct inode *ip;
	struct inode *tip;
	static char serialize_flag = 0;
	ino_t dotdotino;
	int err = 0;

	/*
	 * If two renames of directories were in progress at once, the partially
	 * completed work of one dircheckpath could be invalidated by the other
	 * rename.  To avoid this, all directory renames in the system are
	 * serialized.
	 */
	while (serialize_flag & RENAME_IN_PROGRESS) {
		serialize_flag |= RENAME_WAITING;
		(void) sleep((caddr_t)&serialize_flag, PINOD);
	}
	serialize_flag = RENAME_IN_PROGRESS;
	ip = target;
	if (ip->i_number == source->i_number) {
		err = EINVAL;
		goto out;
	}
	if (ip->i_number == SFSROOTINO)
		goto out;
	/*
	 * Search back through the directory tree, using the ".." entries.
	 * Fail any attempt to move a directory into an ancestor directory.
	 */
	fbp = NULL;
	for (;;) {
		if (((ip->i_mode & IFMT) != IFDIR) || ip->i_nlink == 0
		    || ip->i_size < sizeof (struct dirtemplate)) {
			sfs_dirbad(ip, "bad size, unlinked or not dir", (off_t)0);
			err = ENOTDIR;
			break;
		}
		if (err = sfs_blkatoff(ip, (off_t)0, (char **)&dirp, &fbp))
			break;
		if (dirp->dotdot_namlen != 2 ||
		    dirp->dotdot_name[0] != '.' ||
		    dirp->dotdot_name[1] != '.') {
			sfs_dirbad(ip, "mangled .. entry", (off_t)0);
			err = ENOTDIR;		/* Sanity check */
			break;
		}
		dotdotino = dirp->dotdot_ino;
		if (dotdotino == source->i_number) {
			err = EINVAL;
			break;
		}
		if (dotdotino == SFSROOTINO)
			break;
		if (fbp) {
			fbrelse(fbp, S_OTHER);
			fbp = NULL;
		}
		if (ip != target)
			sfs_iput(ip);
		else
			sfs_iunlock(ip);
		/*
		 * i_dev and i_fs are still valid after sfs_iput
		 * This is a race to get ".." just like sfs_dirlook.
		 */
		if (err = sfs_iget((ITOV(ip))->v_vfsp, ip->i_fs, dotdotino, 
			&tip, cr)) {
			ip = NULL;
			break;
		}
		ip = tip;
	}
	if (fbp) {
		fbrelse(fbp, S_OTHER);
	}
out:
	/*
	 * Unserialize before relocking target to avoid a race.
	 */
	if (serialize_flag & RENAME_WAITING)
		wakeprocs((caddr_t)&serialize_flag, PRMPT);
	serialize_flag = 0;

	if (ip) {
		if (ip != target) {
			sfs_iput(ip);
			/*
			 * Relock target and make sure it has not gone away
			 * while it was unlocked.
			 */
			sfs_ilock(target);
			if ((err == 0) && (target->i_nlink == 0)) {
				err = ENOENT;
			}
		}
	}
	return (err);
}

/*
 * Find a cylinder to place a directory.
 *
 * The policy implemented by this algorithm is to select from
 * among those cylinder groups with above the average number of
 * free inodes, the one with the smallest number of directories.
 */
STATIC ino_t
sfs_dirpref(fs)
	register struct fs *fs;
{
	register int avgifree;
	register int cg;
	register int minndir;
	int mincg;

	avgifree = fs->fs_cstotal.cs_nifree / fs->fs_ncg;
	minndir = fs->fs_ipg;
	mincg = 0;
	for (cg = 0; cg < fs->fs_ncg; cg++)
		if (fs->fs_cs(fs, cg).cs_ndir < minndir &&
		    fs->fs_cs(fs, cg).cs_nifree >= avgifree) {
			mincg = cg;
			minndir = fs->fs_cs(fs, cg).cs_ndir;
		}
	return ((ino_t)(fs->fs_ipg * mincg));
}

/*
 * find_msb(n).  Returns bit number (numbering from right) of most
 * significant bit in n.  find_msb(0)==-1.
 */
static int
find_msb(n)
	int n;
{
	register unsigned int u;
	register int ret;

	ret = -1;
	u = n;
	while (u) {
		++ret;
		u >>= 1;
	}
	return (ret);
}
