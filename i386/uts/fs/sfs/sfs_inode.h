/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_SFS_SFS_INODE_H	/* wrapper symbol for kernel use */
#define _FS_SFS_SFS_INODE_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/sfs/sfs_inode.h	1.15"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _ACC_DAC_ACL_H
#include <acc/dac/acl.h>	/* REQUIRED */
#endif

#ifndef _ACC_MAC_MAC_H
#include <acc/mac/mac.h>	/* REQUIRED */
#endif

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#ifndef _SVC_TIME_H
#include <svc/time.h>		/* REQUIRED */
#endif

#ifndef _FS_VNODE_H
#include <fs/vnode.h>		/* REQUIRED */
#endif

#ifndef _FS_FSINODE_H
#include <fs/fsinode.h>		/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/acl.h>		/* REQUIRED */
#include <sys/mac.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */
#include <sys/time.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/fsinode.h>	/* REQUIRED */

#else

#include <sys/acl.h>
#include <sys/fsinode.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */




/*
 * The I node is the focus of all local file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, each mapping,
 * and the root.  An inode is `named' by its dev/inumber pair.
 * Data in icommon is read in from permanent inode on volume.
 */

#define EFT_MAGIC 0x90909090	/* magic cookie for EFT */
#define	NDADDR	12		/* direct addresses in inode */
#define	NIADDR	3		/* indirect addresses in inode */
#define	NIPFILE	2		/* number of inodes per file */
#define	NACLI	8		/* number of ACL entries in alternate inode */

struct 	icommon {
	o_mode_t ic_smode;	/*  0: mode and type of file */
	short	ic_nlink;	/*  2: number of links to file */
	o_uid_t	ic_suid;	/*  4: owner's user id */
	o_gid_t	ic_sgid;	/*  6: owner's group id */
	quad	ic_size;	/*  8: number of bytes in file */
#ifdef _KERNEL
	struct timeval ic_atime;/* 16: time last accessed */
	struct timeval ic_mtime;/* 24: time last modified */
	struct timeval ic_ctime;/* 32: last time inode changed */
#else
	time_t	ic_atime;	/* 16: time last accessed */
	long	ic_atspare;
	time_t	ic_mtime;	/* 24: time last modified */
	long	ic_mtspare;
	time_t	ic_ctime;	/* 32: last time inode changed */
	long	ic_ctspare;
#endif
	daddr_t	ic_db[NDADDR];	/* 40: disk block addresses */
	daddr_t	ic_ib[NIADDR];	/* 88: indirect blocks */
	long	ic_flags;	/* 100: status, currently unused */
	long	ic_blocks;	/* 104: blocks actually held */
	long	ic_gen;		/* 108: generation number */
	mode_t	ic_mode;	/* 112: EFT version of mode*/
	uid_t	ic_uid;		/* 116: EFT version of uid */
	gid_t	ic_gid;		/* 120: EFT version of gid */
	ulong	ic_eftflag;	/* 124: indicate EFT version*/
};

/*
 * Enhanced security information kept in alternate inode.
 */
union 	i_secure {	
	struct icommon is_com;
	struct isecdata {
		lid_t	isd_lid;	/* Level IDentifier for file */
		long	isd_sflags;	/* flags (used by MLD only) */
		long	isd_aclcnt;	/* ACL count */
		long	isd_daclcnt;	/* default ACL count */
		daddr_t	isd_aclblk;	/* extended ACL disk blk */
		struct acl isd_acl[NACLI];  /* ACL entries */
		lid_t	isd_cmwlid;	/* Level IDentifier for file CMW */
		char	isd_filler[8];	/* reserved */
	} is_secdata;
	char 	is_size[128];
};

/*
 * Incore inode.
 * The first few fields of the structure comprise the header
 * required by the common inode pool scheme; see fsinode.h.
 */
struct inode {
	/* Filesystem independent view of this inode. */
	struct inode	*i_forw;	/* hash chain, forward */
	struct inode	*i_back;	/* hash chain, back */
	struct inode	*i_freef;	/* free chain, forward */
	struct inode	*i_freeb;	/* free chain, back */
	struct vnode	*i_vp;		/* ptr to vnode */
	struct idata	*i_data;	/* pointer to the pool data */

