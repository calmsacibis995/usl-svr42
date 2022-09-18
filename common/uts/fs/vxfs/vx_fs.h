/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_fs.h	1.13 16 May 1992 05:18:53 -  */
#ident	"@(#)uts-comm:fs/vxfs/vx_fs.h	1.5"

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

#ifndef	_FS_VXFS_VX_FS_H
#define	_FS_VXFS_VX_FS_H

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef _UTIL_PARAM_H
#include <util/param.h>	/* REQUIRED */
#endif

#ifndef _SVC_TIME_H
#include <svc/time.h>	/* REQUIRED */
#endif

#ifndef _FS_VFS_H
#include <fs/vfs.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/param.h>	/* REQUIRED */
#include <sys/time.h>	/* REQUIRED */
#include <sys/vfs.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *  fs.h -- primary file system space management structures
 *
 * Each logical volume contains exactly one file system.
 * A file system consists of a number of allocation units.
 * Each allocation unit has inodes and data.
 *
 * A file system is described by its super-block, which in turn
 * describes the allocation units.
 *
 */

/*
 * The mlink structure links asynchronous updates to transactions.
 * The mlinkhd structure is used as a list header in inodes and
 * maps, it must match the start of the mlink structure.
 */

struct	vx_mlinkhd {
	struct vx_mlink	*ml_forw;	/* forw link of inode/map list */
	struct vx_mlink	*ml_back;	/* back link of inode/map list */
};

struct	vx_mlink {
	struct vx_mlink	*ml_forw;	/* forw link of inode/map list */
	struct vx_mlink	*ml_back;	/* back link of inode/map list */
	struct vx_mlink *ml_fsforw;	/* forw link on fs_mlink queue */
	struct vx_mlink *ml_fsback;	/* back link on fs_mlink queue */
	struct vx_tran	*ml_tranp;	/* pointer to tran for mlink */
	int		ml_flag;	/* flag */
	struct fs	*ml_fs;		/* file system structure */
	union {
		struct ml_mli {		/* used for VX_MLINODE mlinks */
			struct inode	*mi_ip;	    /* pointer to inode */
		} ml_mli;
		struct ml_mlm {		/* used for VX_MLMAP mlinks */
			struct vx_map	*mm_map;    /* pointer to map */
			struct vx_ctran	*mm_tcp;    /* pointer to ctran */
		} ml_mlm;
		struct ml_mld {		/* used for VX_MLDATA mlinks */
			struct inode	*md_ip;     /* pointer to inode */
			off_t		md_off;     /* offset of logged write */
			u_int		md_len;     /* length of logged write */
		} ml_mld;
		struct ml_mlbh {	/* used for VX_MLBUFHEAD mlinks */
			struct vx_mlinkhd mh_link;  /* list of mlinks for buf */
			struct inode	*mh_ip;     /* pointer to inode */
			daddr_t		mh_bno;     /* blkno of buffer */
			u_int		mh_len;     /* length of buffer */
		} ml_mlbh;
	} ml_ml; 	
};

#define	mli_ip		ml_ml.ml_mli.mi_ip
#define	mlm_map		ml_ml.ml_mlm.mm_map
#define	mlm_tcp		ml_ml.ml_mlm.mm_tcp
#define	mld_ip		ml_ml.ml_mld.md_ip
#define	mld_off		ml_ml.ml_mld.md_off
#define	mld_len		ml_ml.ml_mld.md_len
#define	mlbh_link	ml_ml.ml_mlbh.mh_link
#define	mlbh_ip		ml_ml.ml_mlbh.mh_ip
#define	mlbh_bno	ml_ml.ml_mlbh.mh_bno
#define	mlbh_len	ml_ml.ml_mlbh.mh_len

/*
 * values for ml_flag
 */

#define	VX_MLTYPEMASK	0x01f		/* mask of mlink types */
#define	VX_MLMAP	0x001		/* mlink is for a map */
#define	VX_MLINODE	0x002		/* mlink is for an inode */
#define	VX_MLDATA	0x004		/* mlink is for logged data */
#define	VX_MLBUFFER	0x008		/* mlink is for async buffer */
#define	VX_MLBUFHEAD	0x010		/* mlink is header for async buffer */
#define	VX_MLAFLUSHED	0x020		/* async flush attempted on mlink */
#define	VX_MLSFLUSHED	0x040		/* sync flush attempted on mlink */
#define	VX_MLERROR	0x080		/* update failed */
#define	VX_MLBFLUSHED	0x100		/* delbuf flush attempted on mlink */

/*
 * Add/remove an entry to/from an mlink ml_forw/back list
 */

#define	VX_MLINK_ADD(mlink, nlink) { \
	(nlink)->ml_forw = (mlink)->ml_forw; \
	(nlink)->ml_back = (mlink); \
	(mlink)->ml_forw = (nlink); \
	(nlink)->ml_forw->ml_back = (nlink); \
}

#define	VX_MLINK_REM(mlink) { \
	(mlink)->ml_forw->ml_back = (mlink)->ml_back; \
	(mlink)->ml_back->ml_forw = (mlink)->ml_forw; \
	(mlink)->ml_forw = (mlink); \
	(mlink)->ml_back = (mlink); \
}

