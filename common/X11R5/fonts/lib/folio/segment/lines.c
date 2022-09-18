/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:segment/lines.c	1.1"
#ifndef lint
static char sccsid[] = "@(#)lines.c 1.2 89/03/10";
#endif
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


#include "cdefs.h"
#include "common.h"
#include "arc.h"
#include "rmbuffer.h"
#include "slopes.h"

/*
------------------------------------------------------------------------
        L I N E    T R A C I N G 
------------------------------------------------------------------------
*/
    /*
    	Implements Bresenham's tracing. It is given the initial value of 
    	the tracing function and the increments for a move in the main and 
    	diagonal (secondary followed by main) directions with the assumption 
    	that a main move increments the function. All the values will
    	be within the accuracy of 32 bit integer arithmetic.
    */    	

#define seg_AlterLineGenerateMoves(MainMove,SecondaryMove,movesp,xcnt,ycnt,fn,fm,fd) \
    	    	    {	    	    	    	    	    	    	\
    	    	    	if ( ycnt ) {       	    	    	    	\
    	    	    	    while (xcnt-- > 0) {    	    	    	\
    	            	    	if (fn < 0) {	    /* main move */ 	\
    	    	    	    	    fn += fm;	    	    	    	\
    	    	    	    	} else {    	    /* diagonal move */	\
    	    	    	    	    fn += fd;	    	    	    	\
    	    	    	    	    SecondaryMove(movesp);  	    	\
    	    	    	    	    ycnt--; 	    	    	    	\
    	    	    	    	}   	    	    	    	    	\
    	            	    	MainMove(movesp);   	    	    	\
    	    	    	    }	    	    	    	    	    	\
    	    	    	    if (ycnt)	    	    	    	    	\
    	    	    	    	SecondaryMove(movesp);	    	    	\
    	    	    	} else {	    /* horizontal line */   	\
    	    	    	    while (xcnt-- > 0)	    	    	    	\
    	    	    	    	MainMove(movesp);   	    	    	\
    	    	    	}   	    	    	    	    	    	\
    	    	    }

#define seg_LineGenerateMoves(MainMove,SecondaryMove,movesp,xcnt,ycnt,fn,fm,fd) \
    	    	    {	    	    	    	    	    	    	\
    	    	    	    while ((xcnt > 0) && (ycnt > 0)) {    	\
    	            	    	if (fn < 0) {	    /* main move */ 	\
    	    	    	    	    fn += fm;	    	    	    	\
				    xcnt--;				\
    	    	    	    	} else {    	    /* diagonal move */	\
    	    	    	    	    fn += fd;	    	    	    	\
    	    	    	    	    SecondaryMove(movesp);  	    	\
    	    	    	    	    xcnt--; ycnt--; 	    	    	\
    	    	    	    	}   	    	    	    	    	\
    	            	    	MainMove(movesp);   	    	    	\
    	    	    	    }	    	    	    	    	    	\
    	    	    	    while (xcnt--)	    	    	    	\
    	    	    	    	MainMove(movesp);	    	    	\
    	    	    	    while (ycnt--)	    	    	    	\
    	    	    	    	SecondaryMove(movesp);	    	    	\
    	    	    }

/* ----------------------------------------------------------------------- */


#define	seg_FRACTION	4	/* bits of fraction used */

/*
    Given a <line> segment and a handle <bhdl> to a large enough rook move 
    buffer, traces the line and  fills the buffer accordingly.
*/

