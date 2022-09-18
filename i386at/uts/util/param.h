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

#ifndef _UTIL_PARAM_H	/* wrapper symbol for kernel use */
#define _UTIL_PARAM_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:util/param.h	1.8"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h> /* COMPATIBILITY */
#endif

#ifndef _FS_S5FS_S5PARAM_H
#include <fs/s5fs/s5param.h> /* COMPATIBILITY */
#endif

#else

#include <sys/types.h>		/* COMPATIBILITY */
#include <sys/fs/s5param.h>	/* COMPATIBILITY */

#endif	/* _KERNEL_HEADERS */

/*
 * Fundamental variables; don't change too often.
 */

#define CANBSIZ	256		/* maximum size of typewriter line */

#define HZ	100		/* 100 ticks/second of the clock */
#define TICK    10000000	/* nanoseconds per tick */

#define RESTRICTED_DMA	1	/* DMA may not be able to access all memory */

#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE 0 /* disable special character functions */
#endif

#ifndef MAX_INPUT
#define MAX_INPUT 512     /* maximum bytes stored in the input queue */
#endif

#ifndef MAX_CANON
#define MAX_CANON 256     /* maximum bytes in a line for canonical processing */
#endif

#define UID_NOBODY	60001	/* user ID nobody */
#define GID_NOBODY	UID_NOBODY

#define UID_NOACCESS    60002   /* user ID no access */

#define MAXPID	30000		/* maximum process id */
#define MAXUID	60002		/* maximum user id */
#define MAXLINK	1000		/* maximum number of links */

#define SSIZE	1		/* initial stack size (*4096 bytes) */
#define SINCR	1		/* increment of stack (*4096 bytes) */
#define USIZE	MINUSIZE	/* inital size of user block (*4096) */
#define MINUSIZE  2		/* min size of user block (*4096 bytes) */
#define MAXUSIZE 18		/* maximum size of user block (*4096 bytes) */

/*
 * This define is here for compatibility purposes only
 * and will be removed in a later release.
 */
#define NOFILE	20

/*
 * The following macros are no longer supported (as of SVR4.0)
 * since there is no longer a limit on the number of files that
 * a process can open. However, for SVR3.2 source compatibility, 
 * you may enable NOFILES_MIN and NOFILES_MAX.
 */
#if 0
#define NOFILES_MIN	20	/* SVR3.2 Source Compatibility */
#define NOFILES_MAX	100	/* SVR3.2 Source Compatibility */
#endif

/*
 * These define the maximum and minimum allowable values of the
 * configurable parameter NGROUPS_MAX.
 */
#define NGROUPS_UMAX	32
#define NGROUPS_UMIN	0

/*
 * The maximum number of pages per kseg.
 */
#define MAXKSEG		127

/*
 * To avoid prefetch errors at the end of a region, the 
 * region must be padded with the following number of bytes.
 */
#define PREFETCH	0

/*
 * Priorities.  Should not be altered too much.
 */

#define PMASK	0177
#define PCATCH	0400
#define PNOSTOP	01000
#define PSWP	0
#define PINOD	10
#define PSNDD	PINOD
#define PRIBIO	20
#define PZERO	25
#define PMEM	0
#define NZERO	20
#define PPIPE	26
#define PVFS	27
#define PWAIT	30
#define PSLEP	39
#define PUSER	60
#define PIDLE	127

/*
 * Fundamental constants of the implementation--cannot be changed easily.
 */

#define NBPW	sizeof(int)	/* number of bytes in an integer */
#define NCPPT	1024		/* number of clicks per page table */
#define CPPTSHIFT	10	/* LOG2(NCPPT) if exact */
#define NBPC	4096		/* number of bytes per click */
#define BPCSHIFT	12	/* LOG2(NBPC) if exact */
#define NULL	0
#define CMASK	0		/* default mask for file creation */
#define CDLIMIT	(1L<<14)	/* default maximum write address */
#define NBPSCTR         512     /* bytes per LOGICAL disk sector */
#define UBSIZE		512	/* unix block size */
#define SCTRSHFT	9	/* shift for BPSECT */

#define USERMODE(cs)	((cs) & SEL_RPL)	/* user mode == levels 1-3 */

#define lobyte(X)	(((unsigned char *)&(X))[0])
#define hibyte(X)	(((unsigned char *)&(X))[1])
#define loword(X)	(((ushort *)&(X))[0])
#define hiword(X)	(((ushort *)&(X))[1])