#define	VX_MLINK_FSADD(mlink, nlink) { \
	(nlink)->ml_fsforw = (mlink)->ml_fsforw; \
	(nlink)->ml_fsback = (mlink); \
	(mlink)->ml_fsforw = (nlink); \
	(nlink)->ml_fsforw->ml_fsback = (nlink); \
}

#define	VX_MLINK_FSREM(mlink) { \
	(mlink)->ml_fsforw->ml_fsback = (mlink)->ml_fsback; \
	(mlink)->ml_fsback->ml_fsforw = (mlink)->ml_fsforw; \
	(mlink)->ml_fsforw = (mlink); \
	(mlink)->ml_fsback = (mlink); \
}

#define	VX_NEFREE	32	/* number of entries in free extent array */

/*
 * root and lost+found inode numbers
 */

#define	VX_ROOTINO	((ino_t)2)	/* i number of all roots */
#define	LOSTFOUNDINO	(VX_ROOTINO + 1)

/*
 * Magic Number and Version
 */

#define	VX_MAGIC	0xa501fcf5
#define	VX_VERSION	1
#define	VX_AUMAGIC	0xa502fcf5

#define	SVX_MAGIC	VX_MAGIC
#define	SVX_VERSION	2
#define	SVX_AUMAGIC	VX_AUMAGIC

#define	VX_SNAPMAGIC	0xa501fcf6
#define	VX_SNAPVERSION	1

/*
 *  values for fs_flags *
 */

#define	VX_FLAGSMASK	0x1f		/* mask for all values */
#define	VX_FULLFSCK	0x01		/* full fsck required */
#define	VX_LOGBAD	0x02		/* log is invalid, don't do replay */
#define	VX_NOLOG	0x04		/* no logging, don't do replay */
#define	VX_RESIZE	0x08		/* resize in progress */
#define	VX_LOGRESET	0x10		/* log reset desired */

/*
 * values for fs_clean
 */

#define	VX_CLEAN	0x5a		/* file system is clean */
#define	VX_DIRTY	0x3c		/* file system is active */

/*
 * Maximum levels in the freeze locking hierarchy (plus 1 for zero base)
 */

#define	VX_ACTIVE_LEVELS	(8 + 1)

/*
 * File system structure. The first portion of this structure is
 * the super-block contents on disk. The tail portion is only
 * used in memory. The disk portions have the offset in the super-block
 * as part of the  comment.
 */

struct fscommon {

	/*
	 * validation and dating info
	 */

	long	fsc_magic;		/* 0x00: magic number */
	long	fsc_version;		/* 0x04: version number */
	time_t	fsc_ctime;		/* 0x08: create time */
	time_t	fsc_ectime;		/* 0x0c: spare for eft */

	/*
	 * fundamental sizes and offsets
	 */

	long	fsc_obsolete1;		/* 0x10: currently unused */
	long	fsc_obsolete2;		/* 0x14: currently unused */
	daddr_t	fsc_logstart;		/* 0x18: addr of first log block */
	daddr_t	fsc_logend;		/* 0x1c: addr of last log block */
	long	fsc_bsize;		/* 0x20: size of blocks in fs */
	long	fsc_size;		/* 0x24: number of blocks in fs */
	long	fsc_dsize;		/* 0x28: number of data blocks in fs */
	long	fsc_ninode;		/* 0x2c: number of inodes in fs */
	long	fsc_nau;		/* 0x30: number of au's in fs */
	long	fsc_reserved1;		/* 0x34: reserved for future use */
	long	fsc_defiextsize;	/* 0x38: default indirect ext size */
	long	fsc_ilbsize;		/* 0x3c: ilist block size in bytes */
	long	fsc_immedlen;		/* 0x40: size of immediate data area */
	long	fsc_ndaddr;		/* 0x44: num of dir exts per inode */

	/*
	 * derived offsets
	 */

	daddr_t	fsc_aufirst;		/* 0x48: addr of first au */
	daddr_t	fsc_emap;		/* 0x4c: offset of extent map in au */
	daddr_t	fsc_imap;		/* 0x50: offset of inode map in au */
	daddr_t	fsc_iextop;		/* 0x54: offset of extop map in au */
	daddr_t	fsc_istart;		/* 0x58: offset of inode list in au */
	daddr_t	fsc_bstart;		/* 0x5c: offset of data block in au*/
	daddr_t	fsc_femap;		/* 0x60: fs_aufirst + fs_emap */
	daddr_t	fsc_fimap;		/* 0x64: fs_aufirst + fs_imap */
	daddr_t	fsc_fiextop;		/* 0x68: fs_aufirst + fs_iextop */
	daddr_t	fsc_fistart;		/* 0x6c: fs_aufirst + fs_istart */
	daddr_t	fsc_fbstart;		/* 0x70: fs_aufirst + fs_bstart */

	/*
	 * derived lengths and sizes
	 */

	long	fsc_nindir;		/* 0x74: number of entries in indir */
	long	fsc_aulen;		/* 0x78: length of au in blocks */
	long	fsc_auimlen;		/* 0x7c: length of au imap in blocks */
	long	fsc_auemlen;		/* 0x80: length of au emap in blocks */
	long	fsc_auilen;		/* 0x84: length of au ilist in blocks */
 	long	fsc_aupad;		/* 0x88: length of au pad in blocks */
 	long	fsc_aublocks;		/* 0x8c: number data blocks in an au */
 	long	fsc_maxtier;		/* 0x90: log base 2 of fs_aublocks */
	long	fsc_inopb;		/* 0x94: number of inodes per block */
	long	fsc_inopau;		/* 0x98: number of inodes per au */
	long	fsc_inopilb;		/* 0x9c: inodes per ilist block */
	long	fsc_ndiripau;		/* 0xa0: num of directory ino per au */
	long	fsc_iaddrlen;		/* 0xa4: sz of ind addr ext in blks */

