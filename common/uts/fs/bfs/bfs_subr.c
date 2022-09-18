/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:fs/bfs/bfs_subr.c	1.12.3.2"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/bfs/bfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

static int bfs_dotsearch();

/*
 * Called whenever anything wants a reference to a BFS vnode.  Searches the
 * in core linked list of vnodes.  If it isn't found, create a new one, and
 * link it in.  Each BFS vnode is the same except for its v_data field, which
 * contains the offset on the disk of its directory entry.
 */
void
bfs_fillvnode(vpp, data, vfsp)
	struct vnode **vpp;
	caddr_t data;
	struct vfs *vfsp;
{
	extern struct vnodeops bfsvnodeops;
	register struct bfs_core_vnode *cvp;
		/* LINTED vfs_data should be word aligned */
	register struct bsuper *bs = (struct bsuper *)vfsp->vfs_data;
	struct bfs_core_vnode *ncvp;
	register struct vnode *vp;

	cvp = bs->bsup_incore_vlist;

	/*
	 * If the in core vnode list exists, search it.
	 */
	if (cvp) {
		while (cvp->core_vnode->v_data != data &&
		       cvp->core_next != (struct bfs_core_vnode *)0)
				cvp = cvp->core_next;

		if (cvp->core_vnode->v_data == data) {
			VN_HOLD(cvp->core_vnode);	/* Up the ref. count */
			*vpp = cvp->core_vnode;
			return;
		}
	}

	/*
	 * It isn't there.  Create a new vnode and link it to the list.
	 */
	vp = (struct vnode *)kmem_zalloc(sizeof(struct vnode), KM_SLEEP);

	/*
	 * Fill the core vnode entry.
	 */	
	ncvp = (struct bfs_core_vnode *)
	  kmem_alloc(sizeof(struct bfs_core_vnode), KM_SLEEP);

	ncvp->core_vnode = vp;
	ncvp->core_next = 0;

	if (!cvp)
		bs->bsup_incore_vlist = ncvp;	/* If no list, start one here */
	else
		cvp->core_next = ncvp;		/* else link after last one */

	vp->v_flag = VNOMAP;
	vp->v_count = 1;
	vp->v_vfsmountedhere = 0;
	vp->v_op = &bfsvnodeops;
	vp->v_vfsp = vfsp;
	vp->v_stream = NULL;
	vp->v_type = VREG;	/* If DIR (root), changed on return */
	vp->v_rdev = vfsp->vfs_dev;
	vp->v_data = (caddr_t)data;
	vp->v_filocks = NULL;
	*vpp = vp;
	return;
}

/*
 * Search the root for a file called nm.  Return the offset of the file inode
 * so that the caller can OPTIONALLY create a vnode.
 */
off_t
bfs_searchdir(vfsp, nm, cr)
	struct vfs *vfsp;
	char *nm;
	struct cred *cr;
{
	ushort ino = 0;
	off_t ino_offset = 0;
	int error;

	error = bfs_dotsearch(vfsp, nm, cr, &ino, (off_t *)0);
	if (error)
		return 0;
	if (ino != 0)
		ino_offset = BFS_INO2OFF(ino);
	return(ino_offset);
}


/*
 * Search the root directory for a file called nm and remove the entry
 * from the directory by zeroing out the inode field.
 */
int
bfs_rmdirent(vfsp, nm, cr)
	struct vfs *vfsp;
	char *nm;
	struct cred *cr;
{

		/* LINTED vfs_data should be word aligned */
	register struct bsuper *bs = (struct bsuper *)vfsp->vfs_data;
	register struct bfs_dirent *dir;
	ushort ino = 0;
	off_t offset = 0;
	int error = 0;

	error = bfs_dotsearch(vfsp, nm, cr, &ino, &offset);
	if (error)
		return error;

