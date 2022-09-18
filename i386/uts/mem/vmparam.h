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

#ifndef _MEM_VMPARAM_H	/* wrapper symbol for kernel use */
#define _MEM_VMPARAM_H	/* subject to change without notice */

#ident	"@(#)uts-x86:mem/vmparam.h	1.7"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Machine dependent constants.
 */

/*
 * SSIZE and SINCR probably belong here rather than in param.h;
 * currently they are defined in both places for compatibility.
 */
#define	SSIZE		1			/* initial stack size */
#define	SINCR		1			/* increment of stack */

/*
 * Paging thresholds (see vm_pageout.c).
 * Strategy of 3/17/83:
 *	lotsfree is 256k bytes, but at most 1/8 of memory
 *	desfree is 100k bytes, but at most 1/16 of memory
 *	minfree is 32k bytes, but at most 1/2 of desfree
 */
#define	LOTSFREE	(256 * 1024)
#define	LOTSFREEFRACT	8
#define	DESFREE		(100 * 1024)
#define	DESFREEFRACT	16
#define	MINFREE		(32 * 1024)
#define	MINFREEFRACT	2

/*
 * There are two clock hands, initially separated by HANDSPREAD bytes
 * (but at most all of user memory).  The amount of time to reclaim
 * a page once the pageout process examines it increases with this
 * distance and decreases as the scan rate rises.
 */
#define	HANDSPREAD	(2 * 1024 * 1024)

/*
 * Paged text files that are less than PGTHRESH bytes
 * may be "prefaulted in" instead of demand paged.
 */
#define PGTHRESH	(32 * 1024)

/*
 * Some architectures allow for discontiguous chunks of physical memory.
 * MAX_MEM_CHUNK is the maximum number of such chunks allowed.
 * For architectures with contiguous memory, set this to 1.
 */
#define MAX_MEM_CHUNK	10

/*
 * Functions that return a physical page ID return NOPAGE if
 * there is no valid physical page ID.
 */
#define NOPAGE		((unsigned int)-1)

/*
 * PSPACE_MAINSTORE is the code for the physical address space that includes,
 * at least, "mainstore" system memory, which is the memory that programs
 * (and the kernel) execute out of.  See hat_getppfnum().
 */
#define PSPACE_MAINSTORE	0

/*
 *  User address space offsets.
 *
 *****************************  NOTE - NOTE  *********************************
 *
 *	ANY CHANGES TO THESE DEFINES MUST BE REFLECTED IN uprt.s.
 */

#define UVBASE	((unsigned long)0x00000000L) /* main store virtual address */
#define UVSTACK	((unsigned long)0x7FFFFFFCL) /* stack bottom virtual address */
#define UVSHM	((unsigned long)0x80000000L) /* shared memory address */
#define KVBASE	((unsigned long)0xC0000000L) /* base of kernel memory map */
#define KVXBASE	((unsigned long)0xC8000000L) /* base for extended memory */
#define KVSBASE	((unsigned long)0xD0000000L) /* base for kern text/data/bss */
/* symbol table put here to optimize searches through kas and to
avoid collision with extra kernel stack page just below UVUBLK */
#define KVDSYMBASE ((unsigned)0xDFB00000L)   /* base for pageable symbol table */
#define KVDSYMEND ((unsigned)0xDFBFFFFFL)    /* end of pageable symbol table */
#define UVUBLK	((unsigned long)0xE0000000L) /* ublock virtual address */

#define UVTEXT		UVBASE		/* beginning address of user text */
#define UVEND		KVBASE		/* end of user virtual address range */
#define MINUVADR	UVTEXT		/* minimum user virtual address */
#define MAXUVADR	KVBASE		/* maximum user virtual address */
#define MINKVADR	KVBASE		/* minimum kernel virtual address */
#define MAXKVADR	UVUBLK		/* maximum kernel virtual address */

#define KADDR(v)	((v) >= MINKVADR)

/*
 * Determine whether [addr, addr+len) are valid user address.
 */
#define VALID_USR_RANGE(addr, len) \
	((u_int)(addr) + (len) > (u_int)(addr) && \
	 (u_int)(addr) >= UVBASE && (u_int)(addr) + (len) <= UVEND)

#ifdef _KERNEL

/* WRITEABLE_USR_RANGE() checks that an address range is within the
 * valid user address range, and that it is user-writeable.
 * On machines where read/write page permissions are enforced on kernel
 * accesses as well as user accesses, this can be simply defined as
 * VALID_USR_RANGE(addr, len), since the appropriate checks will be done
 * at the time of the actual writes.  Otherwise, this must also call a
 * routine to simulate a user write fault on the address range.
 *
 * NOTE:
 * A successful call to WRITEABLE_USR_RANGE() must be terminated with
 * an call to END_USERWRITE
*/

#define WRITEABLE_USR_RANGE(addr, len) \
		(VALID_USR_RANGE(addr, len) && \
		(userwrite(addr, len) == 0) && \
		(u.u_386b1 |= IN_USERWRITE))

#define END_USERWRITE() (u.u_386b1 &= ~IN_USERWRITE)


#if defined(__STDC__)

extern int userwrite(addr_t, size_t);

#else

extern int userwrite();

#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _MEM_VMPARAM_H */
