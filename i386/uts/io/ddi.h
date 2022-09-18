/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DDI_H
#define _IO_DDI_H

#ident	"@(#)uts-x86:io/ddi.h	1.16"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifdef DDI_OFF
#include <proc/user.h>
#include <io/mkdev.h>
#include <svc/time.h>
#include <util/sysmacros.h>
#include <util/sysinfo.h>
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#ifdef	DDI_OFF
#include <sys/user.h>
#include <sys/mkdev.h>
#include <sys/time.h>
#include <sys/sysmacros.h>
#include <sys/sysinfo.h>
#endif

#endif /* _KERNEL_HEADERS */


#ifdef _KERNEL

#define MAXCLOCK_T 0x7FFFFFFF

#endif /* _KERNEL */

/*
 * ddi.h -- the flag and function definitions needed by DDI-conforming
 * drivers.  This header file contains #undefs to undefine macros that
 * drivers would otherwise pick up in order that function definitions
 * may be used. Programmers should place the include of "sys/ddi.h"
 * after any header files that define the macros #undef'ed or the code
 * may compile incorrectly.
 */

/*
 * define min() and max() as macros so that drivers will not pick up the
 * min() and max() kernel functions since they do unsigned comparison only.
 */
#define min(a, b)	((a) < (b) ? (a) : (b))
#define max(a, b)	((a) < (b) ? (b) : (a))

/*
 * The following macros designate a kernel parameter for drv_getparm
 * and drv_setparm. Implementation-specific parameter defines should
 * start at 100.
 */
#ifndef DDI_OFF
#define	TIME	1
#define	UPROCP	2
#define	PPGRP	3
#define	LBOLT	4
#define	SYSRINT	5
#define	SYSXINT	6
#define	SYSMINT	7
#define	SYSRAWC	8
#define	SYSCANC	9
#define	SYSOUTC	10
#define	PPID	11
#define	PSID	12
#define UCRED   13

#if defined (__STDC__)						
extern int drv_getparm(ulong, ulong *);		
extern int drv_setparm(unsigned long, unsigned long);		
#else								
extern int drv_getparm();
extern int drv_setparm();
#endif /*__STDC__*/

#endif

#ifndef NMAJORENTRY
#define NMAJORENTRY	256
#endif

extern int drv_getevtoken();
extern void drv_relevtoken();

#if defined (__STDC__)                                          
extern void drv_usecwait(clock_t);
extern clock_t drv_hztousec(clock_t);
extern clock_t drv_usectohz(clock_t);
/* convert internal to extern major number */
extern int itoemajor(major_t, int);
/* convert external to internal major number */
extern int etoimajor(major_t);
#else
extern void drv_usecwait();
extern clock_t drv_hztousec();
extern clock_t drv_usectohz();
extern int itoemajor();
extern int etoimajor();
#endif

extern u_int hat_getkpfnum();
extern u_int hat_getppfnum();

/* The following declaration takes the place of an inline function
 * defined in sys/inline.h .
 */
#ifndef DDI_OFF
extern paddr_t kvtophys();	
#endif

#ifdef _KERNEL_HEADERS

#ifndef _FS_BUF_H
#include <fs/buf.h>	/* SVR4.0COMPAT */
#endif
#ifndef _IO_UIO_H
#include <io/uio.h>	/* SVR4.0COMPAT */
#endif

#elif defined(_KERNEL)

#include <sys/buf.h>	/* SVR4.0COMPAT */
#include <sys/uio.h>	/* SVR4.0COMPAT */

#else

#include <sys/buf.h>	/* SVR4.0COMPAT */
#include <sys/uio.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#if defined(__STDC__)
struct cred;		/* replaces need for including cred.h */
extern int physiock(void(*)(), struct buf *, dev_t, int, daddr_t, struct uio *);
extern int drv_priv(struct cred *);
#else
extern int drv_priv();
extern int physiock();
#endif

#ifndef DDI_OFF

/* The following declarations take the place of macros in 
 * sysmacros.h The undefs are for any case where a driver includes 
 * sysmacros.h, even though DDI conforming drivers must not.
 */

#undef getemajor
#undef geteminor
#undef getmajor
#undef getminor
#undef makedevice
#undef cmpdev
#undef expdev