	/*
	 * derived shift values
	 */

	long	fsc_bshift;		/* 0xa8: log base 2 of fs_bsize */
	long	fsc_inoshift;		/* 0xac: log base 2 of fs_inopb */

	/*
	 * derived masks
	 */

	ulong	fsc_bmask;		/* 0xb0: ~(fs_bsize - 1) */
	ulong	fsc_boffmask;		/* 0xb4: fs_bsize - 1 */
	long	fsc_inomask;		/* 0xb8: fs_inopilb - 1 */

	/*
	 *  Checksum of all invariant fields (see FS_CHECKSUM)
	 */

	long	fsc_checksum;		/* 0xbc: checksum */
};

/*
 * writable portion of super-block
 */

struct fswrite {

	/*
	 * free resources
	 */

	long	fsw_free;		/* 0x00: number of free blocks */
	long	fsw_ifree;		/* 0x04: number of free inodes */
	long	fsw_efree[VX_NEFREE];	/* 0x08: num. of free extents by size */

	/*
	 * modification and state flags
	 */

	long	fsw_flags;		/* 0x88: flags */
	char	fsw_mod;		/* 0x8c: file system has been changed */
	char	fsw_clean;		/* 0x8d: file system is clean */
	short	fsw_reserved2;		/* 0x8e: reserved space */
	ulong	fsw_firstlogid;		/* 0x90: mount time log id */

#ifdef	_KERNEL
	timestruc_t	fsw_time;	/* 0x94: time last written */
#else
	time_t	fsw_time;		/* 0x94: time last written */
	long	fsw_etime;		/* 0x98: spare for eft */
#endif

	/*
	 *  labels
	 */

	char	fsw_fname[6];		/* 0x9c: file system name */
	char	fsw_fpack[6];		/* 0xa2: file system pack name */
};

/*
 *  Memory only super-block fields
 */

struct fsmem {
	long	fsm_ltop;		/* 0x00: logical to physical shift */
	ulong	fsm_logid;		/* 0x04: log id */
	int	fsm_logactive;		/* 0x08: log is being written */
	daddr_t	fsm_logbegin;		/* 0x0c: beginning of log */
	int	fsm_lbdonelock;		/* 0x10: log buffer done lock */
	int	fsm_lbleft;		/* 0x14: space left in log buffer */
	caddr_t	fsm_flogbufp;		/* 0x18: buffer for first log block */ 
	int	fsm_sblock;		/* 0x1c: super-block lock */
	int	fsm_dirlock;		/* 0x20: directory add or remove lock */
	struct vx_tran	*fsm_tdoneq;	/* 0x24: completed transaction queue */
	int	fsm_logbad;		/* 0x28: log bad flag */
	struct vx_tran	*fsm_acttranq;	/* 0x2c: active trans queue head */
	int	fsm_tranmax;		/* 0x30: max transaction reservation */
	int	fsm_tranleft;		/* 0x34: available reservation */
	struct	vx_map *fsm_emaplist;	/* 0x38: list of extent maps */
	struct	vx_map *fsm_imaplist;	/* 0x3c: list of inode maps */
	struct	vx_map *fsm_iemaplist;	/* 0x40: list of extended op maps */
	struct	vx_ausum *fsm_ausum;	/* 0x44: au summary area */
	char	fsm_rdonly;		/* 0x48: mounted readonly */
	char	fsm_busy;		/* 0x49: fs busy since last sync */
	char	fsm_disabled;		/* 0x4a: file system disabled */
	char	fsm_blkclear;		/* 0x4b: guarantee cleared storage */
	char	fsm_delaylog;		/* 0x4c: ufs semantics */
	char	fsm_tmplog;		/* 0x4d: episode semantics */
	char	fsm_logwrites;		/* 0x4e: log synchronous writes */
	char	fsm_unused1;		/* 0x4f: unused */
	int	fsm_mincache;		/* 0x50: mount caching flags */
	int	fsm_convosync;		/* 0x54: mount convosync flags */
	int	fsm_freeze;		/* 0x58: freeze lock flags */
	int	fsm_active[VX_ACTIVE_LEVELS];	/* 0x5c: procs at each level */
	int	fsm_freezelock;		/* 0x80: level freeze has locked */
	int	fsm_freezewait;		/* 0x84: level freeze is waiting for */
	long	fsm_minfree;		/* 0x88: minimum desired free blocks */
	long	fsm_aufree[5];		/* 0x8c: low desired free per au */
	long	fsm_lastaufree[5];	/* 0xa0: low desired free in last au */
	caddr_t	fsm_mntpt;		/* 0xb4: mount point of file system */
	int	fsm_mntlen;		/* 0xb8: len of mntpt pathname */
	int	fsm_iosize;		/* 0xbc: device sector size */
	int	fsm_ioshift;		/* 0xc0: log base 2 of fs_iosize */
	int	fsm_iomask;		/* 0xc4: fs_iosize - 1 */
	int	fsm_sblkno;		/* 0xc8: block offset of super-block */
	int	fsm_sblklen;		/* 0xcc: len in blocks of super-block */
	int	fsm_sblkoff;		/* 0xd0: offset in buf of super-block */
	int	fsm_flogactive;		/* 0xd4: first log block active */
	int	fsm_ntran;		/* 0xd8: number of transactions on fs */
	int	fsm_replaytran;		/* 0xdc: trans for replay if crash */
	u_long	fsm_logoff;		/* 0xe0: absolute position of log */
	int	fsm_tranlow;		/* 0xe4: flush point in log */
	int	fsm_tranmed;		/* 0xe8: flush point in log */
	struct fs *fsm_snapfs;		/* 0xec: list of snapshots */
	struct vx_snap *fsm_snapp;	/* 0xf0: pointer to snap structure */
	struct buf *fsm_copybp;		/* 0xf4: snapshot copy buffer */
	int	fsm_copies;		/* 0xf8: count of active data copies */
	int	fsm_copylock;		/* 0xfc: prevent copies for umount */
	int	fsm_blkpsg;		/* 0x100: blocks per segment */
	int	fsm_blkppg;		/* 0x104: blocks per page */
	int	fsm_timeout;		/* 0x108: timeout id for freeze */
	struct vx_logbuf *fsm_actlb;	/* 0x10c: active log buffers */
	struct vx_logbuf *fsm_curlb;	/* 0x110: current log buffer */
	struct vnode *fsm_devvp;	/* 0x114: block device vnode for I/O */
	struct buf *fsm_superbuf;	/* 0x118: buffer for super-block */
	long	fsm_dflags;		/* 0x11c: flags confirmed on disk */
	struct vx_logbuf *fsm_lactlb;	/* 0x120: tail of active log buffers */
	dev_t	fsm_dev;		/* 0x124: device number fs on */
	struct vfs *fsm_vfsp;		/* 0x128: point to vfs */
	struct vx_mlink	fsm_mlink;	/* 0x12c: head of async update queue */
					/* 0x15c: end */
};