	/* Filesystem dependent view of this inode. */
	union  i_secure	*i_secp;	/* extra memory for security data */
	struct	vnode	i_vnode;	/* vnode for this inode */

	struct	vnode 	*i_devvp;	/* vnode for block I/O */
	u_short	i_flag;		/* inode flags (see below) */
	dev_t	i_dev;		/* device where inode resides */
	ino_t	i_number;	/* i number, 1-to-1 with device address */
	off_t	i_diroff;	/* offset in dir, where we found last entry */
	struct	fs *i_fs;	/* file sys associated with this inode */
	struct	dquot *i_dquot;	/* quota structure controlling this file */
	short	i_owner;	/* proc index of process locking inode */
	short	i_count;	/* number of inode locks for i_owner */
	daddr_t	i_nextr;	/* next byte read offset (read-ahead) */
	ulong	i_vcode;	/* version code attribute */
	long	i_mapcnt;	/* mappings to file pages */
	int	*i_map;		/* block list for the corresponding file */
	int	i_opencnt;	/* count of opens for this inode */
	lid_t	i_dirofflid;	/* last proc changing i_diroff w/o write access */
	clock_t	i_stamp;	/* time when inode was modified but not copied
				 * to the buffer cache */
	struct 	icommon i_ic;	
};

/*
 * Disk inode, 128 bytes worth.
 * Each file in an SFS file system is composed of an even/odd inode
 * pair.  The even inode contains information in the form of the icommon
 * structure; the odd (or alternate) inode contains information in the
 * the form of the isecdata structure.
 */
struct dinode {
	union {
		struct	icommon di_icom;
		struct	isecdata di_secdata;
		char	di_size[128];
	} di_un;
};

/*
 * Inode hashing
 */

#define	INOHSZ	512
union ihead {			
	union  ihead *ih_head[2];
	struct inode *ih_chain[2];
};
extern union ihead sfs_ihead[]; 	/* sfs/ufs hash table */

/*
 * Defines for isd_sflags, in struct isecdata.
 */

#define ISD_MLD 	0x00000001	/* indicates a Multi-Level Directory */

#define	is_ic		i_secp->is_com
#define	is_is		i_secp->is_secdata
#define i_lid		is_is.isd_lid
#define i_sflags	is_is.isd_sflags
#define	i_aclcnt	is_is.isd_aclcnt
#define	i_daclcnt	is_is.isd_daclcnt
#define	i_aclblk	is_is.isd_aclblk
#define	i_acl		is_is.isd_acl
#define i_mode		i_ic.ic_mode
#define	i_nlink		i_ic.ic_nlink
#define i_uid		i_ic.ic_uid
#define i_gid		i_ic.ic_gid
#define i_smode		i_ic.ic_smode
#define i_suid		i_ic.ic_suid
#define i_sgid		i_ic.ic_sgid
#define i_eftflag	i_ic.ic_eftflag

/*
 * The definition of i_size is dependent on a machine's byte
 * ordering.  This is controlled by _SFS_BYTE_ORDER which is
 * defined to be either _SFS_BIG_ENDIAN (e.g., on a 3b2) or
 * _SFS_LITTLE_ENDIAN (e.g., on a 386).
 */
#define _SFS_BIG_ENDIAN		1234
#define _SFS_LITTLE_ENDIAN	4321
#define _SFS_BYTE_ORDER		_SFS_LITTLE_ENDIAN

#if _SFS_BYTE_ORDER == _SFS_LITTLE_ENDIAN
#define	i_size		i_ic.ic_size.val[0]
#endif
#if _SFS_BYTE_ORDER == _SFS_BIG_ENDIAN
#define	i_size		i_ic.ic_size.val[1]
#endif
#define	i_db		i_ic.ic_db
#define	i_ib		i_ic.ic_ib