#if defined(__STDC__)
extern major_t getemajor(dev_t);
extern minor_t geteminor(dev_t);
#else
extern major_t getemajor();
extern minor_t geteminor();
#endif

extern major_t getmajor();
extern minor_t getminor();
extern dev_t makedevice();
extern dev_t cmpdev();
extern dev_t expdev();

/* The following macros from param.h are also being converted to
 * functions and #undefs must be done here as well since param.h
 * will be included by most if not every driver 
 */

#undef btop
#undef btopr
#undef ptob

extern unsigned long btop();
extern unsigned long btopr();
extern unsigned long ptob();

/*
 * The following declarations take the place of macros in buf.h
 */

#undef bioreset
#undef geterror

#if defined(__STDC__)
extern void bioreset(struct buf *);
extern int geterror(struct buf *);
#else
extern void bioreset();
extern int geterror();
#endif

/* Drivers must include map.h to pick up the structure definition */
/* for the map structure and the declaration of the function malloc(). */
/* Unfortunately, map.h also includes definitions of macros that */
/* drivers should be calling as functions. The following #undefs allow */
/* kernel code to use the macros while drivers call the functions */

#undef mapinit
#undef mapwant

extern void mapinit();
extern unsigned long mapwant();

#if defined(__STDC__)
struct map;		/* replaces need for including map.h */
extern void setmapwant(struct map *);
#else
extern void setmapwant();
#endif

/* when DKI changes are folded back in to DDI, the functions mapinit
 * mapwant and setmapwant should be updated to be the following DKI
 * functions:
 */

extern void rminit();
extern unsigned long rmwant();
extern void rmsetwant();


/* STREAMS drivers and modules must include stream.h to pick up the */
/* needed structure and flag definitions. As was the case with map.h, */
/* macros used by both the kernel and drivers in times past now have */
/* a macro definition for the kernel and a function definition for */
/* drivers. The following #undefs allow drivers to include stream.h */
/* but call the functions rather than macros. */

#undef OTHERQ
#undef RD
#undef WR
#undef datamsg
#undef putnext

extern struct queue *OTHERQ();	/* stream.h */
extern struct queue *RD();
extern struct queue *WR();
extern int datamsg();
extern int putnext();

#if defined(__STDC__)
extern void freerbuf(struct buf *);
#else
extern void freerbuf();
#endif

#else	/* DDI_OFF */

extern clock_t lbolt;		/* time in HZ since last boot */

#if ( HZ == 60 )

#define drv_usectohz(usec)	((usec) / 16667)
#define	drv_hztousec(ticks)	((ticks) * 16667)

#elif ( HZ == 100 )

#define drv_usectohz(usec)	((usec) / 10000)
#define	drv_hztousec(ticks)	((ticks) * 10000)

#else

#define drv_usectohz(usec)	(((usec) * HZ) / 1000000)
#define	drv_hztousec(ticks)	((ticks) * (1000000 / HZ))

#endif

#define UPROCP	(u.u_procp)
#define PPGRP	(u.u_procp->p_pgrp)
#define LBOLT	(lbolt)
#define TIME	(hrestime.tv_sec)
#define PPID	(u.u_procp->p_pid)
#define PSID	(u.u_procp->p_sessp->s_sid)
#define drv_getparm(parm, valuep)  ((*(valuep)=(ulong)(parm)) , 0)

#define SYSRINT	sysinfo.rcvint
#define SYSXINT	sysinfo.xmtint
#define SYSMINT	sysinfo.mdmint
#define SYSRAWC	sysinfo.rawch
#define SYSCANC	sysinfo.canch
#define SYSOUTC	sysinfo.outch
static int ddi_oldpri;	/* we only need one, once set, we can't sleep */
#define drv_setparm(parm, value) (					\
		ddi_oldpri = splhi(),					\
		((parm) += (value)),					\
		splx(ddi_oldpri),					\
		0 )
#define rmsetwant	setmapwant
#define setmapwant(mp)	(						\
	oldpri = splhi(),						\
	mp[0].m_addr++,							\
	splx(oldpri)	)

#define freerbuf(bp)	(kmem_free( (void *)bp, sizeof(struct buf)))