#ifdef	_KERNEL

struct fsnocopy {
	int	fsn_filler;		/* 0x000: unused */
};

#endif	/*_KERNEL*/

struct fs {
	struct fscommon fs_c;		/* 0x000: fixed portion */
	struct fswrite	fs_w;		/* 0x0c0: writable portion */
	struct fsmem	fs_m;		/* 0x168: memory only portion */
#ifdef	_KERNEL
	struct fsnocopy	fs_n;		/* 0x2c4: non freeze safe portion */
#endif	/*_KERNEL*/
};

#define	fs_magic	fs_c.fsc_magic
#define	fs_version	fs_c.fsc_version
#define	fs_ctime	fs_c.fsc_ctime
#define	fs_ectime	fs_c.fsc_ectime
#define	fs_obsolete1	fs_c.fsc_obsolete1
#define	fs_obsolete2	fs_c.fsc_obsolete2
#define	fs_logstart	fs_c.fsc_logstart
#define	fs_logend	fs_c.fsc_logend
#define	fs_bsize	fs_c.fsc_bsize
#define	fs_size		fs_c.fsc_size
#define	fs_dsize	fs_c.fsc_dsize
#define	fs_ninode	fs_c.fsc_ninode
#define	fs_nau		fs_c.fsc_nau
#define	fs_reserved1	fs_c.fsc_reserved1
#define	fs_defiextsize	fs_c.fsc_defiextsize
#define	fs_ilbsize	fs_c.fsc_ilbsize
#define	fs_immedlen	fs_c.fsc_immedlen
#define	fs_ndaddr	fs_c.fsc_ndaddr
#define	fs_aufirst	fs_c.fsc_aufirst
#define	fs_emap		fs_c.fsc_emap
#define	fs_imap		fs_c.fsc_imap
#define	fs_iextop	fs_c.fsc_iextop
#define	fs_istart	fs_c.fsc_istart
#define	fs_bstart	fs_c.fsc_bstart
#define	fs_femap	fs_c.fsc_femap
#define	fs_fimap	fs_c.fsc_fimap
#define	fs_fiextop	fs_c.fsc_fiextop
#define	fs_fistart	fs_c.fsc_fistart
#define	fs_fbstart	fs_c.fsc_fbstart
#define	fs_nindir	fs_c.fsc_nindir
#define	fs_aulen	fs_c.fsc_aulen
#define	fs_auimlen	fs_c.fsc_auimlen
#define	fs_auemlen	fs_c.fsc_auemlen
#define	fs_auilen	fs_c.fsc_auilen
#define	fs_aupad	fs_c.fsc_aupad
#define	fs_aublocks	fs_c.fsc_aublocks
#define	fs_maxtier	fs_c.fsc_maxtier
#define	fs_inopb	fs_c.fsc_inopb
#define	fs_inopau	fs_c.fsc_inopau
#define	fs_inopilb	fs_c.fsc_inopilb
#define	fs_ndiripau	fs_c.fsc_ndiripau
#define	fs_iaddrlen	fs_c.fsc_iaddrlen
#define	fs_bshift	fs_c.fsc_bshift
#define	fs_inoshift	fs_c.fsc_inoshift
#define	fs_bmask	fs_c.fsc_bmask
#define	fs_boffmask	fs_c.fsc_boffmask
#define	fs_inomask	fs_c.fsc_inomask
#define	fs_checksum	fs_c.fsc_checksum
#define	fs_free		fs_w.fsw_free
#define	fs_ifree	fs_w.fsw_ifree
#define	fs_efree	fs_w.fsw_efree
#define	fs_flags	fs_w.fsw_flags
#define	fs_mod		fs_w.fsw_mod
#define	fs_clean	fs_w.fsw_clean
#define	fs_firstlogid	fs_w.fsw_firstlogid
#define	fs_time		fs_w.fsw_time
#ifndef _KERNEL
#define	fs_etime	fs_w.fsw_etime
#endif
#define	fs_fpack	fs_w.fsw_fpack
#define	fs_fname	fs_w.fsw_fname
#define	fs_ltop		fs_m.fsm_ltop
#define	fs_logid	fs_m.fsm_logid
#define	fs_logactive	fs_m.fsm_logactive
#define	fs_logbegin	fs_m.fsm_logbegin
#define	fs_lbdonelock	fs_m.fsm_lbdonelock
#define	fs_lbleft	fs_m.fsm_lbleft
#define	fs_flogbufp	fs_m.fsm_flogbufp
#define	fs_sblock	fs_m.fsm_sblock
#define	fs_dirlock	fs_m.fsm_dirlock
#define	fs_acttranq	fs_m.fsm_acttranq
#define	fs_logbad	fs_m.fsm_logbad
#define	fs_tdoneq	fs_m.fsm_tdoneq
#define	fs_tranmax	fs_m.fsm_tranmax
#define	fs_tranleft	fs_m.fsm_tranleft
#define	fs_emaplist	fs_m.fsm_emaplist
#define	fs_imaplist	fs_m.fsm_imaplist
#define	fs_iemaplist	fs_m.fsm_iemaplist
#define	fs_ausum	fs_m.fsm_ausum
#define	fs_rdonly	fs_m.fsm_rdonly
#define	fs_busy		fs_m.fsm_busy
#define	fs_disabled	fs_m.fsm_disabled
#define	fs_blkclear	fs_m.fsm_blkclear
#define	fs_freeze	fs_m.fsm_freeze
#define	fs_active	fs_m.fsm_active
#define	fs_freezelock	fs_m.fsm_freezelock
#define	fs_freezewait	fs_m.fsm_freezewait
#define	fs_dev		fs_m.fsm_dev
#define	fs_vfsp		fs_m.fsm_vfsp
#define	fs_blkpsg	fs_m.fsm_blkpsg
#define	fs_blkppg	fs_m.fsm_blkppg
#define	fs_timeout	fs_m.fsm_timeout
#define	fs_delaylog	fs_m.fsm_delaylog
#define	fs_tmplog	fs_m.fsm_tmplog
#define	fs_dflags	fs_m.fsm_dflags
#define	fs_minfree	fs_m.fsm_minfree
#define	fs_aufree	fs_m.fsm_aufree
#define	fs_lastaufree	fs_m.fsm_lastaufree
#define	fs_mntpt	fs_m.fsm_mntpt
#define	fs_mntlen	fs_m.fsm_mntlen
#define	fs_iosize	fs_m.fsm_iosize
#define	fs_ioshift	fs_m.fsm_ioshift
#define	fs_iomask	fs_m.fsm_iomask
#define	fs_sblkno	fs_m.fsm_sblkno
#define	fs_sblklen	fs_m.fsm_sblklen
#define	fs_sblkoff	fs_m.fsm_sblkoff
#define	fs_logwrites	fs_m.fsm_logwrites
#define	fs_flogactive	fs_m.fsm_flogactive
#define	fs_ntran	fs_m.fsm_ntran
#define	fs_replaytran	fs_m.fsm_replaytran
#define	fs_logoff	fs_m.fsm_logoff
#define	fs_tranlow	fs_m.fsm_tranlow
#define	fs_tranmed	fs_m.fsm_tranmed
#define	fs_snapfs	fs_m.fsm_snapfs
#define	fs_snapp	fs_m.fsm_snapp
#define	fs_copybp	fs_m.fsm_copybp
#define	fs_copies	fs_m.fsm_copies
#define	fs_copylock	fs_m.fsm_copylock
#define	fs_actlb	fs_m.fsm_actlb
#define	fs_curlb	fs_m.fsm_curlb
#define	fs_devvp	fs_m.fsm_devvp
#define	fs_superbuf	fs_m.fsm_superbuf
#define	fs_lactlb	fs_m.fsm_lactlb
#define	fs_mincache	fs_m.fsm_mincache
#define	fs_convosync	fs_m.fsm_convosync
#define	fs_mlink	fs_m.fsm_mlink