	BFS_IOBEGIN(bs);
	CHECK_LOCK(bs);
	dir = (struct bfs_dirent *)
	  kmem_alloc(sizeof(struct bfs_dirent), KM_SLEEP);
	BFS_INOLOCK(bs, BFSROOTINO);
	/*
	 * Get the ROOT inode.
	 */
	error = BFS_GETINODE(bs->bsup_devnode, BFS_INO2OFF(BFSROOTINO),
				dir, cr);
	if (error) {
		kmem_free(dir, sizeof(struct bfs_dirent));
		BFS_INOUNLOCK(bs, BFSROOTINO);
		BFS_IOEND(bs);
		return(error);
	}
	/*
	 * If the entry is found, zero the inode field in the ROOT directory
	 */
	if (ino != 0  &&  offset > 0) {
		ino = 0;
		error = vn_rdwr(UIO_WRITE, bs->bsup_devnode, (caddr_t)&ino,
				sizeof(ino), offset, UIO_SYSSPACE, IO_SYNC,
				BFS_ULT, cr, (int *)0);
	}
	else
		error = ENOENT;

	if (!error) {
		dir->d_fattr.va_atime = hrestime.tv_sec;
		dir->d_fattr.va_mtime = hrestime.tv_sec;
		/*
		 * Update the ROOT inode
		 */
		error = BFS_PUTINODE(bs->bsup_devnode, BFS_INO2OFF(BFSROOTINO),
					dir, cr);
	}
	kmem_free(dir, sizeof(struct bfs_dirent));
	BFS_INOUNLOCK(bs, BFSROOTINO);
	BFS_IOEND(bs);
	return error;
}


/*
 * Search the root directory for a file called nm. If file nm is not found,
 * add the entry to the directory.
 */
int
bfs_addirent(vfsp, nm, inode, cr)
	struct vfs *vfsp;
	char *nm;
	ushort inode;
	struct cred *cr;
{

		/* LINTED vfs_data should be word aligned */
	register struct bsuper *bs = (struct bsuper *)vfsp->vfs_data;
	register struct bfs_dirent *dir;
	struct bfs_ldirs ld;
	ushort ino = 0;
	off_t offset = 0;
	int error = 0;


	error = bfs_dotsearch(vfsp, nm, cr, &ino, &offset);
	if (error)
		return error;
	if (ino != 0)
		return EEXIST;

	BFS_IOBEGIN(bs);
	CHECK_LOCK(bs);
	dir = (struct bfs_dirent *)
	  kmem_alloc(sizeof(struct bfs_dirent), KM_SLEEP);
	BFS_INOLOCK(bs, BFSROOTINO);
	/*
	 * Get the ROOT inode.
	 */
	error = BFS_GETINODE(bs->bsup_devnode, BFS_INO2OFF(BFSROOTINO),
				dir, cr);
	if (error) {
		kmem_free(dir, sizeof(struct bfs_dirent));
		BFS_INOUNLOCK(bs, BFSROOTINO);
		BFS_IOEND(bs);
		return(error);
	}
	/*
	 * If no match AND no empty slot found, find out is there is
	 * space to increase the ROOT directory. If so, update the ROOT
	 * inode to reflect the new size and set "offset" for the next
	 * entry on the ROOT directory.
	 */
	if (ino == 0  &&  offset == 0) {
		if ( (dir->d_eoffset + sizeof(struct bfs_ldirs)) < 
		     (dir->d_eblock + 1) * BFS_BSIZE) {
			offset = dir->d_eoffset +1;
			dir->d_eoffset += sizeof(struct bfs_ldirs);
		}
		else {
			/*
			 * no more available slots in directory.
			 */
			kmem_free(dir, sizeof(struct bfs_dirent));
			BFS_INOUNLOCK(bs, BFSROOTINO);
			BFS_IOEND(bs);
			return ENFILE;
		}
	}

