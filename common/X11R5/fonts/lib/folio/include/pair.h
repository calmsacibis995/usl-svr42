/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/pair.h	1.1"
/*
 * @(#)pair.h 1.3 89/05/02
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


/*---------------------------------------------------------------
    	pair XY: REPRESENTATION OF POINTS
---------------------------------------------------------------*/


/* Initialize an XY pair <p> */

#define pair_InitXY(p,px,py)    {(p)->x = px; (p)->y = py;}


/* Manhattan distance between point <*p1> and <*p2> */

#define pair_ManhattanDistance(p1,p2) 	(math_Abs((p1)->x - (p2)->x) + \
    	    	    	    	    	 math_Abs((p1)->y - (p2)->y) )

/* Square of the shortest distance between point <*p1> and <*p2> */

#define	pair_SquareOfShortestDistance(p1,p2)						\
	(math_Square((p1)->x - (p2)->x) + math_Square((p1)->y - (p2)->y))


/*----------------------------------------------------------*/

/* 
    Add/Subtract a value pair <*v> to/from original pair <*p>
    to get new pair <*r>.
*/

#define pair_XYAdd(r,p,v)   	{ (r)->x = (p)->x + (v)->x; (r)->y = (p)->y + (v)->y; }
#define pair_XYSubtract(r,p,v)	{ (r)->x = (p)->x - (v)->x; (r)->y = (p)->y - (v)->y; }

/*----------------------------------------------------------*/

/* 
    Add/Subtract a fract pair <*v> to/from original pair <*p>
    to get new pair <*r>.
*/

#define pair_frAdd(r,p,v)   	{ (r)->x = fradd(((p)->x),((v)->x)); (r)->y = fradd(((p)->y),((v)->y)); }
#define pair_frSubtract(r,p,v) 	{ (r)->x = frsub(((p)->x),((v)->x)); (r)->y = frsub(((p)->y),((v)->y)); }

/*----------------------------------------------------------*/
/* 
    shift Left or Right by <s> both components of an integer Pair <*p>
    to get new pair <*r>.
*/

#define pair_XYShiftLeft(r,p,s)	    {(r)->x = (p)->x << s; (r)->y = (p)->y << s;}
#define pair_XYShiftRight(r,p,s)    {(r)->x = (p)->x >> s; (r)->y = (p)->y >> s;}

/*----------------------------------------------------------*/
/* 
    shift Left or Right by <s> both components of a fract Pair <*p>
    to get new pair <*r>.
*/

#define pair_frShiftLeft(r,p,s)	    {(r)->x = frlsh(((p)->x),s); (r)->y = frlsh(((p)->y),s);}
#define pair_frShiftRight(r,p,s)   {(r)->x = frrsh(((p)->x),s); (r)->y = frrsh(((p)->y),s);}

/*----------------------------------------------------------*/
/* 
    Scale original pair <*p> by a constant factor <f>
    to get new pair <*r>.
*/
#define pair_XYScale(r,p,f)    	{(r)->x = (p)->x * f; (r)->y = (p)->y * f;}

/*----------------------------------------------------------*/
/* 
    Scale original fract pair <*p> by a constant factor <f>
    to get new pair <*r>.
*/
#define pair_frScale(r,p,f)    	{(r)->x = frmul(((p)->x),f); (r)->y = frmul(((p)->y),f);}

/*----------------------------------------------------------*/
/* 
    Change sign of both components of original pair <*p> 
    to get new pair <*r>.
*/

#define pair_XYChangeSign(r,p)  { (r)->x = -(p)->x; (r)->y = -(p)->y;}

/*----------------------------------------------------------*/

/* 
    Add Constant <c> to both components of original pair <*p> 
    to get new pair <*r>
*/

#define	pair_XYAddScalar(r,p,c)    {(r)->x = (p)->x + c;(r)->y = (p)->y + c;}

/*----------------------------------------------------------*/

/* 
    Add Constant <c> to both components of original fract pair <*p> 
    to get new pair <*r>
*/

