/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_BUF_H	/* wrapper symbol for kernel use */
#define _FS_BUF_H	/* subject to change without notice */

#ident	"@(#)uts-x86:fs/buf.h	1.17"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *	Each buffer in the pool is usually doubly linked into 2 lists:
 *	the device with which it is currently associated (always)
 *	and also on a list of blocks available for allocation
 *	for other use (usually).
 *	The latter list is kept in last-used order, and the two
 *	lists are doubly linked to make it easy to remove
 *	a buffer from one list when it was found by
 *	looking through the other.
 *	A buffer is on the available list, and is liable
 *	to be reassigned to another disk block, if and only
 *	if it is not marked BUSY.  When a buffer is busy, the
 *	available-list pointers can be used for other purposes.
 *	Most drivers use the forward ptr as a link in their I/O active queue.
 *	A buffer header contains all the information required to perform I/O.
 *	Most of the routines which manipulate these things are in bio.c.
 */
typedef struct	buf {
	int	b_flags;		/* see defines below */
	struct	buf *b_forw;		/* headed by d_tab of conf.c */
	struct	buf *b_back;		/*  "  */
	struct	buf *av_forw;		/* position on free list, */
	struct	buf *av_back;		/*     if not BUSY*/
	o_dev_t	b_dev;			/* major+minor device name */
	unsigned b_bcount;		/* transfer count */
	union {
	    caddr_t b_addr;		/* low order core address */
	    int	*b_words;		/* words for clearing */
	    daddr_t *b_daddr;		/* disk blocks */
	} b_un;

#define paddr(X)	(paddr_t)(X->b_un.b_addr)

	daddr_t	b_blkno;		/* block # on device */
	char	b_oerror;		/* OLD error field returned after I/O */
	char	b_res;			/* XENIX Compatibility	*/
	ushort	b_cylin;			/* XENIX Compatibility	*/
	unsigned int b_resid;		/* words not transferred after error */
	daddr_t	b_sector;		/* physical sector of disk request */
	clock_t	b_start;		/* request start time */
	struct  proc  *b_proc;		/* process doing physical or swap I/O */
	struct	page  *b_pages;		/* page list for PAGEIO */
	clock_t b_reltime;		/* previous release time */
	/* Begin new stuff */
#define b_actf	av_forw
#define	b_actl	av_back
#define	b_active b_bcount
#define	b_errcnt b_resid
	long	b_bufsize;		/* size of allocated buffer */
	void	(*b_iodone)();		/* function called by iodone */
	struct	vnode *b_vp;		/* vnode associated with block */
	struct 	buf *b_chain;		/* chain together all buffers here */
	int	b_reqcnt;		/* number of I/O request generated */
	int	b_error;		/* expanded error field */
	dev_t	b_edev;			/* expanded dev field */
	char	*b_private;		/* private data structure */
	void	(*b_writestrat)();	/* function called by bwrite */
} buf_t;

extern struct buf bfreelist;		/* head of available list */

/*
 * Bufhd structures used at the head of the hashed buffer queues.
 * We only need three words for these, so this abbreviated
 * definition saves some space.
 */
struct bufhd {
	long	b_flags;		/* see defines below */
	struct	buf *b_forw, *b_back;	/* fwd/bkwd pointer in chain */
};

struct diskhd {
	long b_flags;                   /* not used, needed for consistency */
 	struct  buf *b_forw, *b_back;   /* queue of unit queues */
 	struct  buf *av_forw, *av_back; /* queue of bufs for this unit */
 	long    b_bcount;               /* active flag */
};

struct pfree {				/* for physio buffer freelist */
	int		b_flags;
	struct buf	*av_forw;
};

/*
 *	These flags are kept in b_flags.
 */