	if (ino == 0  &&  offset > 0) {
		strncpy(ld.l_name, nm, BFS_MAXFNLEN);
		ld.l_ino = inode;

		/*
		 * Add the entry to the directory
		 */
		error = vn_rdwr(UIO_WRITE, bs->bsup_devnode, (caddr_t)&ld,
				sizeof(struct bfs_ldirs), offset, UIO_SYSSPACE,
				IO_SYNC, BFS_ULT, cr, (int *)0);
	}
	if (!error) {
		dir->d_fattr.va_atime = hrestime.tv_sec;
		dir->d_fattr.va_mtime = hrestime.tv_sec;
		/*
		 * Update the ROOT inode
		 */
		error = BFS_PUTINODE(bs->bsup_devnode, BFS_INO2OFF(BFSROOTINO),
					dir, cr);
	}

	kmem_free(dir, sizeof(struct bfs_dirent));
	BFS_INOUNLOCK(bs, BFSROOTINO);
	BFS_IOEND(bs);
	return error;
}


/*
 * Search the root directory for a file called snm, rename to tnm
 * and update the directory entry.
 */
int
bfs_rendirent(vfsp, snm, tnm, cr)
	struct vfs *vfsp;
	char *snm;
	char *tnm;
	struct cred *cr;
{

		/* LINTED vfs_data should be word aligned */
	register struct bsuper *bs = (struct bsuper *)vfsp->vfs_data;
	register struct bfs_dirent *dir;
	struct bfs_ldirs ld;
	ushort ino = 0;
	off_t offset = 0;
	int error = 0;

	error = bfs_dotsearch(vfsp, snm, cr, &ino, &offset);
	if (error)
		return error;

	BFS_IOBEGIN(bs);
	CHECK_LOCK(bs);
	dir = (struct bfs_dirent *)
	  kmem_alloc(sizeof(struct bfs_dirent), KM_SLEEP);
	BFS_INOLOCK(bs, BFSROOTINO);
	/*
	 * Get the ROOT inode.
	 */
	error = BFS_GETINODE(bs->bsup_devnode, BFS_INO2OFF(BFSROOTINO),
				dir, cr);
	if (error) {
		kmem_free(dir, sizeof(struct bfs_dirent));
		BFS_INOUNLOCK(bs, BFSROOTINO);
		BFS_IOEND(bs);
		return(error);
	}
	/*
	 * If the entry is found, change the name field to tnm.
	 */
	if (ino != 0  &&  offset > 0) {
		ld.l_ino = ino;
		strncpy(ld.l_name, tnm, BFS_MAXFNLEN);
		error = vn_rdwr(UIO_WRITE, bs->bsup_devnode, (caddr_t)&ld,
				sizeof(struct bfs_ldirs), offset, UIO_SYSSPACE,
				IO_SYNC, BFS_ULT, cr, (int *)0);
	}
	else
		error = ENOENT;

	if (!error) {
		dir->d_fattr.va_atime = hrestime.tv_sec;
		dir->d_fattr.va_mtime = hrestime.tv_sec;
		/*
		 * Update the ROOT inode
		 */
		error = BFS_PUTINODE(bs->bsup_devnode, BFS_INO2OFF(BFSROOTINO),
					dir, cr);
	}
	kmem_free(dir, sizeof(struct bfs_dirent));
	BFS_INOUNLOCK(bs, BFSROOTINO);
	BFS_IOEND(bs);
	return error;
}


/*
 * Search a directory for a file call nm. 
 * If file is found, the offset to the entry in dir and the ino to the inode #.
 * If file no found, the offset is set to the first empty slot in the dir.
 * If file no found, and no empty slots in dir, offset and ino are set to 0.
 */
