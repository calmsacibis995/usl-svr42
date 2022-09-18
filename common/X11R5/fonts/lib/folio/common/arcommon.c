/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:common/arcommon.c	1.1"

#ifndef lint
static char sccsid[] = "@(#)arcommon.c 1.2 89/03/10";
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


#include	"cdefs.h"
#include	"common.h"
#include	"arc.h"
#include	"segment.h"
#include	"rmbuffer.h"

/*-----------------------------------------------------------------------------*/
/*
	Finds the integer bounding box for the fixed point arc. This function will
	work if the bounding box is the one in the <frarc> itself.
*/

void	arccommon_GetFrArcBBox(frarc,bbox,LShift)
arc_frSegment	*frarc;
bbox_iBBox		*bbox;
int8			LShift;
{
	switch(frarc->type) {
		case arc_LINETYPE:
			bbox->lox = floorfr(min(frarc->data.line.from.x,frarc->data.line.to.x));
			bbox->loy = floorfr(min(frarc->data.line.from.y,frarc->data.line.to.y));
			bbox->hix = ceilingfr(max(frarc->data.line.from.x,frarc->data.line.to.x));
			bbox->hiy = ceilingfr(max(frarc->data.line.from.y,frarc->data.line.to.y));
			break;
		case arc_CONICTYPE:
			bbox->lox = floorfr(min(min(frarc->data.conic.a.x,
					frarc->data.conic.b.x),frarc->data.conic.c.x));
			bbox->loy = floorfr(min(min(frarc->data.conic.a.y,
					frarc->data.conic.b.y),frarc->data.conic.c.y));
			bbox->hix = ceilingfr(max(max(frarc->data.conic.a.x,
					frarc->data.conic.b.x),frarc->data.conic.c.x));
			bbox->hiy = ceilingfr(max(max(frarc->data.conic.a.y,
					frarc->data.conic.b.y),frarc->data.conic.c.y));
			break;
		case arc_BEZIERTYPE:
			bbox->lox = floorfr(min(min(frarc->data.bezier.a.x,
						frarc->data.bezier.b.x),min(frarc->data.bezier.c.x,
										frarc->data.bezier.d.x)));
			bbox->loy = floorfr(min(min(frarc->data.bezier.a.y,
						frarc->data.bezier.b.y),min(frarc->data.bezier.c.y,
										frarc->data.bezier.d.y)));
			bbox->hix = ceilingfr(max(max(frarc->data.bezier.a.x,
						frarc->data.bezier.b.x),max(frarc->data.bezier.c.x,
										frarc->data.bezier.d.x)));
			bbox->hiy = ceilingfr(max(max(frarc->data.bezier.a.y,
						frarc->data.bezier.b.y),max(frarc->data.bezier.c.y,
										frarc->data.bezier.d.y)));
			break;
	}
	/* don't forget to apply the shift */
	bbox->lox = (bbox->lox << LShift);
	bbox->loy = (bbox->loy << LShift);
	bbox->hix = (bbox->hix << LShift);
	bbox->hiy = (bbox->hiy << LShift);
}

/*-----------------------------------------------------------------------------*/
/*
	Returns whether the arc is a simple one or not.
	A simple arc is one which is contained in a quadrant, and is small enough
	not to overflow the integer arithmetic used in tracing arcs.
*/

bool	arccommon_SimpleArc(frarc,LShift)
arc_frSegment	*frarc;
int8			LShift;
{
	int32	xdim,ydim;			/* dimensions of the arc */
	bbox_frBBox	cbbox; 			/* conic's bbox */

	xdim = (frarc->bbox.hix - frarc->bbox.lox);
	ydim = (frarc->bbox.hiy - frarc->bbox.loy);
	switch(frarc->type) {
		case arc_LINETYPE:
			/* line is simple if small enough */
			if ( (xdim + ydim) >= rm_BUFFERAREASIZE )
				return(false);
			if ( (max(xdim,ydim)) >= seg_MAXLINEDIMENSION )
				return(false);
			/* apply the shift, if neccessary */
			pair_XYShiftLeft(&frarc->data.line.from,&frarc->data.line.from,LShift);
			pair_XYShiftLeft(&frarc->data.line.to,&frarc->data.line.to,LShift);
			break;
		case arc_CONICTYPE:
			/* conic is simple if contained in a quadrant and small enough */
			/* if B is in the bbox of A and C, within one quadrant */
			cbbox.lox = min(frarc->data.conic.a.x,frarc->data.conic.c.x);
			cbbox.loy = min(frarc->data.conic.a.y,frarc->data.conic.c.y);
			cbbox.hix = max(frarc->data.conic.a.x,frarc->data.conic.c.x);
			cbbox.hiy = max(frarc->data.conic.a.y,frarc->data.conic.c.y);
			if ((frarc->data.conic.b.x < cbbox.lox) || 
				(frarc->data.conic.b.x > cbbox.hix) || 
				(frarc->data.conic.b.y < cbbox.loy) || 
				(frarc->data.conic.b.y > cbbox.hiy))
				return(false);
			/* now, is it small enough ? */
			if ( (xdim + ydim) >= rm_BUFFERAREASIZE )
				return(false);
			if ( (max(xdim,ydim)) >= seg_MAXCONICDIMENSION )
				return(false);
			pair_XYShiftLeft(&frarc->data.conic.a,&frarc->data.conic.a,LShift);
			pair_XYShiftLeft(&frarc->data.conic.b,&frarc->data.conic.b,LShift);
			pair_XYShiftLeft(&frarc->data.conic.c,&frarc->data.conic.c,LShift);
			break;
		case arc_BEZIERTYPE:
			return(false);
			break;
	}
	return(true);
}

/*-----------------------------------------------------------------------------*/