#endif 	/* DDI_OFF */

/* declarations of functions for allocating and deallocating the space */
/* for a buffer header (just a header, not the associated buffer) */

#if defined(__STDC__)
extern struct buf *getrbuf(long);
#else
extern struct buf *getrbuf();
#endif

#ifdef _KERNEL

/*
 * Macros and routines related to spl calls.  This duplicates macros
 *	and declarations in <util/spl.h>, but for various reasons that
 *	file is not a nested include here.
 *
 * Calls to spl routines may be redefined, via macros, to call soft-spl
 *	routines.  Whether or not such redefinition is in place depends
 *	on whether or not the symbols SLOWSPL and DDI_OFF are defined,
 *	as follows:
 *
 *	If SLOWSPL is defined, then spl calls are not redefined; they
 *	invoke the standard "hard" spl routines.
 *
 *	If SLOWSPL is not defined and DDI_OFF is defined, then spl calls
 *	are redefined to be soft-spl calls.
 *
 *	If neither SLOWSPL nor DDI_OFF are defined, then spl calls are
 *	not redefined.
 *
 * These rules may be summarized by the following table:
 *
 *	+------------------+------------------+---------------+
 *	| SLOWSPL defined? | DDI_OFF defined? | use soft-spls |
 *	+------------------+------------------+---------------+
 *	|        no        |        no        |       no      |
 *	+------------------+------------------+---------------+
 *	|        yes       |        no        |       no      |
 *	+------------------+------------------+---------------+
 *	|        no        |        yes       |       yes     |
 *	+------------------+------------------+---------------+
 *	|        yes       |        yes       |       no      |
 *	+------------------+------------------+---------------+
 *
 *
 * Note that the redefinition of spl routines to soft-spl's applies even
 * to the routine declarations below, i.e., if the spl calls are re-defined
 * to be soft-spl calls, then the declarations below are actually for spl0s
 * spl1s, etc., instead of spl0, spl1, etc.
 */

#if !defined(SLOW_SPL) && defined(DDI_OFF)

/*
 * Define spl functions to soft-spl functions.
 */
#define	spl0	spl0s
#define	spl1	spl1s
#define	spl2	spl2s
#define	spl3	spl3s
#define	spl4	spl4s
#define	spl5	spl5s
#define	spl6	spl6s
#define	spl7	spl7s
#define	splstr	splstrs
#define	splvm	splvms
#define	splimp	splimps
#define	spltty	splttys
#define	splhi	splhis
#define	splx	splxs

#else

/*
 * Undefine spl macros in case spl.h has been included
 * previously.
 */
#undef	spl0
#undef	spl1
#undef	spl2
#undef	spl3
#undef	spl4
#undef	spl5
#undef	spl6
#undef	spl7
#undef	splstr
#undef	splvm
#undef	splimp
#undef	spltty
#undef	splhi
#undef	splx

#endif

/*
 * Include function prototypes (or declarations) for spl functions.
 *	Note that if spl functions have been defined to be soft-spls,
 *	then these declarations are for the soft-spl routines.
 */
#if	defined(__STDC__)

extern int spl0(void);
extern int spl1(void);
extern int spl2(void);
extern int spl3(void);
extern int spl4(void);
extern int spl5(void);
extern int spl6(void);
extern int spl7(void);
extern int splstr(void);
extern int splvm(void);
extern int splimp(void);
extern int spltty(void);
extern int splhi(void);
extern int splx(int);

#else	/* __STDC__ */

extern int spl0();
extern int spl1();
extern int spl2();
extern int spl3();
extern int spl4();
extern int spl5();
extern int spl6();
extern int spl7();
extern int splstr();
extern int splvm();
extern int splimp();
extern int spltty();
extern int splhi();
extern int splx();

#endif	/* __STDC__ */

#if defined(__STDC__)
extern addr_t physmap(paddr_t, ulong_t, uint_t);
extern void physmap_free(addr_t, ulong_t, uint_t);
extern void rdma_filter(void (*)(struct buf *), struct buf *);
#else
extern addr_t physmap();
extern void physmap_free();
extern void rdma_filter();
#endif

#endif

#endif	/* _IO_DDI_H */
