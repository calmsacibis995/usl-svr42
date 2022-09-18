/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/arc.h	1.1"
/*
 * @(#)arc.h 1.3 89/06/09
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


#ifdef	COMMENT

	The first call to the archandler should always be to split up the
	arc into segments that span at most one quadrant, and push them onto 
	the arc_stack. This is done by a call to 

			arc_SplitArcAndFillStack(arc,LShift) .

	When the stack has been created, the arc segment on the top is the 
	only one that can be used. A call to 

			arc_PopStack(arc,disp)

	will put a copy of the top of the stack into 'arc'. This arc will
	be in 16/16 fixed point, contained in a quadrant and small 
    enough not to overflow the integer arithmetic used in tracing arcs.

#endif /*COMMENT*/


/*----------------------------------------------------------------------
	DEFINITIONS
----------------------------------------------------------------------*/

#define		STKSIZE		((int32)40)	     /* max num of arc segments */  


/*
	STRUCTURES
*/

/* lines in 16/16 fixed point */

typedef struct _arc_frLine{
	pair_frXY	from;					  /* starting point of line */
	pair_frXY	to;								/* endpoint of line */
} arc_frLine;

/* conics in 16/16 fixed point */

typedef struct _arc_frConic{
	pair_frXY		a,b,c;	  /* three points that define the conic */
	fract		sh2;						   /* sharpness squared */
} arc_frConic;


/* beziers in 16/16 fixed point */

typedef struct _arc_frBezier{
	pair_frXY		a,b,c,d;		/* four points that define a bezier */
} arc_frBezier;

/*	Can hold a line, conic or bezier in 16/16 fixed point */

typedef union _arc_frData{
	arc_frLine		line; 
	arc_frConic		conic;
	arc_frBezier	bezier;
} arc_frData;

/* The actual arc structure */

#define     	arc_LINETYPE    ((int32)0)
#define     	arc_CONICTYPE   ((int32)1)
#define     	arc_BEZIERTYPE  ((int32)2)

typedef struct _arc_frSegment{
	int8		type;				/* line, conic or bezier */
	bbox_iBBox	bbox;				/* integer bbox for arc */
	arc_frData	data;				/* holds one type of arc */
} arc_frSegment;

/*----------------------------------------------------------------------
	FUNCTIONS
----------------------------------------------------------------------*/

extern	bool	arc_PopStack(/*arc,disp*/);
/*arc_frSegment	*arc;*/
/*pair_iXY		disp;*/
/* 
	Will return the arc on the top of the stack to the caller. This arc
	will be in 16/16 fixed point, contained in a quadrant and small
	enough not to overflow the integer arithmetic used in tracing arcs.
	It will return false when there are no more arcs on the stack, and
	true otherwise.
*/

extern	void	arc_SplitArcAndFillStack(/*arc,LShift*/);
/*arc_frSegment	*arc;*/
/*int8		LShift;*/
/*
	Takes an arc, splits it up if necessary, and puts the segments on the
	stack. At any time, the stack will have segments of one type of curve;
	bezier, conic or line.
*/
