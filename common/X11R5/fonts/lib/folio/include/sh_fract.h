/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/sh_fract.h	1.1"
/*
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                   PROPRIETARY NOTICE (Combined) 
**   
**            This source code is unpublished proprietary 
**            information constituting, or derived under 
**            license from AT&T's UNIX(r) System V. 
**   
**                       Copyright Notice 
**   
**            Notice of copyright on this source code 
**            product does not indicate publication. 
**   
**    Copyright (C) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
**    Copyright (C) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T
**   
**                      All rights reserved. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                    RESTRICTED RIGHTS LEGEND: 
**    Use, duplication, or disclosure by the Government is subject 
**    to restrictions as set forth in subparagraph (c)(1)(ii) of 
**    the Rights in Technical Data and Computer Software clause at 
**    DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
**    FAR Supplement. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
*/


#ifndef _SH_FRACT

typedef int	fract;

extern	short	fract_overflows;	/* overflow occured flag */

extern	fract	frmul(), frdiv(), frsqrt();
extern	fract	vfradd(), vfrsub(), vfrmul(), vfrdiv();

#define	fradd(x,y)	((x)+(y))
#define	frsub(x,y)	((x)-(y))
#define frrsh(i,n)	((i)>>(n))
#define frlsh(i,n)	((i)<<(n))

#define floorfr(x)	((x)>>16)
#define aafloorfr(x)	((x)>>14)
#define roundfr(x)	(((x)+FRHALF)>>16)
#define ceilingfr(x)	(((x)+fracti(1)-1)>>16)
#define	frfloor(x)	((x)&0xffff0000)
#define	frround(x)	(((x)+FRHALF)&0xffff0000)
#define	frceiling(x)	(((x)+fracti(1)-1)&0xffff0000)

#define fracti(i)	((fract)((i)*(1<<16)))
#define floatfr(x)	(((float) x)/(1<<16))
#define	fractf(x)	(fract) ((x)*(1<<16))

#define FRHUGE		((fract)0x7fffffff)
#define FRSQRT2		((fract)92682)	/* exactly 1.414215087890625 */
#define	FRSQRT2_2	((fract)46341)	/* exactly 0.7071075439453125 */
#define FRPI		((fract)205887)	/* exactly 3.1415863037109375 */
#define FRE		((fract)178142)	/* exactly 2.718281828 */
#define FRHALF		((fract)0x00008000) /* exactly 0.5 */

#define fraction(num, denom) (fracti(num)/(denom))
#define frpercent(pc)	fraction(pc, 100)
#define frsq(x)		frmul(x, x)
#define fractionalbits(x) ((unsigned short) (x))



/* structure for accessing the parts of a fract.
 * POSSIBLE PORTABILITY PROBLEM: WORD ORDER.  These definitions work
 * for both little- and big- endian machines. */

struct FRACT_STRUCT {
#ifdef LITTLEENDIAN
	unsigned short fractional;
	short integer;
#else	/* LITTLEENDIAN */
        short integer;
        unsigned short fractional;
#endif	/* LITTLEENDIAN */
};

/* lfloorfr(x) is equivalent to floorfr(x).  It is only valid for lvalues */
#define lfloorfr(x) ((*(struct FRACT_STRUCT *) &(x)).integer)
#define cfloorfr(x) ((*(struct FRACT_STRUCT *) &(x)).integer)

#define _SH_FRACT
#endif	/* _SH_FRACT */