/*
 * allocation unit header
 */

struct auheader {
	long	au_magic;		/* 0x00: magic number */
	long	au_aun;			/* 0x04: nth au */
	struct	fscommon au_auxsb;	/* 0x08: copy of super-block */
};

/*
 * allocation unit summaries
 */

struct vx_iemapsum {		/* iemap summary */
	long	ausd_iextop;	/* count of iextops enabled */
};

struct vx_imapsum {		/* imap summary */
	long	ausd_difree;	/* free directory inodes */
	long	ausd_dibfree;	/* free directory inode blocks */
	long	ausd_rifree;	/* free regular file inodes */
	long	ausd_ribfree;	/* free regular file inode blocks */
};

struct vx_emapsum {		/* emap summary */
	long	ausd_efree[VX_NEFREE];	/* free extents by size */
};

/*
 * disk format of ausum info
 */

struct vx_ausumd {
	struct vx_iemapsum	ausd_iesum;
	struct vx_imapsum	ausd_isum;
	struct vx_emapsum	ausd_esum;
};

/*
 * The in-memory au summary.
 */
struct	vx_ausum {
	struct vx_ausumd 	aus_cur;	/* current summaries */
	int			aus_flag;	/* flags */
	long			aus_free;	/* free blocks in au */
	long			aus_lastextop;	/* aus_extop at last write */
	ino_t			aus_lastino;	/* last inode allocated */
};