#define MAXSUSE	255

/* REMOTE -- whether machine is primary, secondary, or regular */
#define SYSNAME 9		/* # chars in system name */
#define PREMOTE 39

/* XENIX compatibility */
#define ktop(vaddr)	((paddr_t)svirtophys(vaddr))

/*
 * MAXPATHLEN defines the longest permissible path length,
 * including the terminating null, after expanding symbolic links.
 * MAXSYMLINKS defines the maximum number of symbolic links
 * that may be expanded in a path name. It should be set high
 * enough to allow all legitimate uses, but halt infinite loops
 * reasonably quickly.
 * MAXNAMELEN is the length (including the terminating null) of
 * the longest permissible file (component) name.
 */
#define MAXPATHLEN	1024
#define MAXSYMLINKS	20
#define MAXNAMELEN	256

#ifndef NADDR
#define NADDR 13
#endif

/*
 * The following are defined to be the same as
 * defined in limits.h.  They are
 * needed for pipe and FIFO compatibility.
 */
#ifndef PIPE_BUF
#define PIPE_BUF	5120		/* max # bytes atomic in write to pipe */
#endif

#ifndef PIPE_MAX
#define PIPE_MAX	5120		/* max # bytes in one write to pipe */
#endif

#define NBBY	8			/* number of bits per byte */

/*
 * File system parameters and macros.
 *
 * The file system is made out of blocks of at most MAXBSIZE units,
 * with smaller units (fragments) only in the last direct block.
 * MAXBSIZE primarily determines the size of buffers in the buffer
 * pool. It may be made larger without any effect on existing
 * file systems; however making it smaller make make some file
 * systems unmountable.
 *
 * Note that the block devices are assumed to have DEV_BSIZE
 * "sectors" and that fragments must be some multiple of this size.
 */

#define MAXBSIZE	8192		/* maximum file system block size */
#define MAXBSHIFT	13		/* log2(MAXBSIZE) */
#define MAXBOFFSET	(MAXBSIZE - 1)	/* maximum offset within fs block */
#define MAXBMASK	(~MAXBOFFSET)	/* mask for offset within fs block */
#define DEV_BSIZE	512		/* block device "sector" size */
#define DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define MAXFRAG 	8		/* maximum fragments per fs block */

/* converts number of bytes to number of sectors ("disk blocks") */ 
#define btodb(bytes) 	((unsigned)(bytes) >> DEV_BSHIFT)

/* converts number of sectors ("disk blocks") to number of bytes */
#define dbtob(db)	((unsigned)(db) << DEV_BSHIFT)

/*
 * The MMU_PAGES macro definitions describe the physical page size
 * used by the mapping hardware.
 * The PAGES macro definitions describe the logical page size
 * used by the system.
 */

#define MMU_PAGESIZE	0x1000		/* 4096 bytes */
#define MMU_PAGESHIFT	12		/* log2(MMU_PAGESIZE) */
#define MMU_PAGEOFFSET	(MMU_PAGESIZE-1)/* mask of address bits in page */
#define MMU_PAGEMASK	(~MMU_PAGEOFFSET)

#define PAGESIZE	0x1000		/* same as above for logical page */
#define PAGESHIFT	12
#define PAGEOFFSET	(PAGESIZE - 1)
#define PAGEMASK	(~PAGEOFFSET)

#ifndef NODEV
#define NODEV	(dev_t)(-1)
#endif

/*
 * Some random macros for units conversion.
 */

/*
 * MMU pages to bytes, and back (with and without rounding)
 */
#define mmu_ptob(x)	((x) << MMU_PAGESHIFT)
#define mmu_btop(x)	(((unsigned)(x)) >> MMU_PAGESHIFT)
#define mmu_btopr(x)	((((unsigned)(x) + MMU_PAGEOFFSET) >> MMU_PAGESHIFT)) /* rounded */

/*
 * Pages to bytes, and back (with and without rounding).
 */
#define ptob(x)		((x) << PAGESHIFT)
#define btop(x)		(((unsigned)(x)) >> PAGESHIFT)
#define btopr(x)	((((unsigned)(x) + PAGEOFFSET) >> PAGESHIFT)) /* rounded */

#define shm_alignment	ptob(1)		/* segment size */


#endif	/* _UTIL_PARAM_H */