#define	i_atime		i_ic.ic_atime
#define	i_mtime		i_ic.ic_mtime
#define	i_ctime		i_ic.ic_ctime

#define i_blocks	i_ic.ic_blocks
#define	i_oldrdev	i_ic.ic_db[0]
#define	i_rdev		i_ic.ic_db[1]
#define	i_gen		i_ic.ic_gen

#define di_ic		di_un.di_icom
#define	di_is		di_un.di_secdata
#define	di_aclcnt	di_is.isd_aclcnt
#define	di_daclcnt	di_is.isd_daclcnt
#define	di_aclblk	di_is.isd_aclblk
#define	di_acl		di_is.isd_acl
#define	di_mode		di_ic.ic_mode
#define	di_nlink	di_ic.ic_nlink
#define	di_uid		di_ic.ic_uid
#define	di_gid		di_ic.ic_gid
#define di_smode	di_ic.ic_smode
#define di_suid		di_ic.ic_suid
#define di_sgid		di_ic.ic_sgid
#define di_eftflag	di_ic.ic_eftflag

#if _SFS_BYTE_ORDER == _SFS_LITTLE_ENDIAN
#define	di_size		di_ic.ic_size.val[0]
#endif
#if _SFS_BYTE_ORDER == _SFS_BIG_ENDIAN
#define	di_size		di_ic.ic_size.val[1]
#endif
#define	di_db		di_ic.ic_db
#define	di_ib		di_ic.ic_ib

#define	di_atime	di_ic.ic_atime
#define	di_mtime	di_ic.ic_mtime
#define	di_ctime	di_ic.ic_ctime

#define	di_oldrdev	di_ic.ic_db[0]
#define	di_rdev		di_ic.ic_db[1]
#define	di_blocks	di_ic.ic_blocks
#define	di_gen		di_ic.ic_gen

/* flags */
#define	ILOCKED		0x001		/* inode is locked */
#define	IUPD		0x002		/* file has been modified */
#define	IACC		0x004		/* inode access time to be updated */
#define	IMOD		0x008		/* inode has been modified */
#define	IWANT		0x010		/* some process waiting on lock */
#define	ISYNC		0x020		/* do all allocation synchronously */
#define	ICHG		0x040		/* inode has been changed */
#define	ILWAIT		0x080		/* someone waiting on file lock */
#define	IREF		0x100		/* inode is being referenced */
#define	INOACC		0x200		/* no access time update in getpage */
#define	IMODTIME	0x400		/* mod time already set */
#define	IINACTIVE	0x800		/* iinactive in progress */
#define	IRWLOCKED	0x1000		/* inode is rwlocked */

/* modes */
#define	IFMT		0170000		/* type of file */
#define	IFIFO		0010000		/* named pipe (fifo) */
#define	IFCHR		0020000		/* character special */
#define	IFDIR		0040000		/* directory */
#define	IFNAM		0050000		/* XENIX special named file */
#define	IFBLK		0060000		/* block special */
#define	IFREG		0100000		/* regular */
#define	IFLNK		0120000		/* symbolic link */
#define	IFSOCK		0140000		/* socket */

#define	ISUID		04000		/* set user id on execution */
#define	ISGID		02000		/* set group id on execution */
#define	ISVTX		01000		/* save swapped text even after use */
#define	IREAD		0400		/* read, write, execute permissions */
#define	IWRITE		0200
#define	IEXEC		0100


#ifdef _KERNEL
extern struct vfsops	sfs_vfsops;	/* vfs operations for sfs */
extern struct vfsops	ufs_vfsops;	/* vfs operations for ufs */
extern struct vnodeops	sfs_vnodeops;	/* vnode operations for sfs */

extern int		sfs_aclget();
extern int		sfs_aclstore();
extern void		sfs_flushi();
extern int		sfs_free();
extern void		sfs_fsinvalid();
extern void		sfs_remque();
extern int		sfs_syncip();
extern void		sfs_update();

/*
 * SFS VFS private data.
 */
