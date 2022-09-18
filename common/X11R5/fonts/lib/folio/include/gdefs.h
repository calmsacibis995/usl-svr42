/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/gdefs.h	1.1"
/*
 * @(#)gdefs.h 1.2 89/03/10
 *
 */
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

/*#include <math.h>*/
#include	"frmath.h"


/*---------------------------------------------------------------
    	MATH
---------------------------------------------------------------*/

/*
    These macros are collected here for portability reasons
    and should be examined very carefully when moving the
    code to a different machine. The math functions of the
    raw machine should NOT be used, only the macros below.
*/


#define	math_Floor(x)    ((int32)floor(x))

#define	math_Ceiling(x)  ((int32)ceil(x))


/* translation of floats to integers */

/*	The following round function should be used instead of the
 *	<rint> provided in the math library, which only works if your program
 *	is compiled with the -f68881 option.  And furthermore, there are
 *	inconsistencies in the result.		may 5/11/88
 *      Returns an integer
 */
#define math_iRound(x)        (((x) > 0) ? math_Floor((x)+0.5) : math_Ceiling((x)-0.5))

/* same as above, returns a double */
#define math_dRound(x)        (((x) > 0) ? floor((x)+0.5) : ceil((x)-0.5))


/* computing absolute values */

#define math_Abs(x)     (((x)>=0)?(x):(-(x)))

#define math_FloatAbs(x) (((x)>=0.0)?(x):(-(x)))

#define math_DoubleAbs(x) (((x)>=0.0)?(x):(-(x)))


/* trigonometric stuff */

#define math_Sin(x)     (sin(x))

#define math_Cos(x)     (cos(x))

#define math_Tan(x)     (tan(x))

#define math_Asin(x)    (asin(x))

#define math_Acos(x)    (acos(x))

#define math_Atan(x)    (atan(x))

#define math_Atan2(x)   (atan2(x))


/* miscellaneous others */

#define math_Sqrt(x)    (sqrt(x))

#define math_Square(x)   ((x)*(x))

/*---------------------------------------------------------------
    	pair XY: REPRESENTATION OF POINTS
---------------------------------------------------------------*/

/* In source coordinates */

typedef struct {
    double   x,y;
} pair_dXY;


typedef struct {
    float   x,y;
} pair_fXY;


/* The same in grid coordinates */

typedef struct {
    int32   x,y;
} pair_iXY;

/* The same in 16/16 fixed point coordinates */

typedef struct {
    fract   x,y;
} pair_frXY;

/*---------------------------------------------------------------
    	BOUNDING BOXES
---------------------------------------------------------------*/

/* In source coordinates */

typedef struct {
    double   lox,loy,hix,hiy;
} bbox_dBBox;

typedef struct {
    float   lox,loy,hix,hiy;
} bbox_fBBox;


/* The same for grid coordinates */

typedef struct {
    int32   lox,loy,hix,hiy;
} bbox_iBBox;

/* The same for 16/16 fixed point coordinates */

typedef struct {
    fract   lox,loy,hix,hiy;
} bbox_frBBox;

/*---------------------------------------------------------------
    	TRANSFORMATIONS
---------------------------------------------------------------*/

/*
    The transformation: a 2D transformation in homogeneous coordinates
*/

/* in double */

typedef	struct {
    double    a,b,c,d,dx,dy;
} trans_dTrans;

/* in double */

typedef	struct {
    fract    a,b,c,d,dx,dy;
} trans_frTrans;

extern	trans_dTrans	trans_Identity;