#define	pair_frAddScalar(r,p,c)    {(r)->x = fradd(((p)->x),c);(r)->y = fradd(((p)->y),c);}

/*----------------------------------------------------------*/
    
/*  
    Round double pair <*p> to integer pair <*r>
 */

#define	pair_XYRound(r,p);  {(r)->x = math_iRound((p)->x);(r)->y = math_iRound((p)->y);}

/*----------------------------------------------------------*/
    
/*  
    Round fract pair <*p> to integer pair <*r>
 */

#define	pair_frRound(r,p);  {(r)->x = roundfr((p)->x);(r)->y = roundfr((p)->y);}

/*----------------------------------------------------------*/
    
/*  
   Convert integer pair <*p> to double pair <*r>
 */

#define	pair_iXYTodXY(r,p) {(r)->x = (double)((p)->x); (r)->y = (double)((p)->y);}

/*----------------------------------------------------------*/
    
/*  
   Convert integer pair <*p> to fract pair <*r>
 */

#define	pair_iXYTofrXY(r,p) {(r)->x = fracti((p)->x); (r)->y = fracti((p)->y);}

/*----------------------------------------------------------*/
    
/*  
   Convert fract pair <*p> to double pair <*r>
 */

#define	pair_frXYTodXY(r,p) {(r)->x = (double)(floatfr((p)->x)); (r)->y = (double)(floatfr((p)->y));}

/*----------------------------------------------------------*/
    
/*  
   Convert double pair <*p> to fract pair <*r>
 */

#define	pair_dXYTofrXY(r,p) {(r)->x = math_iRound(fracti((p)->x)); (r)->y = math_iRound(fracti((p)->y));}

/*----------------------------------------------------------*/
/*  
   Convert double pair <*p> to FIXEDPOINT pair <*r>
 */

#define	pair_dToFixedPoint(r,p,f) {					\
			register double	tmp;				\
			tmp = (double)(1<<f);				\
			(r)->x = math_iRound((p)->x*tmp); 		\
    	    	   	(r)->y = math_iRound((p)->y*tmp);		\
		   }


/*----------------------------------------------------------*/
/*
	Round a 16/16 fixed point pair <*p> to a 32 bit integer <*r>
	-- Add A half and truncate
*/

#define pair_RoundFixedToInt(r,p) {					\
		Rint32	tmp;						\
		tmp = 1<<(FIXED16 - 1);	/* a.k.a. FRHALF */		\
		(r)->x = ((p)->x + tmp) >> FIXED16; 			\
		(r)->y = ((p)->y + tmp) >> FIXED16; 			\
	}

/*----------------------------------------------------------*/
/*
	Convert a 16/16 fixed point number into one with f bits of fraction.
*/

#define	pair_Convert16FixedToVarFixed(r,p,f) {				\
		Rint32	tmp,rnd;					\
		tmp = FIXED16 - f;					\
		rnd = 1 << (tmp-1);					\
		(r)->x = ((p)->x + rnd) >> tmp; 			\
		(r)->y = ((p)->y + rnd) >> tmp; 			\
	}

/*----------------------------------------------------------*/
/*
	Convert a 32-f/f fixed point number into an integer
*/

#define	pair_RoundVarFixed(r,p,f) {					\
		Rint32	rnd;						\
		rnd = 1 << (f-1);					\
		(r)->x = ((p)->x + rnd) >> f; 				\
		(r)->y = ((p)->y + rnd) >> f; 				\
	}

/*----------------------------------------------------------*/
/*
	Convert a 16/16 fixed point pair <*p> into a double pair <*r>.
*/

#define pair_Convert16FixedToDouble(r,p) {				\
		register double tmp;					\
		tmp = (double)(1<<FIXED16);				\
		(r)->x = ( ((double)(p)->x) / tmp ); 			\
		(r)->y = ( ((double)(p)->y) / tmp ); 			\
	}

/*----------------------------------------------------------*/
