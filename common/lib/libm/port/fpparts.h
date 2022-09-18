/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/fpparts.h	1.3"

/* Macros to pull apart parts of single and  double precision
 * floating point numbers in IEEE format
 * Be sure to include /usr/include/values.h before including
 * this file to get the required definition of _IEEE
 */

#if _IEEE

#include "machfp.h"

/* parts of a double precision floating point number */
#define	SIGNBIT(X)	(((_dval *)&(X))->fparts.sign)
#define EXPONENT(X)	(((_dval *)&(X))->fparts.exp)
#define HIFRACTION(X)	(((_dval *)&(X))->fparts.hi)
#define LOFRACTION(X)	(((_dval *)&(X))->fparts.lo)
#define QNANBIT(X)	(((_dval *)&(X))->nparts.qnan_bit)
#define HIWORD(X)	(((_dval *)&(X))->fwords.hi)
#define LOWORD(X)	(((_dval *)&(X))->fwords.lo)

#define MAXEXP	0x7ff /* maximum exponent of double*/
#define ISMAXEXP(X)	((EXPONENT(X)) == MAXEXP)

/* macros used to create quiet NaNs as return values */
#define SETQNAN(X)	((((_dval *)&(X))->nparts.qnan_bit) = 0x1)
#define HIQNAN(X)	((HIWORD(X)) = 0x7ff80000)
#define LOQNAN(X)	((((_dval *)&(X))->fwords.lo) = 0x0)

/* macros used to extract parts of single precision values */
#define	FSIGNBIT(X)	(((_fval *)&(X))->fparts.sign)
#define FEXPONENT(X)	(((_fval *)&(X))->fparts.exp)
#define FFRACTION(X)	(((_fval *)&(X))->fparts.fract)
#define FWORD(X)	(((_fval *)&(X))->fword)
#define FQNANBIT(X)	(((_fval *)&(X))->nparts.qnan_bit)
#define MAXEXPF	255 /* maximum exponent of single*/
#define FISMAXEXP(X)	((FEXPONENT(X)) == MAXEXPF)

#endif  /*IEEE*/
