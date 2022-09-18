/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_tran.h	1.7 16 May 1992 04:40:38 -  */
#ident	"@(#)uts-comm:fs/vxfs/vx_tran.h	1.6"

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

#ifndef	_FS_VXFS_VX_TRAN_H
#define	_FS_VXFS_VX_TRAN_H

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * transaction function ids
 */

#define	VX_IMTRAN	2			/* inode modification */
#define	VX_DATRAN	3			/* add directory entry */
#define	VX_DACTRAN	4			/* add and compress directory */
#define	VX_DRTRAN	5			/* remove directory entry */
#define	VX_IETRAN	7			/* IMAP or IEMAP operation */
#define	VX_EMTRAN	8			/* extent map */
#define	VX_TRTRAN	10			/* indirect truncation */
#define	VX_WRTRAN	11			/* obsolete */
#define	VX_WLTRAN	12			/* write a symlink */
#define	VX_PUTTRAN	13			/* obsolete */
#define	VX_TATRAN	15			/* indirect allocation */
#define	VX_LWRTRAN	16			/* logged write */
#define	VX_LWRDATA	17			/* logged write data */
#define	VX_LWRDONE	18			/* logged writes done */
#define	VX_UNDO		101			/* done/undo a transaction */
#define VX_NULLTRAN	102			/* null (space filler) */

/*
 * transaction common area
 */

struct vx_ctran {
	char		*t_logp;		/* 0x00 pointer to log area */
	int		t_loglen;		/* 0x04 log size */
	int		t_bflags;		/* 0x08 flags */
	struct buf	*t_bp;			/* 0x0c buffer pointer */
	short		t_func;			/* 0x10 function type */
	short		t_flags;		/* 0x12 flags */
	struct vx_mlink	*t_mlink;		/* 0x14 pointer to mlink */
	struct inode	*t_ip;			/* 0x18 inode pointer */
	union vx_subfunc {			/* 0x1c tran specific data */
		caddr_t			addr;
		struct vx_ietran	*ietran;
		struct vx_imtran	*imtran;
		struct vx_datran	*datran;
		struct vx_drtran	*drtran;
		struct vx_daclog	*daclog;
		struct vx_emtran	*emtran;
		struct vx_trtran	*trtran;
		struct vx_tatran	*tatran;
 		struct vx_lwrtran	*lwrtran;
 		struct vx_wltran	*wltran;
 		struct vx_duclog	*duclog;
	} t_spec;
};

#define	T_ALLOCATED		0x01	/* buffer for newly allocated extent */
#define	T_SYNC			0x02	/* write buffer at commit time */

/*
 * inode map update subfunction
 */

struct vx_ietran {
	struct vx_ielog {
		int	iel_maptype;	/* IMAP or IEMAP */
		int	iel_ino;	/* inode number */
		int	iel_op;		/* set or clear */
		char	iel_old;	/* previous map values */
	} ie_log;
	long		ie_aun;			/* au number */
	int		ie_flag;		/* flag for bad ialloc */
};

#define	ie_maptype	ie_log.iel_maptype
#define	ie_ino		ie_log.iel_ino
#define	ie_old		ie_log.iel_old
#define	ie_op		ie_log.iel_op

/*
 * ie_op values
 */

#define	VX_IASET	1			/* bit set */
#define	VX_IACLR	2			/* bit clear */

/*
 * inode modification transaction
 */

struct vx_imtran {
	struct vx_imlog {
		struct icommon log_oldic;	/* old inode common area */
		struct icommon log_newic;	/* new inode common area */
		int	log_ino;		/* inode number */
	} im_log;
	struct vx_imtran *im_next;	/* next imtran subfunction */
	struct inode	*im_ip;		/* inode pointer */
	struct vx_ctran	*im_tcp;	/* ctran for this transaction */
	int		im_oldiflag;	/* flags from inode */
	off_t		im_nsize;	/* saved value of i_nsize */
	int		im_ibad;	/* inode must be marked bad in undo */
};

#define	im_oldic	im_log.log_oldic
#define	im_newic	im_log.log_newic
#define	im_ino		im_log.log_ino

/*
 * directory entry addition transaction
 * boff is offset in block of old entry
 * if old entry has an ino, then new entry follows old entry
 * otherwise new entry overwrites old entry
 */

struct vx_datran {
	struct vx_dalog {
		daddr_t	log_ino;		/* directory inode */
		daddr_t	log_bno;		/* directory block no */
		off_t	log_boff;		/* offset in block */
		off_t	log_blen;		/* length of block */
		struct mindirect log_old;	/* old directory entry */
		struct direct log_new;		/* new directory entry */
	} da_log;
	struct inode	*da_tdp;		/* target directory */
	struct vx_ctran	*da_side;		/* tcp if shared inode */
	off_t		da_eoff;		/* offset of block in extent */
	off_t		da_elen;		/* length of extent */
	int		da_hashoff;		/* offset of hash chain head */
	caddr_t		da_np;			/* compress block */
	off_t		da_nlen;		/* length of compress block */
};

