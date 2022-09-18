/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_SYSMACROS_H	/* wrapper symbol for kernel use */
#define _UTIL_SYSMACROS_H	/* subject to change without notice */

#ident	"@(#)uts-x86:util/sysmacros.h	1.6"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_PARAM_H
#include <util/param.h> /* COMPATIBILITY */
#endif

#else

#include <sys/param.h> /* COMPATIBILITY */

#endif	/* _KERNEL_HEADERS */

/*
 * Some macros for units conversion.
 */

/*
 * Core clicks to page tables and vice versa.
 * These are still called ctos, etc. to avoid large scale
 * changes in the code. "Segment" here means the memory
 * spanned by a page table.
 */
#define ctos(x)		(((x) + (NCPPT-1)) >> CPPTSHIFT)	/* rounded */
#define ctost(x)	((x) >> CPPTSHIFT)			/* truncated */
#define stoc(x)		((x) * NCPPT)

/*
 * Disk blocks (sectors) and bytes.
 */
#define dtob(DD)	((DD) << SCTRSHFT)
#define btod(BB)	(((BB) + NBPSCTR - 1) >> SCTRSHFT)	/* rounded */
#define btodt(BB)	((BB) >> SCTRSHFT)			/* truncated */

/*
 * Disk blocks (sectors) and pages.
 */
#define NDPP		8	/* number of disk blocks per page */
#define DPPSHFT		3	/* shift for disk blocks per page */
#define ptod(PP)	((PP) << DPPSHFT)
#define dtop(DD)	(((DD) + NDPP - 1) >> DPPSHFT)		/* rounded */
#define dtopt(DD)	((DD) >> DPPSHFT)			/* truncated */

/* clicks to bytes */

#ifdef BPCSHIFT
#define ctob(x)	((x)<<BPCSHIFT)
#else
#define ctob(x)	((x)*NBPC)
#endif

/* bytes to clicks */

#ifdef BPCSHIFT
#define btoc(x)	(((unsigned)(x)+(NBPC-1))>>BPCSHIFT)		/* rounded */
#define btoct(x)	((unsigned)(x)>>BPCSHIFT)		/* truncated */
#else
#define btoc(x)	(((unsigned)(x)+(NBPC-1))/NBPC)			/* rounded */
#define btoct(x)	((unsigned)(x)/NBPC)			/* truncated */
#endif

/* common macros */

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) < (b) ? (b) : (a))

/*
 * WARNING: The device number macros defined here should not be used by device 
 * drivers or user software. Device drivers should use the device functions
 * defined in the DDI/DKI interface (see also ddi.h). Application software 
 * should use of the library routines available in makedev(3). A set of new 
 * device macros are provided to operate on the expanded device number format 
 * supported in SVR4. Macro versions of the DDI device functions are provided 
 * for use by kernel proper routines only. Macro routines bmajor(), major(),
 * minor(), emajor(), eminor(), and makedev() will be removed or
 * their definitions changed at the next major release following SVR4.
 */

#define O_BITSMAJOR	7	/* # of SVR3 major device bits */
#define O_BITSMINOR	8	/* # of SVR3 minor device bits */
#define O_MAXMAJ	0x7f	/* SVR3 max major value */
#define O_MAXMIN	0xff	/* SVR3 max minor value */

#define L_BITSMAJOR	14	/* # of SVR4 major device bits */
#define L_BITSMINOR	18	/* # of SVR4 minor device bits */

/*
 * Although 14 bits are reserved, the 3b2
 * major number is restricted to 8 bits. 
 */
#define L_MAXMAJ	0xff	

/*
 * MAX minor for 3b2 software drivers.
 * For 3b2 hardware devices the minor is
 * restricted to 256 (0-255).
 */
#define L_MAXMIN	0x3ffff	

/* major part of a device external from the kernel (same as emajor below) */
#define major(x)	(int)(((unsigned)x>>O_BITSMINOR)&O_MAXMAJ)
#define bmajor(x)	(int)(((unsigned)x>>O_BITSMINOR)&O_MAXMAJ)