#define	aus_iextop	aus_cur.ausd_iesum.ausd_iextop
#define	aus_difree	aus_cur.ausd_isum.ausd_difree
#define	aus_dibfree	aus_cur.ausd_isum.ausd_dibfree
#define	aus_rifree	aus_cur.ausd_isum.ausd_rifree
#define	aus_ribfree	aus_cur.ausd_isum.ausd_ribfree
#define	aus_efree	aus_cur.ausd_esum.ausd_efree

#define	ausumd_iextop	ausd_iesum.ausd_iextop
#define	ausumd_difree	ausd_isum.ausd_difree
#define	ausumd_dibfree	ausd_isum.ausd_dibfree
#define	ausumd_rifree	ausd_isum.ausd_rifree
#define	ausumd_ribfree	ausd_isum.ausd_ribfree
#define	ausumd_efree	ausd_esum.ausd_efree

#define	VX_AUBADSUM	0x1
#define	VX_AUDIRTYSUM	0x2

/*
 * calculate check sum of invariant super-block fields
 */

#define	VX_FSCHECKSUM(fs)	( \
	(fs)->fs_magic + \
	(fs)->fs_version + \
	(fs)->fs_ctime + \
	(fs)->fs_ectime + \
	(fs)->fs_obsolete1 + \
	(fs)->fs_obsolete2 + \
	(fs)->fs_logstart + \
	(fs)->fs_logend + \
	(fs)->fs_bsize + \
	(fs)->fs_size + \
	(fs)->fs_dsize + \
	(fs)->fs_ninode + \
	(fs)->fs_nau + \
	(fs)->fs_reserved1 + \
	(fs)->fs_defiextsize + \
	(fs)->fs_ilbsize + \
	(fs)->fs_immedlen + \
	(fs)->fs_ndaddr + \
	(fs)->fs_aufirst + \
	(fs)->fs_emap + \
	(fs)->fs_imap + \
	(fs)->fs_iextop + \
	(fs)->fs_istart + \
	(fs)->fs_bstart + \
	(fs)->fs_femap + \
	(fs)->fs_fimap + \
	(fs)->fs_fiextop + \
	(fs)->fs_fistart + \
	(fs)->fs_fbstart + \
	(fs)->fs_nindir + \
	(fs)->fs_aulen + \
	(fs)->fs_auimlen + \
	(fs)->fs_auemlen + \
	(fs)->fs_auilen + \
	(fs)->fs_aupad + \
	(fs)->fs_aublocks + \
	(fs)->fs_maxtier + \
	(fs)->fs_inopb + \
	(fs)->fs_inopau + \
	(fs)->fs_inopilb + \
	(fs)->fs_ndiripau + \
	(fs)->fs_iaddrlen + \
	(fs)->fs_bshift + \
	(fs)->fs_inoshift + \
	(fs)->fs_bmask + \
	(fs)->fs_boffmask + \
	(fs)->fs_inomask)

/*
 * Snapshot structure used to control snapshot file systems.
 */

struct vx_snap {
	struct fs	*sh_snapfs;	/* 0x0 snapshot file system */
	struct fs	*sh_primaryfs;	/* 0x4 primary file system */
	char		*sh_bitmap;	/* 0x8 bitmap of copied blocks */
	daddr_t		sh_blkmap_bno;	/* 0xc start of block map */
	int		sh_blkmap_bad;	/* 0x10 set if i/o error on block map */
	struct buf	*sh_resbp;	/* 0x14 reserve buffer for block map */
	struct buf	*sh_curbp;	/* 0c18 current, locked, block map bp */
	daddr_t		sh_nextblk;	/* 0x1c next block to be allocated */
	daddr_t		sh_maxblk;	/* 0x20 max blocks in snapshot area */
	int		sh_flags;	/* 0x24 sundry flags */
};

/*
 * Values for sh_flags
 */

#define	VX_SNLOCKED	0x01		/* snap struct and blkmap are locked */
#define	VX_SNWANT	0x02		/* snap lock is wanted */
#define	VX_SNBLKWAIT	0x04		/* waiting for block copy to complete */

/*
 * conversion from logical to physical blocks
 */

#define	FsLTOP(fs, b)	((b) << (fs)->fs_ltop)
#define	FsPTOL(fs, b)	((b) >> (fs)->fs_ltop)

/*
 * conversion from logical or physical to I/O size blocks
 */

#define	FsLTOIO(fs, b)	((b) << ((fs)->fs_bshift - (fs)->fs_ioshift))
#define	FsIOTOL(fs, b)	((b) >> ((fs)->fs_bshift - (fs)->fs_ioshift))