#define	da_ino	da_log.log_ino
#define	da_bno	da_log.log_bno
#define	da_boff	da_log.log_boff
#define	da_blen	da_log.log_blen
#define	da_old	da_log.log_old
#define	da_new	da_log.log_new

/*
 * directory entry removal transaction
 * boff is offset in block of old entry
 * if boff is not beginning of block, then 
 * otherwise new entry overwrites old entry
 */
struct vx_drtran {
	struct vx_drlog {
		daddr_t	log_ino;		/* directory inode */
		daddr_t	log_bno;		/* directory block no */
		off_t	log_poff;		/* offset in block  of prev */
		off_t	log_roff;		/* offset in block of rem */
		off_t	log_blen;		/* length of block */
		struct mindirect log_prev;	/* previous directory entry */
		struct direct log_rem;		/* removed directory entry */
	} dr_log;
	struct inode	*dr_tdp;		/* target directory */
	struct vx_ctran	*dr_side;		/* tcp if shared inode */
	off_t		dr_eoff;		/* offset of block in extent */
	off_t		dr_elen;		/* length of extent */
	int		dr_hashoff;		/* offset of hash chain head */
	short		dr_hashold;		/* old hash values */
};

#define	dr_ino	dr_log.log_ino
#define	dr_bno	dr_log.log_bno
#define	dr_poff	dr_log.log_poff
#define	dr_roff	dr_log.log_roff
#define	dr_blen	dr_log.log_blen
#define	dr_prev	dr_log.log_prev
#define	dr_rem	dr_log.log_rem

/*
 * if VX_DACTRAN, then the log area is an vx_daclog structure followed by
 * the before an after contents of the entire block
 */

struct vx_daclog {
	daddr_t	log_ino;		/* inode containing extent */
	daddr_t	log_bno;		/* directory block no */
	off_t	log_oldblen;		/* length of pre-image block */
	off_t	log_newblen;		/* length of post-image block */
};

/*
 * Used to log the pathname if the symbolic link doesn't go
 * into immediate data.
 */

struct vx_wltran {
	daddr_t	log_ino;		/* inode containing extent */
	daddr_t	log_bno;		/* symlink block no */
	off_t	log_len;		/* length of extent */
	off_t	log_dlen;		/* length of data */
	char	log_data[4];		/* symlink data */
};

/*
 * Logged write to a file.
 */

struct vx_lwrtran {
	struct vx_lwrlog {
		daddr_t	log_ino;	/* inode being written */
		u_int	log_offset;	/* starting offset of write */
		u_int	log_len;	/* length of write */
	} lwr_log;
	struct inode	*lwr_ip;	/* vnode pointer */
};

#define	lwr_ino		lwr_log.log_ino
#define	lwr_len		lwr_log.log_len
#define	lwr_offset	lwr_log.log_offset

/*
 * Logged writes on a file are done.
 */

struct vx_lwrdonelog {
	daddr_t	log_ino;	/* inode number */
};

/*
 * extent map manipulation
 */

#define	VX_EMMAX	32			/* number of ops in a trans */

struct vx_emtran {
	struct vx_emlog {
		struct vx_ement {
			long	em_op;		/* VX_EMALLOC or VX_EMFREE */
			daddr_t	em_ebno;	/* starting block */
			long	em_elen;	/* extent length */
		} log_ent[VX_EMMAX];
	} em_log;
	struct vx_emtran *em_next;		/* next emtran */
	struct vx_emtran *em_prev;		/* previous emtran */
	long		em_nent;		/* number of entries */
	struct vx_ctran *em_tcp;		/* tcp linkage */
	long		em_aun;			/* au number */
	daddr_t		em_austart;		/* start block of au */
};

#define	VX_EMALLOC	1			/* extent allocation */
#define	VX_EMFREE	2			/* extent free */

/*
 * indirect extent truncation
 */

#define	VX_MAXTRUNC	16			/* maximum truncs per tran */

struct vx_trtran {
	struct vx_trlog {
		long	log_inode;		/* inode number */
		int	log_lvl;		/* indirection level */
		daddr_t	log_abno;		/* bno of address extent */
		struct vx_trent {
			off_t	log_aoff;	/* offset in address extent */
			daddr_t	log_ebno;	/* bno of data extent */
		} log_ent[VX_MAXTRUNC];
	} tr_log;
	struct inode	*tr_ip;		/* inode pointer */
	int		tr_nent;	/* number of extries */
};

