/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_BFS_BFS_H	/* wrapper symbol for kernel use */
#define _FS_BFS_BFS_H	/* subject to change without notice */

#ident	"@(#)uts-comm:fs/bfs/bfs.h	1.4.3.3"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _FS_VNODE_H
#include <fs/vnode.h>		/* REQUIRED */
#endif

#ifndef _SVC_RESOURCE_H
#include <svc/resource.h>	/* SVR4.0COMPAT */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/resource.h>	/* SVR4.0COMPAT */

#else

#include <sys/resource.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#define BFS_MAXFNLEN 14			/* Maximum file length */
#define BFS_MAXFNLENN (BFS_MAXFNLEN+1)  /* Used for NULL terminated copies */

struct bfsvattr {
	vtype_t		va_type;	/* vnode type (for create) */
	mode_t		va_mode;	/* file access mode */
	uid_t		va_uid;		/* owner user id */
	uid_t		va_gid;		/* owner group id */
	nlink_t		va_nlink;	/* number of references to file */
	time_t		va_atime;	/* time of last access */
	time_t		va_mtime;	/* time of last modification */
	time_t		va_ctime;	/* time file ``created'' */
	long		va_filler[4];	/* padding */
};

/*
 * The bfs_dirent is the "inode" of BFS.  Always on disk, it is pointed
 * to (by disk offset) by the vnode and is referenced every time an
 * operation is done on the vnode.  It must be referenced every time,
 * as things can move around very quickly
 */
struct bfs_dirent
{
	ushort  d_ino;				/* inode */
	daddr_t d_sblock;			/* Start block */
	daddr_t d_eblock;			/* End block */
	daddr_t d_eoffset;			/* EOF disk offset (absolute) */
	struct  bfsvattr d_fattr;		/* File attributes */
};


struct bfs_ldirs {
	ushort l_ino;
	char   l_name[BFS_MAXFNLEN];
};

/*
 * We keep a linked list of all referenced BFS vnodes.  bfs_inactive will remove
 * them from the list, and bfs_fillvnode will add to and search through the list
 */
struct bfs_core_vnode
{
	struct vnode *core_vnode;
	struct bfs_core_vnode *core_next;
};

/*
 * The BFS superbuf contains all private data about a given BFS filesystem.
 * It is pointed to by the data field of the vfs structure and is thus passed
 * to every vfsop and vnodeops even if indirectly
 */
struct bsuper
{
	off_t bsup_start;		/* The filesystem data start offset */
	off_t bsup_end;			/* The filesystem data end offset */
	long bsup_freeblocks;		/* # of freeblocks (for statfs) */
	long bsup_freedrents;		/* # of free dir entries (for statfs) */
	struct vnode *bsup_devnode;	/* The device special vnode */
	struct vnode *bsup_root;	/* Root vnode */
	off_t bsup_lastfile;		/* Last file directory offset */

	long  bsup_inomapsz;
	char *bsup_inomap;

	/* Linked vnode list */

	struct bfs_core_vnode *bsup_incore_vlist;	

	/*
	 * bsup_ioinprog is the count of the number of io operations is 
	 * in progress.  Compaction routines sleep on this being zero
	 */
	ushort bsup_ioinprog;
	struct vnode *bsup_writelock;	/* The file which is open for write */

	/* Booleans */

	unsigned char bsup_fslocked;	/* Fs is locked when compacting */
	unsigned char bsup_compacted;	/* Fs compacted, no removes done */

};

/* The header of the disk superbuff */
struct bfs_bdsuphead
{
	long 	bh_bfsmagic;		/* Magic number */
	off_t	bh_start;		/* Filesystem data start offset */
	off_t	bh_end;			/* Filesystem data end offset */
};

/*
 * The sanity structure is used to promote sanity in compaction.  Used
 * correctly, a crash at any point during compaction is recoverable.
 */
struct bfs_sanity
{
	daddr_t fromblock;		/* "From" block of current transfer */
	daddr_t toblock;		/* "To" block of current transfer */
	daddr_t bfromblock;		/* Backup of "from" block */
	daddr_t btoblock;		/* Backup of "to" block */
};