#define	FsPTOIO(b)	((b) >> ((fs)->fs_ioshift - DEV_BSHIFT))
#define	FsIOTOP(b)	((b) << ((fs)->fs_ioshift - DEV_BSHIFT))

/*
 * Superblock size and location
 */

#define	VX_SUPERBOFF	1024
#define	VX_SBDISKSIZE	(sizeof (struct fscommon) + sizeof (struct fswrite))
#define	VX_SBCOPYSIZE	(sizeof (struct fscommon) + \
			 sizeof (struct fswrite) + \
			 sizeof (struct fsmem))

/*
 * conversion to aunumber from block number
 */

#define	VX_DTOAU(fs, b) (((b) - (fs)->fs_aufirst) / (fs)->fs_aulen)

/*
 * Maximum allowed file offset
 */

#define	VX_MAXOFF	INT_MAX

/*
 * Maximum allowed block in a file system
 */

#define	VX_MAXBLK(fs)	(((unsigned) VX_MAXOFF + (fs)->fs_boffmask) >> \
			  (fs)->fs_bshift)

/*
 * flag values for freeze locks
 */

#define	VX_LOCK_INTERRUPT	0x1
#define	VX_FREEZE_TIMEOUT	0x2
#define	VX_FREEZING		0x4
#define	VX_FROZEN		0x8

#ifdef	_KERNEL

/*
 * active/inactive interface crossing macros
 */

#define VX_ACTIVE1(fs) { \
	while ((fs)->fs_freezelock >= 1) { \
		sleep((caddr_t)&(fs)->fs_freeze, PINOD); \
	} \
	(fs)->fs_active[1]++; \
	(fs)->fs_busy = 1; \
	XTED_ACTIVE1(fs); \
}

#define VX_INACTIVE1(fs) { \
	if (lbolt >= vx_flushlbolt && vx_flushing == 0) { \
		vx_delxwri_sync(); \
	} \
	XTED_INACTIVE1(fs); \
	if (--(fs)->fs_active[1] == 0) { \
		if ((fs)->fs_freezewait == 1) { \
			wakeup((caddr_t)(fs)->fs_active); \
		} \
	} \
}

#define VX_ACTIVE_COMMON(fs, level) { \
	while ((fs)->fs_freezelock >= (level)) { \
		sleep((caddr_t)&(fs)->fs_freeze, PINOD); \
	} \
	(fs)->fs_active[(level)]++; \
	XTED_ACTIVE_COMMON((fs), (level)); \
}

#define VX_INACTIVE_COMMON(fs, level) { \
	XTED_INACTIVE_COMMON((fs), (level)); \
	if (--(fs)->fs_active[(level)] == 0) { \
		if ((fs)->fs_freezewait == (level)) { \
			wakeup((caddr_t)(fs)->fs_active); \
		} \
	} \
}

#define VX_ACTIVE2(fs)		VX_ACTIVE_COMMON(fs, 2);
#define VX_ACTIVE3(fs)		VX_ACTIVE_COMMON(fs, 3);
#define VX_ACTIVE4(fs)		VX_ACTIVE_COMMON(fs, 4);
#define VX_ACTIVE5(fs)		VX_ACTIVE_COMMON(fs, 5);
#define VX_ACTIVE6(fs)		VX_ACTIVE_COMMON(fs, 6);
#define VX_ACTIVE7(fs)		VX_ACTIVE_COMMON(fs, 7);
#define VX_ACTIVE8(fs)		VX_ACTIVE_COMMON(fs, 8);

#define VX_INACTIVE2(fs)	VX_INACTIVE_COMMON(fs, 2);
#define VX_INACTIVE3(fs)	VX_INACTIVE_COMMON(fs, 3);
#define VX_INACTIVE4(fs)	VX_INACTIVE_COMMON(fs, 4);
#define VX_INACTIVE5(fs)	VX_INACTIVE_COMMON(fs, 5);
#define VX_INACTIVE6(fs)	VX_INACTIVE_COMMON(fs, 6);
#define VX_INACTIVE7(fs)	VX_INACTIVE_COMMON(fs, 7);
#define VX_INACTIVE8(fs)	VX_INACTIVE_COMMON(fs, 8);

/*
 * directory add or delete inhibit
 */

#define	VX_DIRLOCK(fs) { \
	while ((fs)->fs_dirlock) { \
		(void)sleep((caddr_t)&((fs)->fs_dirlock), PINOD); \
	} \
	(fs)->fs_dirlock = 1; \
	XTED_DIRLOCK(fs); \
} 

#define	VX_DIRUNLOCK(fs) { \
	XTED_DIRUNLOCK(fs); \
	(fs)->fs_dirlock = 0; \
	wakeup((caddr_t)&((fs)->fs_dirlock)); \
} 

/*
 * lock super-block
 */

#define	VX_SBLOCK(fs) { \
	XTED_SBLOCK1(fs); \
	while ((fs)->fs_sblock) { \
		(void)sleep((caddr_t)&((fs)->fs_sblock), PINOD); \
	} \
	(fs)->fs_sblock = 1; \
	XTED_SBLOCK2(fs); \
}

#define	VX_SBUNLOCK(fs) { \
	XTED_SBUNLOCK(fs); \
	(fs)->fs_sblock = 0; \
	wakeup((caddr_t)&((fs)->fs_sblock)); \
}

/*
 * Locks for the transaction queues.
 */