#define B_WRITE   0x0000	/* non-read pseudo-flag */
#define B_READ    0x0001	/* read when I/O occurs */
#define B_DONE    0x0002	/* transaction finished */
#define B_ERROR   0x0004	/* transaction aborted */
#define B_BUSY    0x0008	/* not on av_forw/back list */
#define B_PHYS    0x0010	/* Physical IO potentially using UNIBUS map */
#define B_MAP     0x0020	/* This block has the UNIBUS map allocated */
#define B_WANTED  0x0040	/* issue wakeup when BUSY goes off */
#define B_AGE     0x0080	/* delayed write for correct aging */
#define B_ASYNC   0x0100	/* don't wait for I/O completion */
#define B_DELWRI  0x0200	/* delayed write - wait until buffer needed */
#define B_OPEN    0x0400	/* open routine called */
#define B_STALE   0x0800
#define B_VERIFY  0x1000
#define B_FORMAT  0x2000
#define B_S52K	  0x8000	/* 2k buffer flag */
#define B_EXPEDITE 0x4000	/* expedite disk I/O */
#define B_PRIVLG  0xf000	/* privileged operation (internal driver use) */


/* fix these numbers; remove ones we don't need */
#define	B_PAGEIO	0x10000		/* do I/O to pages on bp->p_pages */
#define	B_DONTNEED	0x20000		/* after write, need not be cached */
#define	B_TAPE		0x40000		/* this is a magtape (no bdwrite) */
#define	B_UAREA		0x80000		/* add u-area to a swap operation */
#define	B_REMAPPED	0x100000	/* buffer is kernel addressable */
#define	B_FREE		0x200000	/* free page when done */
#define	B_PGIN		0x400000	/* pagein op, so swap() can count it */
#define	B_CACHE		0x800000	/* did bread find us in the cache ? */
#define	B_INVAL		0x1000000	/* does not contain valid info  */
#define	B_FORCE		0x2000000	/* semi-permanent removal from cache */
#define	B_HEAD		0x4000000	/* a buffer header, not a buffer */
#define	B_NOCACHE	0x8000000	/* don't cache block when released */
#define	B_BAD		0x10000000	/* bad block revectoring in progress */
#define B_KERNBUF	0x20000000	/* buffer is a kernel buffer */
#define B_RAIO		0x80000000	/* raw disk async I/O */

/*
 * Insq/Remq for the buffer hash lists (used by pageio).
 */
#define	bremhash(bp) { \
	(bp)->b_back->b_forw = (bp)->b_forw; \
	(bp)->b_forw->b_back = (bp)->b_back; \
}
#define	binshash(bp, dp) { \
	(bp)->b_forw = (dp)->b_forw; \
	(bp)->b_back = (dp); \
	(dp)->b_forw->b_back = (bp); \
	(dp)->b_forw = (bp); \
}

/*
 * Insq/Remq for the buffer free lists.
 */
#define	bremfree(bp) { \
	(bp)->av_back->av_forw = (bp)->av_forw; \
	(bp)->av_forw->av_back = (bp)->av_back; \
}
#define	binsheadfree(bp, dp) { \
	(dp)->av_forw->av_back = (bp); \
	(bp)->av_forw = (dp)->av_forw; \
	(dp)->av_forw = (bp); \
	(bp)->av_back = (dp); \
}
#define	binstailfree(bp, dp) { \
	(dp)->av_back->av_forw = (bp); \
	(bp)->av_back = (dp)->av_back; \
	(dp)->av_back = (bp); \
	(bp)->av_forw = (dp); \
}

/*
 *	Fast access to buffers in cache by hashing.
 */

#define bhash(d, b)	((struct buf *)&hbuf[((int)d+(int)b)&v.v_hmask])

struct	hbuf {
	int	b_flags;
	struct	buf	*b_forw;
	struct	buf	*b_back;
	int	b_pad;			/* round size to 2^n */
};

/*
 * Reset a buffer after completed I/O so it can be used again.
 */
#define bioreset(bp) \
{\
	(bp)->b_flags &= ~(B_DONE|B_ERROR); \
        if ((bp)->b_flags & B_KERNBUF) \
                (bp)->b_error = 0; \
        (bp)->b_oerror = 0; \
}