static int
bfs_dotsearch(vfsp, nm, cr, inop, offp)
	struct vfs *vfsp;
	char *nm;
	struct cred *cr;
	ushort *inop;
	off_t *offp;
{
		/* LINTED vfs_data should be word aligned */
	register struct bsuper *bs = (struct bsuper *)vfsp->vfs_data;
	register struct bfs_dirent *dir;
	register struct bfs_ldirs *ld;
	char *buf;
	off_t offset;
	off_t eslot = -1;
	off_t i;
	int error = 0;
	int found = 0;
	int buflen = 0;
	int len, chunksize;

	BFS_IOBEGIN(bs);
	CHECK_LOCK(bs);
		/* LINTED kmem_alloc returns an aligned address */
	dir = (struct bfs_dirent *)
	  kmem_alloc(sizeof(struct bfs_dirent), KM_SLEEP);
	BFS_INOLOCK(bs, BFSROOTINO);
	/*
	 * Get the ROOT inode.
	 */
	error = BFS_GETINODE(bs->bsup_devnode, BFS_INO2OFF(BFSROOTINO), dir,cr);
	if (error) {
		kmem_free(dir, sizeof(struct bfs_dirent));
		BFS_INOUNLOCK(bs, BFSROOTINO);
		BFS_IOEND(bs);
		return(error);
	}
	len = dir->d_eoffset - (dir->d_sblock * BFS_BSIZE) +1;
	chunksize = MIN(len, DIRBUFSIZE);
		/* LINTED kmem_alloc returns an aligned address */
	buf = kmem_alloc(chunksize, KM_SLEEP);

	for (offset = (dir->d_sblock * BFS_BSIZE); len > 0 && found == 0;
	     len -= buflen, offset +=buflen) {	

		buflen = MIN(chunksize, len);
		/*
		 * Get the list of entries stored in the ROOT directory
		 */
		error = BFS_GETDIRLIST(bs->bsup_devnode, offset, buf,buflen,cr);
		if (error) {
			kmem_free(dir, sizeof(struct bfs_dirent));
			kmem_free(buf, chunksize);
			BFS_INOUNLOCK(bs, BFSROOTINO);
			BFS_IOEND(bs);
			return(error);
		}

		for (i = 0; i < buflen; i += sizeof(struct bfs_ldirs)) {
			ld = (struct bfs_ldirs *) (buf + i);
			if (ld->l_ino != 0 && 
			    strncmp(ld->l_name, nm, BFS_MAXFNLEN) == 0) {
				*inop = ld->l_ino;
				found++;
				if (offp)
					*offp = offset + i;
				break;
			}
			if (ld->l_ino == 0 && eslot == -1) {
				eslot = offset + i;
				if (offp)
					*offp = offset + i;
			}
		}
	}

	/*
	 * Update the access time of the ROOT inode
	 */
	if (WRITEALLOWED(bs->bsup_root, cr)) {
		dir->d_fattr.va_atime = hrestime.tv_sec;
		error = BFS_PUTINODE(bs->bsup_devnode, BFS_INO2OFF(BFSROOTINO), dir,cr);
	}

	kmem_free(dir, sizeof(struct bfs_dirent));
	kmem_free(buf, chunksize);
	BFS_INOUNLOCK(bs, BFSROOTINO);
	BFS_IOEND(bs);
	return(error);
}

/*
 * Return a pointer to an empty inode entry, and the next block to write.
 */
bfs_nextfile(bs, newdir, newblock, cr)
	struct bsuper *bs;
	off_t *newdir;
	daddr_t *newblock;
	struct cred *cr;
{
	off_t i;
	int error;
	register struct bfs_dirent *dir;

	dir = (struct bfs_dirent *)
	  kmem_alloc(sizeof(struct bfs_dirent), KM_SLEEP);
	*newdir = 0;
	*newblock = 0;

	/*
	 * Get empty inode offset.
	 */
	for (i = BFS_DIRSTART; i < bs->bsup_start;
	     i += sizeof(struct bfs_dirent)) {
		error = BFS_GETINODE(bs->bsup_devnode, i, dir, cr);
		if (error) {
			kmem_free(dir, sizeof(struct bfs_dirent));
			return error;
		}
         
		if (dir->d_ino == 0) {
			*newdir = i;
			break;
		}

	}