#define	tr_inode	tr_log.log_inode
#define	tr_lvl		tr_log.log_lvl
#define	tr_abno		tr_log.log_abno
#define	tr_trent	tr_log.log_ent

/*
 * indirect extent addition
 */

/*
 * The bmap allocator can allocate up to VX_MAXKCONTIG extents in indirects
 * and return them as one extent.  This means the kernel must limit itself
 * to VX_MAXKALLOC allocating bmap calls to avoid overflowing the tatran.
 */

#define	VX_MAXALLOC	64		/* maximum ind ext allocs per tran */
#define	VX_MAXKALLOC	8		/* maximum bmap allocs per tran */
#define	VX_MAXKCONTIG	8		/* VX_MAXALLOC / VX_MAXKALLOC */

struct vx_tatran {
	struct vx_talog {
		int	log_new;	/* set if addr extent to be cleared */
		long	log_inode;	/* inum */
		int	log_lvl;	/* indirection level */
		daddr_t	log_abno;	/* bno of address extent */
		struct vx_taent {
			off_t	log_aind;	/* index in address extent */
			daddr_t	log_ebno;	/* bno of data extent */
		} log_taent[VX_MAXALLOC];
	} ta_log;
	struct inode	*ta_ip;		/* inode pointer */
	int		ta_nblk;	/* number of entries in array */
	int		ta_swap;	/* part of extent swap */
};

#define	ta_new		ta_log.log_new
#define	ta_lvl		ta_log.log_lvl
#define	ta_inode	ta_log.log_inode
#define	ta_abno		ta_log.log_abno
#define	ta_taent	ta_log.log_taent

/*
 * transaction control structure
 * contains a list of subfunctions, log areas
 * and transaction control areas
 */

#define	VX_MAXTRAN	16		/* maximum number of subfunctions */

struct vx_tran {
	struct vx_duclog {		/* done/undo/commit tran data */
		int	duc_logid;	/* 0x00 transaction log id */
		int	duc_err;	/* 0x04 error */
	} t_duclog;
	short		t_logflag;	/* 0x08 logging control flags */
	short		t_nlog;		/* 0x0a number of log entries */
	short		t_reserve;	/* 0x0c log reservation */
	short		t_replaytran;	/* 0x0e log transaction count */
	short		t_ntran;	/* 0x10 number of subfunctions */
	short		t_acount;	/* 0x12 async activity count */
	struct vx_tran	*t_lognext;	/* 0x14 log/done queue linkage */
	struct vx_tran	*t_forw;	/* 0x18 active tran q forward link */
	struct vx_tran	*t_back;	/* 0x1c active tran q back link */
	struct vx_tran	*t_next;	/* 0x20 pointer to next trans */
	u_long		t_logoff;	/* 0x24 offset of tran in log */
	struct vx_imtran *t_imp;	/* 0x28 inode subfunction chain */
	struct vx_ctran	*t_ctran;	/* 0x2c common area pointer */
	caddr_t		*t_pages;	/* 0x30 pointer to array of pages */
	short		t_pgindex;	/* 0x34 index into t_pages */
	short		t_poff;		/* 0x36 offset from start of page */
	struct vx_mlinkhd t_ilink;	/* 0x38 trans/inode linkage */
	struct vx_mlink *t_mlinks[VX_MAXTRAN]; /* 0x40 linkage for async ops */
					/* 0x80 end */
};

#define	t_logid		t_duclog.duc_logid
#define	t_logerr	t_duclog.duc_err

/*
 *  t_logflag values
 */

#define VX_TLOGDONE	0x1		/* log record written */
#define VX_TLOGGED	0x2		/* transaction put into log queue */
#define VX_TLOGDELAY	0x4		/* delayed log write for transaction */
#define VX_TLOGSYNC	0x8		/* must not delay log write */
#define VX_TLOGUNDO	0x10		/* undo of log record written */

/*
 * The vxfs fast memory allocator.  This is used for transactions instead
 * of the kmem_fast_alloc so the memory actually gets released.
 */

struct vx_mem {
	caddr_t	vxm_bufs;		/* linked list of free buffers */
	int	vxm_size;		/* size of buffers on this list */
	int	vxm_highfree;		/* free list highwater mark */
	int	vxm_lowfree;		/* free list lowwater mark */
	int	vxm_nfree;		/* number of buffers on free list */
	int	vxm_allocating;		/* flag for new allocations */
	int	vxm_want;		/* flag for waiting on freelist */
	int	vxm_totallocs;		/* number of allocations */
	int	vxm_totfrees;		/* number of frees */
	int	vxm_alloc;		/* did kmem_alloc */
	int	vxm_free;		/* did kmem_free */
};

#endif	/* _FS_VXFS_VX_TRAN_H */
