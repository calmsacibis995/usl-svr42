/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_VXFS_VX_INODE_H   /* wrapper symbol for kernel use */
#define _FS_VXFS_VX_INODE_H   /* subject to change without notice */

/* @(#)usr/src/i386/uts/fs/vxfs/vx_inode.h	1.11 16 May 1992 04:41:43 -  */
#ident	"@(#)uts-x86:fs/vxfs/vx_inode.h	1.7"
#ident	"$Header: $"

/*
 * Copyright (c) 1991, 1992 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 * 
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 * 
 *               RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *               VERITAS SOFTWARE
 * 4800 GREAT AMERICA PARKWAY, SUITE 420, SANTA CLARA, CA 95054
 */

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef _SVC_TIME_H
#include <svc/time.h>	/* REQUIRED */
#endif

#ifndef _FS_VNODE_H
#include <fs/vnode.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/time.h>	/* REQUIRED */
#include <sys/vnode.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *  setup endian type
 */
#define _VXFS_BIG_ENDIAN	1234
#define _VXFS_LITTLE_ENDIAN	4321
#define _VXFS_BYTE_ORDER	_VXFS_LITTLE_ENDIAN

#define	NDADDR_N	10		/* direct addresses in inode */
#define	NIMMED_N	96		/* immediate data in inode */
#define	ACLSZ		80		/* size of acl area */
#define	NIADDR		2		/* indirect addresses in inode */

/*
 * flags for ex_flags 
 */

#define	VX_EXF_NEW		0x0001	/* extent is newly allocated */

struct extent {
	off_t	ex_off;		/* offset of extent in file */
	ulong_t	ex_sz;		/* size of extent */
	daddr_t ex_st;		/* start block of extent */
	u_int	ex_flags;	/* flags */
};

struct hinode {		/* must match struct inode */
	struct inode	*i_forw;
	struct inode	*i_back;
	struct inode	*i_bforw;
	struct inode	*i_bback;
};

struct ifreelist {	/* must match struct inode */
	struct inode	*pad[4];
	struct inode	*av_forw;
	struct inode	*av_back;
};

