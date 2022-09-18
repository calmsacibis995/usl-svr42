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

#ifndef _FS_VFS_H	/* wrapper symbol for kernel use */
#define _FS_VFS_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/vfs.h	1.5"
#ident	"$Header: $"

/*
 * Data associated with mounted file systems.
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * File system identifier. Should be unique (at least per machine).
 */
typedef struct {
	long val[2];			/* file system id type */
} fsid_t;

/*
 * File identifier.  Should be unique per filesystem on a single
 * machine.  This is typically called by a stateless file server
 * in order to generate "file handles".
 */
#define	MAXFIDSZ	16
#define	freefid(fidp) \
  kmem_free((caddr_t)(fidp), sizeof (struct fid) - MAXFIDSZ + (fidp)->fid_len)

typedef struct fid {
	u_short		fid_len;		/* length of data in bytes */
	char		fid_data[MAXFIDSZ];	/* data (variable length) */
} fid_t;

/*
 * Structure per mounted file system.  Each mounted file system has
 * an array of operations and an instance record.  The file systems
 * are kept on a singly linked list headed by "rootvfs" and terminated
 * by NULL.
 */
typedef struct vfs {
	struct vfs	*vfs_next;		/* next VFS in VFS list */
	struct vfsops	*vfs_op;		/* operations on VFS */
	struct vnode	*vfs_vnodecovered;	/* vnode mounted on */
	u_long		vfs_flag;		/* flags */
	u_long		vfs_bsize;		/* native block size */
	int		vfs_fstype;		/* file system type index */
	fsid_t		vfs_fsid;		/* file system id */
	caddr_t		vfs_data;		/* private data */
	dev_t		vfs_dev;		/* device of mounted VFS */
	u_long		vfs_bcount;		/* I/O count (accounting) */
	u_short		vfs_nsubmounts;		/* immediate sub-mount count */
	level_t		vfs_macceiling;		/* MAC ceiling of a mounted fs */
	level_t		vfs_macfloor;		/* MAC floor of a mounted fs */
} vfs_t;

/*
 * VFS flags.
 */
#define VFS_RDONLY	0x01		/* read-only vfs */
#define VFS_MLOCK	0x02		/* lock vfs so that subtree is stable */
#define VFS_MWAIT	0x04		/* someone is waiting for lock */
#define VFS_NOSUID	0x08		/* setuid disallowed */
#define VFS_REMOUNT	0x10		/* modify mount options only */
#define VFS_NOTRUNC	0x20		/* does not truncate long file names */
#define VFS_UNLINKABLE	0x40		/* unlink(2) can be applied to root */
#define VFS_BADBLOCK	0x80		/* disk based fs has bad block */


/*
 * Argument structure for mount(2).
 */
struct mounta {
	char	*spec;
	char	*dir;
	int	flags;
	char	*fstype;
	char	*dataptr;
	int	datalen;
};

/*
 * Operations supported on virtual file system.
 */
typedef struct vfsops {
	int	(*vfs_mount)();		/* mount file system */
	int	(*vfs_unmount)();	/* unmount file system */
	int	(*vfs_root)();		/* get root vnode */
	int	(*vfs_statvfs)();	/* get file system statistics */
	int	(*vfs_sync)();		/* flush fs buffers */
	int	(*vfs_vget)();		/* get vnode from fid */
	int	(*vfs_mountroot)();	/* mount the root filesystem */
	int	(*vfs_swapvp)();	/* not used */
	int	(*vfs_setceiling)();	/* set mac ceiling on the file system */
	int	(*vfs_filler[7])();	/* for future expansion */
} vfsops_t;

#define VFS_MOUNT(vfsp, mvp, uap, cr) \
	(*(vfsp)->vfs_op->vfs_mount)(vfsp, mvp, uap, cr)
#define VFS_UNMOUNT(vfsp, cr)	(*(vfsp)->vfs_op->vfs_unmount)(vfsp, cr)
#define VFS_ROOT(vfsp, vpp)	(*(vfsp)->vfs_op->vfs_root)(vfsp, vpp)
#define	VFS_STATVFS(vfsp, sp)	(*(vfsp)->vfs_op->vfs_statvfs)(vfsp, sp)
#define VFS_SYNC(vfsp, flag, cr) \
		(*(vfsp)->vfs_op->vfs_sync)(vfsp, flag, cr)
#define VFS_VGET(vfsp, vpp, fidp) \
		(*(vfsp)->vfs_op->vfs_vget)(vfsp, vpp, fidp)
#define VFS_MOUNTROOT(vfsp, init) \
		 (*(vfsp)->vfs_op->vfs_mountroot)(vfsp, init)
#define VFS_SETCEILING(vfsp, level) \
		(*(vfsp)->vfs_op->vfs_setceiling)(vfsp, level)
/*
 * Filesystem type switch table.
 */
typedef struct vfssw {
	char		*vsw_name;	/* type name string */
	int		(*vsw_init)();	/* init routine */
	struct vfsops	*vsw_vfsops;	/* filesystem operations vector */
	long		vsw_flag;	/* flags */
} vfssw_t;

/*
 * Public operations.
 */
extern void	vfs_mountroot();	/* mount the root */
extern void	vfs_add();		/* add a new vfs to mounted vfs list */
extern void	vfs_remove();		/* remove a vfs from mounted vfs list */
extern int	vfs_lock();		/* lock a vfs */
extern void	vfs_unlock();		/* unlock a vfs */
extern vfs_t 	*getvfs();		/* return vfs given fsid */
extern vfs_t 	*vfs_devsearch();	/* find vfs given device */
extern vfssw_t 	*vfs_getvfssw();	/* find vfssw ptr given fstype name */
extern u_long	vf_to_stf();		/* map VFS flags to statfs flags */
extern int	dounmount();		/* unmount a vfs */

#define VFS_INIT(vfsp, fstype, data) {			\
	(vfsp)->vfs_next = (struct vfs *)0;		\
	(vfsp)->vfs_op = vfssw[(fstype)].vsw_vfsops;	\
	(vfsp)->vfs_fstype = (fstype);			\
	(vfsp)->vfs_flag = 0;				\
	(vfsp)->vfs_data = (data);			\
	(vfsp)->vfs_nsubmounts = 0;			\
}

/*
 * Globals.
 */
extern struct vfs *rootvfs;		/* ptr to root vfs structure */
extern struct vfssw vfssw[];		/* table of filesystem types */
extern char rootfstype[];		/* name of root fstype */
extern int nfstype;			/* # of elements in vfssw array */

/*
 * Reasons for calling the vfs_mountroot() operation.
 */

enum whymountroot { ROOT_INIT, ROOT_REMOUNT, ROOT_UNMOUNT };
typedef enum whymountroot whymountroot_t;

/*
 * VFS_SYNC flags.
 */
#define SYNC_ATTR	0x01		/* sync attributes only */
#define SYNC_CLOSE	0x02		/* close open file */

#endif	/* _FS_VFS_H */