void	seg_TraceLine( line, bhdl )
arc_frLine    	    *line;	/* end points are 16.16 fracts */
rm_bufferHandle	    bhdl;
{
    /*
    	We always convert the problem to one of tracing a line so that
    	the starting point of the trace is close to (0,0). The line itself 
    	is defined by two points which may not have integral coordinates
    	but one of them is less than one unit (pixel) away from (0,0).
    	The line will be transformed so that its slope lies in the
    	first octant to simplify the rook move generation: x is the 
    	"main" direction of tracing, y is the "secondary" and because 
    	two secondary moves cannot occur consecutively, we consider 
    	"diagonal" moves (composed of a move in the secondary direction 
    	followed by a move in the main direction) to be primitive. 
    	The function used for the tracing is 
    		    fn = 2*(dy*x - dx*y)
    	where the sign has been selected so that the function is positive
    	below the curve (hence moves in the main direction increase 
    	the function) and the 2 insures that the function will have integral
    	values at half points. Because the move used to trace the line at a 
    	given point depends on the value of the function at the pixel center
    	that lies above and to the right of the grid point, the value
    	kept for tracing is modified accordingly -- when at (x,y) we
    	use the value at (x+0.5,y+0.5). Even though the tracing is always 
    	done in the first octant, the method of resolving ties (i.e., how 
    	to treat points that fall in the line) is different for different 
    	lines depending on the octant in which the lines lay originally. In
    	practice, this translates into two different ways of tracing: one 
    	that considers ties to be to the right of the line mapped to the
    	1st octant, and another that considers them to the left. The code
    	for both is identical and we simply decrement the value of the
    	function by one in the appropriate case so that the two can be
    	treated the same. 
    	All the arithmetic used is 32-bit integer arithmetic, even when 
    	the numbers have fraction bits (they are represented as fixed 
    	point numbers). The largest number employed is approximate 2*D*D 
    	where D is the horizontal or vertical distance between the points 
    	that define the line. For 32-bit arithmetic, that means that
    	D <= 33,000. Thus if the numbers that define the line have f bits 
    	of fraction, they must be closer than 33,000/2^f.
    	   ****** CURRENT ALGORITHM USES 4 bits of fraction ******
	The bounding box of largest line traceable is therefore less than
	~2K pixels to a side.
    */

    pair_iXY	ifrom,ito;  	/* rounded end points in integer */
    pair_iXY	ffrom,fto;  	/* end points in FIXEDPOINT 28.4 */
    pair_iXY	tmp;		/* ifrom in FIXEDPOINT 28.4 */
    pair_iXY	fslope;	    	/* slope in FIXEDPOINT 28.4 */
    line_iLine	ln; 	    	/* equation of line in FIXEDPOINT 28.4 */
    int32   	op; 	    	/* operation to map line to 1st octant */

    /* turn endpoints into real pixel coordinates */
    pair_RoundFixedToInt(&ifrom,&line->from);
    pair_RoundFixedToInt(&ito,&line->to);

    /* represent endpoints in 28.4 FIXEDPOINT numbers */
    pair_Convert16FixedToVarFixed(&ffrom,&line->from,seg_FRACTION);
    pair_Convert16FixedToVarFixed(&fto,&line->to,seg_FRACTION);

    /* translate the 28.4 endpoints close to origin */
    pair_XYShiftLeft(&tmp, &ifrom, seg_FRACTION);
    pair_XYSubtract(&ffrom,&ffrom,&tmp);
    pair_XYSubtract(&fto,&fto,&tmp);
    
    /* transpose endpoints to first octant using rotation/reflexion */
    slope_SlopeFrom2Points(&fslope,&ffrom,&fto);
    op = slope_FindOpToOct1(&fslope);
    slope_ApplyOp(&ffrom,op);
    slope_ApplyOp(&fto,op);


    /* setup tracing information and trace */
    {
    	Rbyte   *movesp;	    	/* pointer to sequence of rook moves */
    	Rint32  xcnt,ycnt;	    	/* # of rook moves to be generated */
    	Rint32  fn,fm,fd;	    	/* local tracing values (Bresenham) */
	Rint32	tmp;

    	/* convert endpoints to a line equation of the form  mx + ny + p */
    	line_iLineFrom2Points(&ffrom,&fto,&ln);
    
    	/* 
    	    Evaluate initial function value fn = 2*(.05m + .05n + p).
    	    Remember p has 2*seg_FRACTION bits of fraction.
    	    *********  TRICKIER THAN IT LOOKS  *******
    	    Modify fn such that test during ties is always "fn < 0"
    	    Modification is simply decrementing fn by one if
    	    	a. there is a possibility for real ties
    	    	b. AND the mapping operation to the first octant
    	    	   would results in ties to occur when fn is positive.
    	*/
    	fn = ln.m + ln.n + ((ln.p >> seg_FRACTION) << 1);
    	if ( seg_FRACTION ) {
    	    if ( !(ln.p & ( (1<<seg_FRACTION) - 1)) &&
    	    	  (slope_OpEffectOnTies(op) == slope_TIEISNEG))
    	    	fn -= 1;
    	} else {
    	    if (slope_OpEffectOnTies(op) == slope_TIEISNEG)
    	    	fn -= 1;
    	}

    	fm = ln.m << 1;  	    	/* change in fn for main move */
    	fd = (ln.m + ln.n) << 1; 	/* change in fn for diagonal move */

    	rm_InitMovesp(bhdl,movesp);

    	if (slope_OpSwapsXY(op)) {
	    tmp = ito.y - ifrom.y; xcnt = abs(tmp);
	    tmp = ito.x - ifrom.x; ycnt = abs(tmp);
    	    seg_LineGenerateMoves(rm_PutY,rm_PutX,movesp,xcnt,ycnt,fn,fm,fd);
    	} else {
	    tmp = ito.x - ifrom.x; xcnt = abs(tmp);
	    tmp = ito.y - ifrom.y; ycnt = abs(tmp);
    	    seg_LineGenerateMoves(rm_PutX,rm_PutY,movesp,xcnt,ycnt,fn,fm,fd);
    	}
    }
}   