struct inode {
	struct inode *i_forw;	/* 0x00 inode hash chain */
	struct inode *i_back;	/* 0x04 " */
	struct inode *i_bforw;	/* 0x08 inode block hash chain */
	struct inode *i_bback;	/* 0x0c " */
	struct inode *av_forw;	/* 0x10 freelist chain */
	struct inode *av_back;	/* 0x14 " */
 	struct vx_mlinkhd i_blink; /* 0x18 head of buffer mlinks */
 	struct vx_mlinkhd i_dlink; /* 0x20 head of logged write mlinks */
 	struct vx_mlinkhd i_tlink; /* 0x28 head of logged write trans */
 	struct vx_mlinkhd i_mlink; /* 0x30 head of inode modification mlinks */
	int	i_wsize;	/* 0x38 size of writes */
	clock_t	i_ftime;	/* 0x3c time inode put on freelist */
	struct vx_tran *i_tran;	/* 0x40 transaction for logged writes */
	dev_t	i_dev;		/* 0x44 device where inode resides */
	ino_t	i_number;	/* 0x48 i number, 1-to-1 with device address */
	struct fs *i_fs;	/* 0x4c file sys associated with this inode */
	off_t	i_diroff;	/* 0x50 offset in dir, where last entry found */
	u_long	i_vcode;	/* 0x54 version code attribute */
	short	i_owner;	/* 0x58 proc index of process locking inode */
	short	i_rwowner;	/* 0x5a proc index of process rwlocking inode */
	short	i_count;	/* 0x5c number of inode locks for i_owner */
	short	i_rwcount;	/* 0x5e num of read/write locks for i_rwowner */
	short	i_gowner;	/* 0x60 index of proc getpage locking inode */
	short	i_gcount;	/* 0x62 number of locks for i_gowner */
	short	i_diocount;	/* 0x64 count of procs using direct I/O */
	short	i_putcount;	/* 0x66 count of procs in putpage for inode */
	u_int	i_flag;		/* 0x68 flags - see below */
	int	i_advise;	/* 0x6c advisories */
	off_t	i_nsize;	/* 0x70 "new" bytes in file for IDELXWRI */
	long	i_ndblocks;	/* 0x74 number of blocks in direct extents */
	struct extent i_ext;	/* 0x78 last extent for bmap cache */
	ino_t	i_lastino;	/* 0x88 last inode allocated into directory */
	long	i_auoff;	/* 0x8c offset within au */
	ulong_t	i_nextr;	/* 0x90 expected offset of next read */
	ulong_t	i_nextio;	/* 0x94 offset of next read ahead I/O point */
	ulong_t	i_raend;	/* 0x98 offset where read ahead area ends */
	int	i_ralen;	/* 0x9c length of read ahead area */
	off_t	i_errsize;	/* 0xa0 off of lowest failing delxwri page */
	ulong_t	i_nextw;	/* 0xa4 expected offset of next write */
	int	i_pageflushes;	/* 0xa8 blocks of outstanding write flush */
	ulong_t	i_lastflushoff;	/* 0xac offset of last completed write flush */
	ulong_t	i_wflush;	/* 0xb0 offset of last write flush point */
	ulong_t	i_wseqstart;	/* 0xb4 start of series of sequential writes */
	ulong_t	i_nextfault;	/* 0xb8 next fault expected on vnode */
	ulong_t	i_getraoff;	/* 0xbc getpage read ahead offset */
	daddr_t	*i_map;		/* 0xc0 block map for swap files */
	int	i_mapsz;	/* 0xc4 length of block map */
	long	i_mapcnt;	/* 0xc8 number of pages of user mappings */
	u_int	i_intrflag;	/* 0xcc flags touched at interrupt time */
	long	i_aun;		/* 0xd0 au number */
	daddr_t	i_bno;		/* 0xd4 ilist block - in sectors */
	off_t	i_boff;		/* 0xd8 offset of disk inode in list block */
	off_t	i_dsize;	/* 0xdc max possible inode size on disk */
	struct 	icommon {	/* 0xe0 disk inode */
		long	ic_mode;	/*  0x00: mode and type of file */
		long	ic_nlink;	/*  0x04: number of links to file */
		long	ic_uid;		/*  0x08: owner's user id */
		long	ic_gid;		/*  0x0c: owner's group id */
		quad	ic_size;	/*  0x10: "disk" num of bytes in file */
#ifdef	_KERNEL
		timestruc_t ic_atime;
		timestruc_t ic_mtime;
		timestruc_t ic_ctime;
#else
		time_t	ic_atime;	/* 0x18: time last accessed */
		long	ic_atspare;
		time_t	ic_mtime;	/* 0x20: time last modified */
		long	ic_mtspare;
		time_t	ic_ctime;	/* 0x28: last time inode changed */
		long	ic_ctspare;
#endif
		char	ic_aflags;	/* 0x30: allocation flags */
		char	ic_orgtype;	/* 0x31: org type */
		ushort	ic_eopflags;	/* 0x32: extended operations */
		long	ic_eopdata;	/* 0x34: extended operations */
		union ic_ftarea {	/* 0x38: */
			struct ic_ftarea_dev {
				long	ic_rdev;    /* 0x38: device number */
			} ic_ftarea_dev;
			struct ic_ftarea_dir {
				long	ic_dotdot;  /* 0x38: parent directory */
			} ic_ftarea_dir;
			struct ic_ftarea_reg {
				long	ic_reserve; /* 0x38: prealloc space */
				long	ic_fixextsize; /* 0x3c: fixed ext size*/
			} ic_ftarea_reg;
		} ic_ftarea;
		long	ic_blocks;	/* 0x40: blocks actually held */
		long	ic_gen;		/* 0x44: generation number */
		long	ic_serial[2];	/* 0x48: inode serial number */
		union ic_org {		/* 0x50: */
			struct ic_vx_immed {
				char ic_immed[NIMMED_N];  /* 0x50 immediate */
			} ic_vx_immed;
			struct ic_vx_e4 {
				long	ic_spare;	  /* 0x50: unused */
				long	ic_ies;		  /* 0x54: ind ext sz */
				daddr_t	ic_ie[NIADDR];	  /* 0x58: indir exts */
				struct ic_dext {	  /* 0x60: dir exts */
					daddr_t	ic_de;	  /* dir ext */
					long	ic_des;	  /* dir ext size */
				} ic_dext[NDADDR_N];
			} ic_vx_e4;
		} ic_org;
	} i_ic;
	struct vnode i_vnode;	/* 0x190 vnode associated with this inode */
				/* 0x1d0 is length */
};

struct dinode {
	struct icommon	di_ic;
	char		reserved[ACLSZ];
};

#define	DINODESHIFT	8		/* log base 2 of dinode size */

#define	i_mode		i_ic.ic_mode
#define	i_nlink		i_ic.ic_nlink
#define	i_uid		i_ic.ic_uid
#define	i_gid		i_ic.ic_gid

