/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/slopes.h	1.1"
/*
 * @(#)slopes.h 1.2 89/03/10
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


/*
    The operations used to map points to the first quadrant or to the 
    first octant: rotations of multiples of 90 degrees clockwise and
    reflection (flip) around the line through the origin and (1,1)
*/

#define	slope_NullOp	0 
#define	slope_R90   	1  
#define	slope_R180	2
#define	slope_R270	3  
#define	slope_F11    	4
#define	slope_R90F11    5  
#define	slope_R180F11   6
#define	slope_R270F11   7

#define	slope_OpHasF11(o)     ((o)>=slope_F11)
#define	slope_AddF11ToOp(o)   ((o)+slope_F11)

#define slope_OpSwapsXY(o)    ( ((o) == slope_R90)  || \
    	    	    	      ((o) == slope_R270) ||   \
    	    	    	      ((o) == slope_F11)  ||   \
    	    	    	      ((o) == slope_R180F11) )

/* Treatment of ties during tracing */
/* 
   WARNING: The current implementation for conics assumes
   that ties are always treated as positive, since they
   are mapped to the first octant ans no flipping is necessary.
   THEREFORE: The following definitions should not be changed lightlty.
*/
   
#define	slope_TIEISPOS	0
#define	slope_TIEISNEG	1


#define	slope_OpEffectOnTies(o)	(slope_OpHasF11(o)?  	\
    	    	    	    	 slope_TIEISNEG:	\
    	    	    	    	 slope_TIEISPOS)


/* Making a Slope */
#define	slope_SlopeFrom2Points(s,pf,pt) {	    	    	    	    	\
    	    	    	    	    	(s)->x=((pt)->x)-((pf)->x);	\
    	    	    	    	    	(s)->y=((pt)->y)-((pf)->y);	\
    	    	    	    	      }


/*
    Given a point <*p> returns the operation type that can be used to
    map the point to the first quadrant
*/
extern int32	slope_FindOpToQuad1(/* p */);
/*  pair_iXY	*p; */


/*
    Given a point <*p> returns the operation type that can be used to
    map the point to the first octant.
*/
extern int32	slope_FindOpToOct1(/* p */);
/* pair_iXY	*p; */


/*
    Given a point <*p> and a operation maps the point accordingly
*/
extern void	slope_ApplyOp (/* p, op */);
/*  pair_iXY    *p; */
/*  int32	op; */