	/*
	 * If there are any files, the next block is the one after the
	 * last block of the last file.  Otherwise it is the first block.
	 */
	if (bs->bsup_lastfile) {
		error = BFS_GETINODE(bs->bsup_devnode,bs->bsup_lastfile,dir,cr);
		if (error) {
			kmem_free(dir, sizeof(struct bfs_dirent));
			return error;
		}
		*newblock = dir->d_eblock+1;
	}
	else
		*newblock = (bs->bsup_start / BFS_BSIZE) + 1;

	kmem_free(dir, sizeof(struct bfs_dirent));
	return 0;
}

/*
 * A file is being written past the end.  We must move it to the end of the
 * filesystem.
 */
int
bfs_filetoend(bs, dir, offset, cr)
	struct bsuper *bs;
	struct bfs_dirent *dir;
	off_t offset;
	struct cred *cr;
{
	off_t newdir,newblock;
	off_t offset1;
	off_t offset2;
	register int size;
	int chunksize;
	int error;
	long j;
	char *buf;
	
	BFS_LOCK(bs);  /* No other I/O can take place */

	/*
	 * If I/O is already in progress, sleep on it.
	 */
	if (bs->bsup_ioinprog > 1)
		while (bs->bsup_ioinprog > 1)
			sleep((caddr_t)&bs->bsup_ioinprog, PINOD);

	/*
	 * Get the next block.
	 */
	error = bfs_nextfile(bs, &newdir, &newblock, cr);
	if (error) {
		bfs_unlock(bs);
		return error;
	}

	offset1 = newblock * BFS_BSIZE;
	offset2 = dir->d_sblock * BFS_BSIZE;

	size = dir->d_eoffset - offset2;

	/*
	 * If moving this file will write over the end of the filesystem, 
	 * attempt to compact the filesystem.  If it is already compacted,
	 * we are out of space.
	 */
	if (offset1 + size > bs->bsup_end) {
		if (!bs->bsup_compacted)
			bfs_compact(bs, cr);
		else {
			bfs_unlock(bs);
			return ENOSPC;
		}
		bfs_unlock(bs);

		/*
		 * Call recursively.
		 */
		return bfs_filetoend(bs, dir, offset, cr);
	}

	/*
	 * Move the file in large chunks.
	 */
	chunksize = min(dir->d_eoffset - offset2 +1, MAXBSIZE);
	if ((buf = (char *)kmem_alloc(chunksize, KM_NOSLEEP)) == NULL) {
		bfs_unlock(bs);
		return ENOMEM;
	}

	do {
		/*
		 * Move either the whole file or chunksize, whichever is
		 * smaller.
		 */
		j = min(dir->d_eoffset - offset2 +1, chunksize);

		(void) vn_rdwr(UIO_READ, bs->bsup_devnode, (caddr_t)buf,
		  j, offset2, UIO_SYSSPACE, 0, 0, cr, 0);

		offset2 = offset2 +  j;

		(void) vn_rdwr(UIO_WRITE, bs->bsup_devnode, (caddr_t)buf,
		  j, offset1, UIO_SYSSPACE, IO_SYNC, BFS_ULT, cr, (int *)0);

		offset1 = offset1 + j;
	} while (offset2 < dir->d_eoffset);

	kmem_free(buf, chunksize);
	dir->d_eoffset = offset1 -1;
	dir->d_sblock = newblock;
	dir->d_eblock = dir->d_eoffset / BFS_BSIZE;

	/*
	 * Write the updated inode entry.
	 */
	error = BFS_PUTINODE(bs->bsup_devnode, offset, dir, cr);
	if (error) {
		bfs_unlock(bs);
		return error;
	}

	bs->bsup_lastfile = offset;