struct sfs_vfs {
	struct vnode	*vfs_root;	/* root vnode */
	struct buf	*vfs_bufp;	/* buffer containing superblock */
	struct vnode	*vfs_devvp;	/* block device vnode */
	u_long 		vfs_flags;	/* private vfs flags */
	struct inode	*vfs_qinod;	/* QUOTA: pointer to quota file */
	u_short		vfs_qflags;	/* QUOTA: filesystem flags */
	u_long		vfs_btimelimit;	/* QUOTA: block time limit */
	u_long		vfs_ftimelimit;	/* QUOTA: file time limit */
};

/*
 * Flags for vfs_flags in struct sfs_vfs
 */

#define SFS_FSINVALID	0x1	/* file system invalid due to error */
#define SFS_UFSMOUNT	0x2	/* file system mounted as UFS file system */

/*
 * Macros to determine UFS file system type.
 */
#define UFSVFSP(vfsp) ( \
	((struct sfs_vfs *)vfsp->vfs_data)->vfs_flags \
	& SFS_UFSMOUNT ? 1 : 0 \
)

#define UFSIP(ip) ( \
	(UFSVFSP((ITOV(ip))->v_vfsp)) \
)

/*
 * Convert between inode pointers and vnode pointers
 */
#define VTOI(vp)	((struct inode *)(vp)->v_data)
#define ITOVADDR(ip)	((struct vnode *)&(ip)->i_vnode)
#define ITOV(ip)	((struct vnode *)&(ip)->i_vnode)
#define ITOI_SEC(ip)	(ip)->i_secp

/*
 * Lock and unlock inodes.
 */

#define	IRWLOCK(ip) { \
	while ((ip)->i_flag & IRWLOCKED) { \
		(ip)->i_flag |= IWANT; \
		(void) sleep((caddr_t)(ip), PINOD); \
	} \
	(ip)->i_flag |= IRWLOCKED; \
	if (((ip)->i_vnode.v_flag & VISSWAP) != 0) { \
		curproc->p_swlocks++; \
		curproc->p_flag |= SSWLOCKS; \
	} \
}

#define	IRWUNLOCK(ip) { \
	ASSERT((ip)->i_flag & IRWLOCKED); \
	if (((ip)->i_vnode.v_flag & VISSWAP) != 0) \
		if (--curproc->p_swlocks == 0)  \
			curproc->p_flag &= ~SSWLOCKS; \
	(ip)->i_flag &= ~IRWLOCKED; \
	if ((ip)->i_flag & IWANT) { \
		(ip)->i_flag &= ~IWANT; \
		wakeprocs((caddr_t)(ip), PRMPT); \
	} \
}

#define	ILOCK(ip) { \
	while (((ip)->i_flag & ILOCKED) && \
	    (ip)->i_owner != curproc->p_slot) { \
		(ip)->i_flag |= IWANT; \
		(void) sleep((caddr_t)(ip), PINOD); \
	} \
	(ip)->i_owner = curproc->p_slot; \
	(ip)->i_count++; \
	(ip)->i_flag |= ILOCKED; \
	if (((ip)->i_vnode.v_flag & VISSWAP) != 0){ \
		curproc->p_swlocks++;	\
		curproc->p_flag |= SSWLOCKS;\
	} \
}

#define	IUNLOCK(ip) { \
	if (--(ip)->i_count < 0) \
		panic("IUNLOCK"); \
	if (((ip)->i_vnode.v_flag & VISSWAP) != 0) \
		if (--curproc->p_swlocks == 0)  \
			curproc->p_flag &= ~SSWLOCKS; \
	if ((ip)->i_count == 0) { \
		(ip)->i_flag &= ~ILOCKED; \
		if ((ip)->i_flag & IWANT) { \
			(ip)->i_flag &= ~IWANT; \
			wakeprocs((caddr_t)(ip), PRMPT); \
		} \
	} \
}

#define IUPDAT(ip, waitfor) { \
	if (ip->i_flag & (IUPD|IACC|ICHG|IMOD)) \
		sfs_iupdat(ip, waitfor); \
}