/* The disk superbuff */
struct bdsuper
{
	struct bfs_bdsuphead bdsup_head;/* Header info */
	struct bfs_sanity bdsup_sane;	/* Crash recovery info whilst compacting */
	char    bdsup_fsname[6];	/* file system name */
	char    bdsup_volume[6];	/* file system volume name */
	long    bdsup_filler[118];	/* Padding */

};

#define	bdsup_bfsmagic	bdsup_head.bh_bfsmagic
#define	bdsup_start	bdsup_head.bh_start
#define	bdsup_end	bdsup_head.bh_end
#define	bdcp_fromblock	bdsup_sane.fromblock
#define	bdcp_toblock	bdsup_sane.toblock
#define	bdcpb_fromblock	bdsup_sane.bfromblock
#define	bdcpb_toblock	bdsup_sane.btoblock

/* Used to overlay the kernel struct fid */
struct bfs_fid_overlay
{
	ushort o_len;
	long o_offset;
};


#define BFS_MAGIC	0x1BADFACE
#define BFS_SUPEROFF	0
#define BFS_DIRSTART	(BFS_SUPEROFF + sizeof(struct bdsuper))
#define BFS_SANITYWSTART (BFS_SUPEROFF + sizeof(struct bfs_bdsuphead))
#define BFS_DEVNODE(vfsp) ((struct bsuper *)vfsp->vfs_data)->bsup_devnode
#define BFS_BSIZE	512
#define BFS_ULT		RLIM_INFINITY	/* file size limit not enforced */
#define BFS_YES		(char)1
#define BFS_NO		(char)0
#define CHUNKSIZE	4096
#define BIGFILE		500
#define SMALLFILE	10
#define BFSROOTINO	2
#define DIRBUFSIZE	1024


#define BFS_INOLOCK(bs, inode) { \
	while (bs->bsup_inomap[inode]) \
		sleep(&(bs->bsup_inomap[inode]), PINOD); \
	bs->bsup_inomap[inode] = 1; \
}

#define BFS_INOUNLOCK(bs, inode) { \
	ASSERT (bs->bsup_inomap[inode]); \
	bs->bsup_inomap[inode] = 0; \
	wakeprocs(&(bs->bsup_inomap[inode]), PRMPT); \
}

#define BFS_OFF2INO(offset) \
	((offset - BFS_DIRSTART) / sizeof(struct bfs_dirent)) + BFSROOTINO

#define BFS_INO2OFF(inode) \
	((inode - BFSROOTINO) * sizeof(struct bfs_dirent)) + BFS_DIRSTART

#define BFS_GETINODE(bvp, offset, buf, cr) \
	vn_rdwr(UIO_READ, bvp, (caddr_t)buf, sizeof(struct bfs_dirent), \
				offset, UIO_SYSSPACE, 0, 0, cr, 0)

#define BFS_PUTINODE(bvp, offset, buf, cr) \
	vn_rdwr(UIO_WRITE, bvp, (caddr_t)buf, sizeof(struct bfs_dirent), \
			offset, UIO_SYSSPACE, IO_SYNC, BFS_ULT, cr, (int *)0)

#define BFS_GETDIRLIST(bvp, offset, buf, len, cr) \
	vn_rdwr(UIO_READ, bvp, buf, len, offset, UIO_SYSSPACE, 0, 0, cr, 0)

#define CHECK_LOCK(bs) \
	if (bs->bsup_fslocked) \
		while (bs->bsup_fslocked) \
			sleep((caddr_t)&bs->bsup_fslocked, PINOD)


#define BFS_LOCK(bs) bs->bsup_fslocked = BFS_YES


#define BFS_IOBEGIN(bs) bs->bsup_ioinprog++

#define BFS_IOEND(bs) if (!(--bs->bsup_ioinprog)) \
			wakeprocs((caddr_t)&bs->bsup_ioinprog, PRMPT)

#ifdef _KERNEL
extern int bfs_addirent();
extern int bfs_compact();
extern int bfs_filetoend();
extern void bfs_fillvnode();
extern int bfs_iaccess();
extern int bfs_nextfile();
extern int bfs_rendirent();
extern int bfs_rmdirent();
extern int bfs_truncate();
extern void bfs_unlock();
extern int bfsinit();
extern off_t bfs_searchdir();
#endif	/* _KERNEL */

#endif	/* _FS_BFS_BFS_H */