	/*
	 * We are no longer compact.
	 */
	bs->bsup_compacted = BFS_NO;

	bfs_unlock(bs);

	return 0;
}

/*
 * Compaction or filetoend has ended, wake up sleepers.
 */
void
bfs_unlock(bs)
	struct bsuper *bs;
{
	bs->bsup_fslocked = BFS_NO;
	wakeprocs((caddr_t)&bs->bsup_fslocked, PRMPT);
}

/*
 * File needs to be truncated.
 */
int
bfs_truncate(bs, diroff, length, cr)
	struct bsuper *bs;
	off_t diroff;
	long length;
	struct cred *cr;
{
	register struct bfs_dirent *dir;
	register struct bfs_dirent *ldir;
	daddr_t oeblock;
	int error = 0;

	BFS_IOBEGIN(bs);

	dir = (struct bfs_dirent *)
	  kmem_alloc(sizeof(struct bfs_dirent), KM_SLEEP);

	BFS_INOLOCK(bs, BFS_OFF2INO(diroff));
	error = BFS_GETINODE(bs->bsup_devnode, diroff, dir, cr);
	if (error) {
		BFS_IOEND(bs);
		BFS_INOUNLOCK(bs, BFS_OFF2INO(diroff));
		kmem_free(dir, sizeof(struct bfs_dirent));
		return error;
	}
	if (!dir->d_sblock) {
		BFS_IOEND(bs);
		BFS_INOUNLOCK(bs, BFS_OFF2INO(diroff));
		kmem_free(dir, sizeof(struct bfs_dirent));
		if (length == 0)
			return 0;
		else
			return EINVAL;
	}

	if (dir->d_eoffset + 1 - (dir->d_sblock*BFS_BSIZE) == length) {
		BFS_IOEND(bs);
		BFS_INOUNLOCK(bs, BFS_OFF2INO(diroff));
		kmem_free(dir, sizeof(struct bfs_dirent));
		return 0;
	}
	if (dir->d_eoffset + 1 - (dir->d_sblock*BFS_BSIZE) < length) {
		BFS_IOEND(bs);
		BFS_INOUNLOCK(bs, BFS_OFF2INO(diroff));
		kmem_free(dir, sizeof(struct bfs_dirent));
		return EINVAL;
	}

	oeblock = dir->d_eblock;
	dir->d_eoffset = (dir->d_sblock * BFS_BSIZE) + length - 1;
	dir->d_eblock = dir->d_eoffset / BFS_BSIZE;
	if (oeblock > dir->d_eblock)
		bs->bsup_freeblocks += (oeblock - dir->d_eblock);
	if (oeblock == dir->d_sblock &&  length == 0)
		bs->bsup_freeblocks++;