#if _VXFS_BYTE_ORDER == _VXFS_LITTLE_ENDIAN
#define	i_size		i_ic.ic_size.val[0]
#endif
#if _VXFS_BYTE_ORDER == _VXFS_BIG_ENDIAN
#define	i_size		i_ic.ic_size.val[1]
#endif

#define	i_atime		i_ic.ic_atime
#define	i_mtime		i_ic.ic_mtime
#define	i_ctime		i_ic.ic_ctime

#define	i_aflags	i_ic.ic_aflags
#define	i_orgtype	i_ic.ic_orgtype
#define	i_eopflags	i_ic.ic_eopflags
#define	i_eopdata	i_ic.ic_eopdata

#define	i_rdev		i_ic.ic_ftarea.ic_ftarea_dev.ic_rdev
#define	i_dotdot	i_ic.ic_ftarea.ic_ftarea_dir.ic_dotdot
#define	i_fixextsize	i_ic.ic_ftarea.ic_ftarea_reg.ic_fixextsize
#define	i_reserve	i_ic.ic_ftarea.ic_ftarea_reg.ic_reserve

#define	i_blocks	i_ic.ic_blocks
#define	i_gen		i_ic.ic_gen

#if _VXFS_BYTE_ORDER == _VXFS_LITTLE_ENDIAN
#define	i_lserial	i_ic.ic_serial[0]
#define	i_hserial	i_ic.ic_serial[1]
#endif
#if _VXFS_BYTE_ORDER == _VXFS_BIG_ENDIAN
#define	i_lserial	i_ic.ic_serial[1]
#define	i_hserial	i_ic.ic_serial[0]
#endif

#define	i_spare		i_ic.ic_spare

#define	i_immed		i_ic.ic_org.ic_vx_immed.ic_immed

#define	i_spare2	i_ic.ic_org.ic_vx_e4.ic_spare
#define	i_ies		i_ic.ic_org.ic_vx_e4.ic_ies
#define	i_ie		i_ic.ic_org.ic_vx_e4.ic_ie
#define	i_dext		i_ic.ic_org.ic_vx_e4.ic_dext


#define	di_mode		di_ic.ic_mode
#define	di_nlink	di_ic.ic_nlink
#define	di_uid		di_ic.ic_uid
#define	di_gid		di_ic.ic_gid

#if _VXFS_BYTE_ORDER == _VXFS_LITTLE_ENDIAN
#define	di_size		di_ic.ic_size.val[0]
#endif
#if _VXFS_BYTE_ORDER == _VXFS_BIG_ENDIAN
#define	di_size		di_ic.ic_size.val[1]
#endif

#define	di_atime	di_ic.ic_atime
#define	di_mtime	di_ic.ic_mtime
#define	di_ctime	di_ic.ic_ctime

#define	di_aflags	di_ic.ic_aflags
#define	di_orgtype	di_ic.ic_orgtype
#define	di_eopflags	di_ic.ic_eopflags
#define	di_eopdata	di_ic.ic_eopdata

#define	di_rdev		di_ic.ic_ftarea.ic_ftarea_dev.ic_rdev
#define	di_dotdot	di_ic.ic_ftarea.ic_ftarea_dir.ic_dotdot
#define	di_fixextsize	di_ic.ic_ftarea.ic_ftarea_reg.ic_fixextsize
#define	di_reserve	di_ic.ic_ftarea.ic_ftarea_reg.ic_reserve

#define	di_blocks	di_ic.ic_blocks
#define	di_gen		di_ic.ic_gen

#if _VXFS_BYTE_ORDER == _VXFS_LITTLE_ENDIAN
#define	di_lserial	di_ic.ic_serial[0]
#define	di_hserial	di_ic.ic_serial[1]
#endif
#if _VXFS_BYTE_ORDER == _VXFS_BIG_ENDIAN
#define	di_lserial	di_ic.ic_serial[1]
#define	di_hserial	di_ic.ic_serial[0]
#endif

#define	di_spare	di_ic.ic_spare

#define	di_immed	di_ic.ic_org.ic_vx_immed.ic_immed

#define	di_spare2	di_ic.ic_org.ic_vx_e4.ic_spare
#define	di_ies		di_ic.ic_org.ic_vx_e4.ic_ies
#define	di_ie		di_ic.ic_org.ic_vx_e4.ic_ie
#define	di_dext		di_ic.ic_org.ic_vx_e4.ic_dext

/*
 * flags in i_flag
 */