/* minor part of a device external from the kernel (same as eminor below) */
#define minor(x)	(int)(x&O_MAXMIN)

/* create old device number */
#define makedev(x,y)	(unsigned short)(((x)<<O_BITSMINOR) | (y&O_MAXMIN))

/* make a new device number */
#define makedevice(x,y)	(unsigned long)(((x)<<L_BITSMINOR) | ((y)&L_MAXMIN))

/*
 * emajor() allows kernel/driver code to print external major numbers.
 * eminor() allows kernel/driver code to print external minor numbers.
 */

#define emajor(x)	(int)((((unsigned long)(x)>>O_BITSMINOR) > O_MAXMAJ) ? \
			NODEV : (((unsigned long)(x)>>O_BITSMINOR)&O_MAXMAJ))
#define eminor(x)	(int)((x)&O_MAXMIN)

/*
 * Get external major and minor device 
 * components from expanded device number.
 */

#define getemajor(x)	(int)((((unsigned long)(x)>>L_BITSMINOR) > L_MAXMAJ) ? \
			NODEV : (((unsigned long)(x)>>L_BITSMINOR)&L_MAXMAJ))
#define geteminor(x)	(int)((x)&L_MAXMIN)

/* convert to old dev format */

#define cmpdev(x) 	(unsigned long)((((x)>>L_BITSMINOR) > O_MAXMAJ || \
				((x)&L_MAXMIN) > O_MAXMIN) ? NODEV : \
				((((x)>>L_BITSMINOR)<<O_BITSMINOR)|((x)&O_MAXMIN)))

/* convert to new dev format */

#define expdev(x) 	(unsigned long)(((((x)>>O_BITSMINOR)&O_MAXMAJ)<<L_BITSMINOR) \
				| ((x)&O_MAXMIN))

/*
 * Evaluate to true if the process is an RFS server.
 */
#define RF_SERVER()	(u.u_procp->p_sysid != 0)

/*
 * Machine dependent operations - defined for RFS and STREAMS.
 * Examples are provided for some well-known machines; use these as models.
 * Note:  The [SIL]ALIGN macros expect a pointer argument.
 */

#define SALIGN(p)	(char *)(((int)(p)+1) & ~1)
#define IALIGN(p)	(char *)(((int)(p)+3) & ~3)
#define LALIGN(p)	(char *)(((int)(p)+3) & ~3)

 /*
 **	pdp11
 **#define SALIGN(p)	(char *)(((int)(p)+1) & ~1)
 **#define IALIGN(p)	(char *)(((int)(p)+1) & ~1)
 **#define LALIGN(p)	(char *)(((int)(p)+1) & ~3)
 **
 **	vax
 **#define SALIGN(p)	(char *)(((int)(p)+1) & ~1)
 **#define IALIGN(p)	(char *)(((int)(p)+3) & ~3)
 **#define LALIGN(p)	(char *)(((int)(p)+3) & ~3)
 **
 **	u3b2
 **#define SALIGN(p)	(char *)(((int)(p)+1) & ~1)
 **#define IALIGN(p)	(char *)(((int)(p)+3) & ~3)
 **#define LALIGN(p)	(char *)(((int)(p)+3) & ~3)
 */

/*
 * Note:  The [SIL]NEXT macros expect an aligned pointer argument.
 */
#define SNEXT(p)	(char *)((int)(p) + sizeof (short))
#define INEXT(p)	(char *)((int)(p) + sizeof (int))
#define LNEXT(p)	(char *)((int)(p) + sizeof (long))

/*
 * Macros for counting and rounding.
 */

#define howmany(x, y)	(((x)+((y)-1))/(y))
#define roundup(x, y)	((((x)+((y)-1))/(y))*(y))

#endif	/* _UTIL_SYSMACROS_H */