/*
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized code.
 */
#define geterror(bp) \
	(((bp)->b_flags & B_ERROR) == 0 ? 0 : \
	 ((bp)->b_error && ((bp)->b_flags & B_KERNBUF)) ? (bp)->b_error : \
	 (bp)->b_oerror ? (bp)->b_oerror : EIO)

/*
 * Unlink a buffer from the available list and mark it busy.
 * (internal interface).  Must be called from priority level 6
 * or higher.
 */
#define notavail(bp) \
{\
	(bp)->av_back->av_forw = (bp)->av_forw;\
	(bp)->av_forw->av_back = (bp)->av_back;\
	(bp)->b_flags |= B_BUSY;\
	bfreelist.b_bcount--;\
}

struct buf	*bread();
struct buf	*breada();
struct buf	*pbread();
struct buf	*blookup();	/* see if a buffer is already in the cache */
void		bwrite();
void		bdwrite();
void		bawrite();
void		btwrite();	/* release a buffer, marking it dirty and
				 * setting a minimum age for it */
void		brelse();
int		incore();
struct buf	*getblk();
struct buf	*ngeteblk();
struct buf	*geteblk();
struct buf	*pgetblk();
void		clrbuf();
void		bflush();
void		blkflush();
void		bdwait();
void		binval();
void		binit();
int		biowait();
struct buf	*pageio_setup();
void		pageio_done();
void		biodone();
void		buf_breakup();

#if defined(__STDC__)
extern void dma_pageio(void (*)(), struct buf *);
extern void dma_access(u_char, u_int, u_int, u_char, u_char);
#else
extern void dma_pageio();
extern void dma_access();
#endif

#ifdef	MX300I
/*
 * These macros replace the direct use of B_DONE, B_BUSY, B_WANTED.
 */
#define SBIODONE(bp)    ((bp)->b_flags |= B_DONE )
#define RBIODONE(bp)    ((bp)->b_flags &= ~B_DONE )
#define GBIODONE(bp)    ((bp)->b_flags & B_DONE )
#define SBAVAIL(bp)      ((bp)->b_flags |= B_BUSY )
#define RBAVAIL(bp)      ((bp)->b_flags &= ~B_BUSY )
#define GBAVAIL(bp)      ((bp)->b_flags & B_BUSY )
#define SBWANTED(bp)     ((bp)->b_flags |= B_WANTED )
#define RBWANTED(bp)     ((bp)->b_flags &= ~B_WANTED )
#define GBWANTED(bp)     ((bp)->b_flags & B_WANTED )
#define bufinit( a, b )
#define bufalloc(bp) {\
	int x = spl6(); \
	while (GBAVAIL(bp)) { \
	    SBWANTED(bp); \
	    (void) sleep((caddr_t)bp, PRIBIO); \
	} \
	SBAVAIL(bp); (void)splx(x); }

#define buffree(bp)  { \
	int x = spl6(); \
	RBAVAIL(bp); \
	if (GBWANTED(bp)) { \
	    RBWANTED(bp); \
	    wakeup((caddr_t)bp); \
	} \
	(void)splx (x); }
#endif	/* MX300I */

/*
 * Hooks for physio pre- and post-processing, to allow for systems
 * which need to set up special mappings for B_PHYS buffers before
 * handing them to drivers.
 *
 * PHYSIO_STRAT() is used whenever a strategy routine is to be called
 * for a B_PHYS buffer (i.e. a buffer with user virtual b_un.b_addr).
 *
 * PHYSIO_DONE() is used after I/O started by PHYSIO_STRAT() has completed,
 * and we've waited for it with biowait(); this can be used to unmap
 * special mappings.
 *
 * For i386, we don't do any special processing.
 */

#define PHYSIO_STRAT(strat, bp) \
		(void) (*(strat)) (bp)

#define PHYSIO_DONE(bp, addr, count)	/**/

#endif	/* _FS_BUF_H */