#define	ILOCKED		0x00000001	/* inode is locked */
#define	IUPD		0x00000002	/* file has been modified */
#define	IACC		0x00000004	/* inode access time to be updated */
#define	IMOD		0x00000008	/* inode has been modified */
#define	IWANT		0x00000010	/* some process waiting on lock */
#define	ICHG		0x00000020	/* inode has been changed */
#define	IATIMEMOD	0x00000040	/* atime modified */
#define	IMTIMEMOD	0x00000080	/* mtime modified */
#define	IADDRVALID	0x00000100	/* bmap has validated address map */
#define	IWRITEI		0x00000200	/* mod time already set */
#define	IREADI		0x00000400	/* mod time already set */
#define	ILOG		0x00000800	/* must log inode changes to disk */
#define	IRWLOCKED	0x00001000	/* inode is rwlocked */
#define	IINACTIVE	0x00002000	/* inode is inactive */
#define	IBAD		0x00004000	/* inode is bad */
#define	IUEREAD		0x00008000	/* getpage did an unexpected read */
#define	ITRANLAZYMOD	0x00010000	/* transaction lazy mod */
#define	IGLOCKED	0x00020000	/* inode is getpage locked */
#define	INOBMAPCACHE	0x00040000	/* bmap cache is disabled */
#define	INOPUTPAGE	0x00080000	/* no putpages during reallocation */
#define	IFLUSHPAGES	0x00100000	/* flush pages before removing file */
#define	IPAGESCREATED	0x00200000	/* pages were created, no clustering */
#define	IDIRTYPAGES	0x00400000	/* inode has non-logged dirty pages */
#define	ICLOSED		0x00800000	/* delxwri file was closed */
#define	IFLUSHED	0x01000000	/* delxwri file was flushed */
#define	ISHORTENED	0x02000000	/* file has been shortened */
#define	ISYNCWRITES	0x04000000	/* file has had O_SYNC writes */
#define	IBADUPD		0x08000000	/* must be marked bad to disk */
#define	IDELSETATTR	0x10000000	/* file has had delayed setattr */

/*
 * flags in i_intrflag
 */

#define	IDELXWRI	0x0001		/* inode has delxwri data */
#define	IDELXWRIERR	0x0002		/* delxwri write has failed */
#define	ILOGWRITE	0x0004		/* inode has pending logged writes */
#define	ILOGWRIERR	0x0008		/* logged write error occurred */
#define	IDELBUF		0x0010		/* inode has pending delayed buffers */
#define	IDELBUFERR	0x0020		/* delayed buffer error occurred */
#define	IPUTERROR	0x0040		/* putpage error occurred */
#define	ILOGWRIFLUSH	0x0080		/* logged write flush occurred */

/* modes */
#define	IFMT		0170000		/* type of file */
#define	IFIFO		0010000		/* named pipe (fifo) */
#define	IFCHR		0020000		/* character special */
#define	IFDIR		0040000		/* directory */
#define	IFNAM		0050000		/* xenix special file */
#define	IFBLK		0060000		/* block special */
#define	IFREG		0100000		/* regular */
#define	IFLNK		0120000		/* symbolic link */

#define	ISUID		04000		/* set user id on execution */
#define	ISGID		02000		/* set group id on execution */
#define	ISVTX		01000		/* save swapped text even after use */
#define	IREAD		0400		/* read, write, execute permissions */
#define	IWRITE		0200
#define	IEXEC		0100

#define	VX_GEMODE	(ISGID | (IEXEC >> 3))	/* setgid + group exec mode */

/*
 * org types
 */

#define	IORG_EXT4	1		/* inode has 4 byte data block addrs */
#define	IORG_IMMED	2		/* data stored in inode */

/*
 * allocation flags
 */

#define	VX_AF_MASK	0x000f		/* allocation flags mask */
#define	VX_AF_IBAD	0x0001		/* inode is bad */
#define	VX_AF_NOEXTEND	0x0002		/* file can't be auto extended */
#define	VX_AF_ALIGN	0x0004		/* all extents must be aligned */
#define	VX_AF_NOGROW	0x0008		/* file can't grow */

/*
 * extop flags
 */

#define	VX_IEMASK	0x003f		/* extended op masks */
#define	VX_IEREMOVE	0x0001		/* deferred inode removal */
#define	VX_IETRUNC	0x0002		/* extended truncation */
#define	VX_IERTRUNC	0x0004		/* trim blocks down to i_reserve */
#define	VX_IESHORTEN	0x0008		/* shorten file to i_size */
#define	VX_IEZEROEXT	0x0010		/* zeroing an extent */
#define	VX_IETRIM	0x0020		/* trim reservation to i_size */