/*
 * Mark an inode with the current (unique) timestamp.
 */
extern struct timeval sfs_iuniqtime;


#define TMARK(ip) { \
	if (hrestime.tv_sec > sfs_iuniqtime.tv_sec || \
		hrestime.tv_nsec/1000 > sfs_iuniqtime.tv_usec) { \
		sfs_iuniqtime.tv_sec = hrestime.tv_sec; \
		sfs_iuniqtime.tv_usec = hrestime.tv_nsec/1000; \
	} else { \
		if (!mac_installed || UFSIP(ip)) \
			sfs_iuniqtime.tv_usec++; \
	} \
}
#define IMARK(ip) { \
	TMARK(ip); \
	if ((ip)->i_flag & IACC) \
		(ip)->i_atime = sfs_iuniqtime; \
	if ((ip)->i_flag & IUPD) { \
		(ip)->i_mtime = sfs_iuniqtime; \
		(ip)->i_flag |= IMODTIME; \
	} \
	if ((ip)->i_flag & ICHG) { \
		(ip)->i_diroff = 0; \
		(ip)->i_ctime = sfs_iuniqtime; \
	} \
}
#define ACC_TIMES(ip) {\
	if ((ip)->i_flag & IACC) {\
		(ip)->i_flag |= IMOD; \
		TMARK(ip); \
		(ip)->i_atime = sfs_iuniqtime; \
		(ip)->i_flag &= ~IACC; \
	}\
}
/*
 * When calling UPD_TIMES, both IUPD and ICHG are set, so check only one
 */
#define UPD_TIMES(ip) {\
	if ((ip)->i_flag & IUPD) {\
		(ip)->i_flag |= IMOD; \
		TMARK(ip); \
		(ip)->i_mtime = sfs_iuniqtime; \
		(ip)->i_flag |= IMODTIME; \
		(ip)->i_ctime = sfs_iuniqtime; \
		(ip)->i_flag &= ~(IUPD|ICHG);\
	}\
}
#define ITIMES(ip) { \
	if ((ip)->i_flag & (IUPD|IACC|ICHG)) { \
		(ip)->i_flag |= IMOD; \
		IMARK(ip); \
		(ip)->i_flag &= ~(IACC|IUPD|ICHG); \
	} \
}


/*
 * Allocate the specified block in the inode
 * and make sure any in-core pages are initialized.
 */
#define	BMAPALLOC(ip, lbn, size) \
	sfs_bmap((ip), (lbn), (daddr_t *)NULL, (daddr_t *)NULL, (size), S_WRITE, 0, cr)

#define ESAME	(-1)		/* trying to rename linked files (special) */

/*
 * Check that file is owned by current user (or P_OWNER privilege).
 */
#define OWNER(CR, IP)	(((CR)->cr_uid == (IP)->i_uid)? 0: (!pm_denied(CR, P_OWNER)? 0: EPERM))

#define	SFS_HOLE	(daddr_t)-1	/* value used when no block allocated */

/*
 * enums
 */
enum de_op { DE_CREATE, DE_MKDIR, DE_LINK, DE_RENAME, DE_MKMLD }; /* direnter ops */
enum dr_op	{ DR_REMOVE, DR_RMDIR, DR_RENAME };	/* dirremove ops */
enum iupmode { IUP_SYNC, IUP_DELAY, IUP_LAZY };	/* iupdate modes */
/*
 * This overlays the fid structure (see vfs.h)
 */
struct ufid {
	u_short	ufid_len;
	ino_t	ufid_ino;
	long	ufid_gen;
};

/*
 * If the length of a target of a symbolic link is short enough, then the
 * target will be stored in the disk block field of the inode rather than
 * keeping a full page busy with the data.  SHORTSYMLINK is the maximum
 * name length, in bytes, of a symlink whose target can be stored in the
 * disk address blocks.
 */
#define	SHORTSYMLINK	((NDADDR + NIADDR - 1) * sizeof(daddr_t))

#endif	/* _KERNEL */

#endif /* _FS_SFS_SFS_INODE_H */