#ifndef	TED_
#define	VX_FSQ_LOCK(fs, splfunc)	(splfunc)()
#else									/*TED_*/
#define	VX_FSQ_LOCK(fs, splfunc)	xted_fsq_lock(fs, splfunc)	/*TED_*/
#endif									/*TED_*/
#define	VX_FSQ_UNLOCK(fs, pri) {					\
	XTED_FSQ_UNLOCK(fs);					/*TED_*/\
	(void)splx(pri);						\
}

#ifndef	TED_
#define	VX_TRANQ_LOCK(fs, splfunc)	(splfunc)()
#else									/*TED_*/
#define	VX_TRANQ_LOCK(fs, splfunc)	xted_tranq_lock(fs, splfunc)	/*TED_*/
#endif									/*TED_*/
#define	VX_TRANQ_UNLOCK(fs, pri) {					\
	XTED_TRANQ_UNLOCK(fs);					/*TED_*/\
	(void)splx(pri);						\
}

#ifndef	TED_
#define	VX_FSQ_TRANQ_SWITCH(fs, pri, splfunc)
#define	VX_TRANQ_FSQ_SWITCH(fs, pri, splfunc)
#else									/*TED_*/
#define	VX_FSQ_TRANQ_SWITCH(fs, pri, splfunc) {			/*TED_*/\
	XTED_FSQ_UNLOCK(fs);					/*TED_*/\
	(void)splx(pri);					/*TED_*/\
	xted_tranq_lock(fs, splfunc);				/*TED_*/\
}									/*TED_*/
#define	VX_TRANQ_FSQ_SWITCH(fs, pri, splfunc) {			/*TED_*/\
	XTED_TRANQ_UNLOCK(fs);					/*TED_*/\
	(void)splx(pri);					/*TED_*/\
	xted_fsq_lock(fs, splfunc);				/*TED_*/\
}									/*TED_*/
#endif									/*TED_*/

/*
 * Update vxfs time
 */

#define	VX_TIME(timep) { \
	if (hrestime.tv_sec != vx_time.tv_sec || \
	    hrestime.tv_nsec > vx_time.tv_nsec) { \
		vx_time = hrestime; \
	} else { \
		vx_time.tv_nsec++; \
	} \
	*timep = vx_time; \
}

/*
 * true if primary file system with active snapshots
 */

#define	VX_SNAPPED(fs)	((fs)->fs_snapfs && !(fs)->fs_snapp)

/*
 * true if snapshot file system
 */

#define	VX_SNAPSHOT(fs)	((fs)->fs_snapp)

/*
 * Strategy routine for almost all i/o
 */

#define	VX_STRATEGY(fs, bp)	vx_snap_strategy((fs), (bp))

/*
 * Copy lock and unlock for snapshot file system, either shared or exclusive.
 */

#define	VX_COPYLOCK(fs, flag)	vx_copylock((fs), (flag))
#define	VX_COPYUNLOCK(fs, flag)	vx_copyunlock((fs), (flag))

/*
 * Flags for VX_COPYLOCK() and VX_COPYUNLOCK().
 * The copy lock is either shared or exclusive.
 */

#define	VX_LOCK_SHARED	1
#define	VX_LOCK_EXCL	2

/*
 * snap structure lock and unlock for snapshot file system
 */

#define	VX_SNAPLOCK(shp)	vx_snaplock(shp)
#define	VX_SNAPUNLOCK(shp)	vx_snapunlock(shp)

/*
 * round x upto the the next boundary (bnd) where bnd is a power of 2.
 */

#define	VX_ROUNDUP(x, bnd)	(((x) + (bnd) - 1) & ~((bnd) - 1))

extern int	vx_bufspace;	/* total Kbytes held in map buffers */
extern int	vx_maxbufspace;	/* max kbytes allowed to be held by maps */
extern int	vx_bufsleep;	/* flag for sleeping on bufs in vx_traninit */
extern int	vx_fstype;	/* file system type; set at boot time */
extern int	vx_replay_low;	/* low log contention flush point */
extern int	vx_replay_med;	/* medium log contention flush point */

extern timestruc_t	vx_time;	/* vxfs idea of time */
extern whymountroot_t	vx_rootstate;	/* mount state of root fs */
extern int		vx_rootmflags;	/* mount flags for root fs */

extern int	vx_flushing;	/* flag for serializing delxwri_sync */
extern clock_t	vx_flushlbolt;	/* time of next delxwri_sync out of inactive1 */
extern clock_t	vx_fsflushlbolt; /* time of next delxwri_sync by fsflush */

#endif	/*_KERNEL*/

/*
 * Structure for log buffer management.
 */

struct vx_logbuf	{
	struct vx_logbuf	*lb_next;	/* next buf in act/free list */
	struct vx_tran		*lb_logq;	/* q of trans logged in buf */
	struct vx_tran		*lb_lastlogq;	/* last entry on lb_logq */
	struct vx_tran		*lb_doneq;	/* q of done records in buf */
	struct fs		*lb_fs;		/* fs that owns buf */
	struct buf		*lb_bp;		/* buffer header */
	caddr_t			lb_addr;	/* buffer space */
	ulong			lb_logoff;	/* position of buffer in log */
	int			lb_done;	/* flag indicating I/O done */
	int			lb_len;		/* length of buffer */
	int			lb_off;		/* current end of buffer */
};

#endif	/*_FS_VXFS_VX_FS_H */