	if (dir->d_eoffset == ((dir->d_sblock * BFS_BSIZE) - 1)) {
		dir->d_sblock = 0;
		dir->d_eblock = 0;
		dir->d_eoffset = 0;

		error = BFS_PUTINODE(bs->bsup_devnode, diroff, dir, cr);
		if (error) {
			BFS_IOEND(bs);
			BFS_INOUNLOCK(bs, BFS_OFF2INO(diroff));
			kmem_free(dir, sizeof(struct bfs_dirent));
			return error;
		}

		if (bs->bsup_lastfile == diroff) {
			off_t l;
			off_t lastoff;
			daddr_t lastblock;

			register struct bfs_dirent *tdir;

			tdir = (struct bfs_dirent *)
			  kmem_alloc(sizeof(struct bfs_dirent), KM_SLEEP);
			lastoff = 0;
			lastblock = 0;

			for (l = BFS_DIRSTART; l < bs->bsup_start;
			     l += sizeof(struct bfs_dirent)) {

				error =BFS_GETINODE(bs->bsup_devnode,l,tdir,cr);
				if (error) {
				    BFS_IOEND(bs);
				    BFS_INOUNLOCK(bs, BFS_OFF2INO(diroff));
				    kmem_free(dir, sizeof(struct bfs_dirent));
				    kmem_free(tdir, sizeof(struct bfs_dirent));
				    return error;
				}
				if (l != diroff &&
				    tdir->d_eblock > lastblock &&
				    tdir->d_ino != 0) {
					lastblock = tdir->d_eblock;
					lastoff = l;
				}
			}
			bs->bsup_lastfile = lastoff;
			kmem_free(tdir, sizeof(struct bfs_dirent));
		} else if (oeblock - dir->d_sblock > BIGFILE) {
			ldir = (struct bfs_dirent *)
			  kmem_alloc(sizeof(struct bfs_dirent), KM_SLEEP);

			error = BFS_GETINODE(bs->bsup_devnode,bs->bsup_lastfile,
			  ldir, cr);
			if (error) {
			    BFS_IOEND(bs);
			    BFS_INOUNLOCK(bs, BFS_OFF2INO(diroff));
			    kmem_free(dir, sizeof(struct bfs_dirent));
			    kmem_free(ldir, sizeof(struct bfs_dirent));
			    return error;
			}

			if (ldir->d_sblock == oeblock + 1 &&
			    ldir->d_eblock - ldir->d_sblock < SMALLFILE) {
				bfs_compact(bs, cr);
			} else
				bs->bsup_compacted = BFS_NO;

			kmem_free(ldir, sizeof(struct bfs_dirent));
		} else
			bs->bsup_compacted = BFS_NO;
	} else
		error = BFS_PUTINODE(bs->bsup_devnode, diroff, dir, cr);

	BFS_IOEND(bs);
	BFS_INOUNLOCK(bs, BFS_OFF2INO(diroff));
	kmem_free(dir, sizeof(struct bfs_dirent));
	return error;
}

/*
 * Check for mode permission.
 */

#define	TST_GROUP	3
#define	TST_OTHER	6

int
bfs_iaccess(vp, mode, cr)
	struct vnode *vp;
	int mode;
	struct cred *cr;
{
	register struct bfs_dirent *dir;
	register struct bfsvattr attrs;

		/* LINTED vfs_data should be word aligned */
	register struct bsuper *bs = (struct bsuper *)vp->v_vfsp->vfs_data;
	register denied_mode, lshift;
	int	error, i;


		/* LINTED kmem_alloc returns an aligned address */
	dir = (struct bfs_dirent *)
	  kmem_alloc(sizeof(struct bfs_dirent), KM_SLEEP);

	BFS_IOBEGIN(bs);
	CHECK_LOCK(bs);

	error = BFS_GETINODE(BFS_DEVNODE(vp->v_vfsp), vp->v_data, dir, cr);
	if (error) {
		kmem_free(dir, sizeof(struct bfs_dirent));
		BFS_IOEND(bs);
		return error;
	}
	BFS_IOEND(bs);
	attrs = dir->d_fattr;

	kmem_free((caddr_t)dir, sizeof(struct bfs_dirent));

	if ((mode & VWRITE) && (vp->v_vfsp->vfs_flag & VFS_RDONLY))
		return EROFS;
	if (cr->cr_uid == attrs.va_uid) {
		lshift = 0;			/* TST OWNER */
	}
	else if (groupmember(attrs.va_gid, cr)) {
		mode >>= TST_GROUP;
		lshift = TST_GROUP;
	}
	else {
		mode >>= TST_OTHER;
		lshift = TST_OTHER;
	}
	if ((i = ((mode_t)attrs.va_mode & mode)) == mode)
		return 0;

	denied_mode = (mode & (~i));
	denied_mode <<= lshift;

	if ((denied_mode & (VREAD | VEXEC)) && pm_denied(cr, P_DACREAD))
		return EACCES;
	if ((denied_mode & VWRITE) && pm_denied(cr, P_DACWRITE))
		return EACCES;

	return 0;
}