/*
 * vxfs specific errno values
 */

#define	VX_ERETRY	0xf001	/* iget must be retried */
#define	VX_ENOENT	0xf003	/* ENOENT return from vx_dirscan */

/*
 * Convert between inode pointers and vnode pointers
 */

#define	VTOI(vp)	((struct inode *)(void *)(vp)->v_data)
#define	ITOV(ip)	(&(ip)->i_vnode)

#define	VX_HOLE	(daddr_t)-1	/* value used when no block allocated */

/*
 * Lock and unlock inodes.
 */

#ifdef	_KERNEL

#define	VX_IRWLOCK(ip)		vx_irwlock(ip)
#define	VX_IRWUNLOCK(ip)	vx_irwunlock(ip)
#define	VX_IGLOCK(ip)		vx_iglock(ip)
#define	VX_IGUNLOCK(ip)		vx_igunlock(ip)
#define	VX_ILOCK(ip)		vx_ilock(ip)
#define	VX_IUNLOCK(ip)		vx_iunlock(ip)

#define	VX_ITIMES(ip) {					\
	TED_ASSERT("vx_itimes:1a", ip->i_flag & ILOCKED	\
		   || (ip->i_flag & IRWLOCKED		/*TED_*/	\
		      && ip->i_vnode.v_type == VDIR));	/*TED_*/	\
	if ((ip)->i_flag & (IUPD|IACC|ICHG)) {		\
		vx_itimes(ip);				\
	}						\
}

/*
 * Force any delayed writes on the inode to complete.
 */

#define	VX_DELBUF_FLUSH(ip) {			\
	if ((ip)->i_intrflag & IDELBUF) {	\
		vx_delbuf_flush(ip);		\
	}					\
}

#define	VX_LOGWRI_FLUSH(ip) {			\
	if ((ip)->i_intrflag & ILOGWRITE) {	\
		vx_logwrite_flush(ip);		\
	}					\
}

#define	VX_DELXWRI_FLUSH(ip) {			\
	if ((ip)->i_intrflag & IDELXWRI) {	\
		vx_idelxwri_realloc(ip, 1);	\
	}					\
}

/*
 * Find advisories in effect for current process
 */

#define	VX_ADVISEGET(ip)	((ip)->i_advise)

/*
 * Clean up advisories on close
 */

#define	VX_ADVISECLOSE(ip) {			\
	if ((ip)->i_advise & VX_DIRECT)	{	\
		(ip)->i_diocount = 0;		\
	}					\
	(ip)->i_advise = 0;			\
}

/*
 * Mark an inode bad.
 */

#define	VX_MARKIBAD(ip, function)	vx_markibad((ip), (function))

/*
 * Update an inode serial number.
 */

#define	VX_BUMPSERIAL(ip) {				\
	if (++(ip)->i_lserial == 0) {			\
		(ip)->i_hserial++;			\
	}						\
}

/*
 * Increment and decrement the swap lock
 */

#define	VX_SWLOCKINC() {				\
	curproc->p_swlocks++;				\
	curproc->p_flag |= SSWLOCKS;			\
}

#define	VX_SWLOCKDEC() {				\
	if (--curproc->p_swlocks == 0) {		\
		curproc->p_flag &= ~SSWLOCKS;		\
	}						\
}

/*
 * Swap lock and unlock ourselves if this is a swap vnode
 */

#define	VX_SWLOCK_VP(vp) {				\
	if ((vp)->v_flag & VISSWAP) {			\
		VX_SWLOCKINC();				\
	}						\
}

#define	VX_SWUNLOCK_VP(vp) {				\
	if ((vp)->v_flag & VISSWAP) {			\
		VX_SWLOCKDEC();				\
	}						\
}

extern struct vnodeops	vx_vnodeops;

#endif	/* _KERNEL */


/*
 * This overlays the fid structure (see vfs.h).
 */

struct vfid {
	u_short	vfid_len;
	ino_t	vfid_ino;
	long	vfid_gen;
};

/*
 * VXFS VFS private data.
 */

struct vx_vfs {
	struct vnode	*vfs_root;	/* root vnode */
	struct vnode	*vfs_devvp;	/* block device vnode */
	struct fs	*vfs_fs;	/* pointer to fs */
};

#define	VX_VFS(vfsp)	((struct vx_vfs *)(void *)(vfsp->vfs_data))
#define	getfs(vfsp)	(VX_VFS(vfsp)->vfs_fs)

#endif	/* _FS_VXFS_VX_INODE_H */
