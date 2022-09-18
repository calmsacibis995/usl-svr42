/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_SPL_H	/* wrapper symbol for kernel use */
#define _UTIL_SPL_H	/* subject to change without notice */

#ident	"@(#)uts-x86:util/spl.h	1.3"
#ident	"$Header: $"

/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*
 * This file contains declarations for the spl routines.  Also, it
 * allows the redefinition of spl routines to a soft-spl routines
 * on the 386.
 *
 * By default, spl* are redefined, via macros, to the soft-spl routines.
 * This default can be overridden by compiling with -DSLOWSPL.
 *
 * Note that the redefinition of spl routines to soft-spl's applies even
 * to the routine declarations below, i.e., if the spl calls are re-defined
 * to be soft-spl calls, then the declarations below are actually for spl0s
 * spl1s, etc instead of spl0, spl1, etc.
 */

#ifndef	SLOWSPL

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

#endif

#ifdef	__STDC__

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

#endif	/* _UTIL_SPL_H */
