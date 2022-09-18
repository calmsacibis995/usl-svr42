/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)uts-x86:fs/s5fs/s5machdep.c	1.4"
#ident	"$Header: $"

#include <util/types.h>
#include <fs/buf.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <util/sysmacros.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/s5fs/s5param.h>
#include <fs/s5fs/s5dir.h>
#include <fs/s5fs/s5ino.h>
#include <fs/s5fs/s5inode.h>
#include <fs/s5fs/s5macros.h>


int
iread(ip, ino)
	register struct inode *ip;
	int ino;
{
	register char *p1, *p2;
	register struct vnode *vp = ITOV(ip);
	register struct dinode *dp;
	struct buf *bp;
	register unsigned i;
	register struct vfs *vfsp;
	register struct s5vfs *s5vfsp;
	int error;

	vfsp = vp->v_vfsp;
	s5vfsp = S5VFS(vfsp);
	i = vfsp->vfs_bsize;
	bp = bread(vfsp->vfs_dev, FsITOD(s5vfsp, ino), i);
	if (error = geterror(bp)) {
		brelse(bp);
		return error;
	}
	dp = (struct dinode *)bp->b_un.b_addr;
	dp += FsITOO(s5vfsp, ino);
	ip->i_nlink = dp->di_nlink;
	ip->i_uid = dp->di_uid;
	ip->i_gid = dp->di_gid;
	ip->i_size = dp->di_size;
	ip->i_mode = dp->di_mode;
	ip->i_atime = dp->di_atime;
	ip->i_mtime = dp->di_mtime;
	ip->i_ctime = dp->di_ctime;
	ip->i_number = (o_ino_t)ino;
	ip->i_nextr = 0;
	ip->i_gen = dp->di_gen;
	p1 = (char *) ip->i_addr;
	p2 = (char *) dp->di_addr;
		/*
		** the following copy is machine (byte order) specific
		*/
	for (i = 0; i < NADDR; i++) {
		*p1++ = *p2++;
		*p1++ = *p2++;
		*p1++ = *p2++;
		*p1++ = 0;
	}

	if (ip->i_mode & IFBLK || ip->i_mode == IFCHR) {
		if (ip->i_bcflag & NDEVFORMAT)
			ip->i_rdev = makedevice(ip->i_major, ip->i_minor);
		else
			ip->i_rdev = expdev(ip->i_oldrdev);
	} else if (ip->i_mode & IFNAM)
		ip->i_rdev = ip->i_oldrdev;

	brelse(bp);
	return 0;
}

/*
 * Compare incore inode to disk inode.
 */
int
s5icmp(ip, dp)
	register struct inode *ip;
	register struct dinode *dp;
{
	register char *p1, *p2;
	register int i;

	if ( ip->i_nlink != dp->di_nlink || ip->i_uid != dp->di_uid ||
		ip->i_gid != dp->di_gid || ip->i_size != dp->di_size ||
		ip->i_mode != dp->di_mode || ip->i_atime != dp->di_atime || 
		ip->i_mtime != dp->di_mtime || ip->i_ctime != dp->di_ctime ||
		ip->i_gen != dp->di_gen )

			return 1;
	
	p1 = (char *) ip->i_addr;
	p2 = (char *) dp->di_addr;

	for (i = 0; i < NADDR; i++) {
		if ( *p1++ != *p2++ || *p1++ != *p2++ || *p1++ != *p2++ )
			return 1;
		p1++;
	}
	return 0;
}

/*
 * Flush inode to disk, updating timestamps if requested.
 */
void
iupdat(ip)
	register struct inode *ip;
{
	struct buf *bp;
	register struct vnode *vp = ITOV(ip);
	register struct s5vfs *s5vfsp = S5VFS(vp->v_vfsp);
	register struct dinode *dp;
	register char *p1;
	register char *p2;
	register unsigned i;

	ASSERT(ip->i_flag & ILOCKED);
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
		return;
	i = VBSIZE(vp);
	bp = bread(ip->i_dev, FsITOD(s5vfsp, ip->i_number), i);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return;
	}
	dp = (struct dinode *)bp->b_un.b_addr;
	dp += FsITOO(s5vfsp, ip->i_number);
	dp->di_mode = ip->i_mode;
	dp->di_nlink = ip->i_nlink;
	dp->di_uid = ip->i_uid;
	dp->di_gid = ip->i_gid;
	dp->di_size = ip->i_size;
	dp->di_gen = ip->i_gen;
	p1 = (char *)dp->di_addr;
	p2 = (char *)ip->i_addr;
		/*
		** the following copy is machine (byte order) specific
		*/
	for (i = 0; i < NADDR; i++) {
		*p1++ = *p2++;
		*p1++ = *p2++;
		*p1++ = *p2++;
		p2++;
	}
	if (ip->i_flag & IACC)
		ip->i_atime = hrestime.tv_sec;
	if (ip->i_flag & IUPD) 
		ip->i_mtime = hrestime.tv_sec;
	if (ip->i_flag & ICHG) 
		ip->i_ctime = hrestime.tv_sec;
	dp->di_atime = ip->i_atime;
	dp->di_mtime = ip->i_mtime;
	dp->di_ctime = ip->i_ctime;
	if (ip->i_flag & ISYN)
		bwrite(bp);
	else
		bdwrite(bp);
	ip->i_flag &= ~(IACC|IUPD|ICHG|ISYN|IMOD);
}
