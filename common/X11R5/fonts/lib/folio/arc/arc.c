/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:arc/arc.c	1.1"

#ifndef lint
static char sccsid[] = "@(#)arc.c 1.13 89/07/07";
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


#include	<stdio.h>
#include	"cdefs.h"
#include	"common.h"
#include	"segment.h"
#include	"rmbuffer.h"
#include	"arc.h"
#include	"arclocal.h"

/* GLOBALS */

arc_fxSegment		arc_stack[STKSIZE];	/* where the arc segments are stored */
arc_fxSegment		*arc_stacktop;	   /* points to the top arc in the stack */

/* I suspect that this is safer - see EDU 1/9/88 */
#define mysqrt(v)   ((v)>SQRTBOUND? math_Sqrt(v) : ((double) (0.0)))
#define	arc_maxsqrt(n)	((n)>(2.0)? (n) : (2.0))
#define	fx_rm_BUFFERAREASIZE		(fracti(rm_BUFFERAREASIZE))
#define	fx_seg_MAXCONICDIMENSION	(fracti(seg_MAXCONICDIMENSION))
/*---------------------------------------------------------------------*/
/*
	Miscellaneous routines that lines, conics and beziers use.
*/
/*---------------------------------------------------------------------*/
/* Finds the absolute max of 4 fract numbers. */

fract	arc_MaxOf4(a,b,c,d)
fract	a,b,c,d;
{
	fract	maxab, maxcd;

	maxab = max(math_Abs(a),math_Abs(b));
	maxcd = max(math_Abs(c),math_Abs(d));
	return(max(maxab,maxcd));
}

/*---------------------------------------------------------------------*/
/* Finds the absolute max of 6 fract numbers. */

fract	arc_MaxOf6(a,b,c,d,e,f)
fract	a,b,c,d,e,f;
{
	fract	maxab, maxcd, maxef, maxabcd;

	maxab = max(math_Abs(a),math_Abs(b));
	maxcd = max(math_Abs(c),math_Abs(d));
	maxef = max(math_Abs(e),math_Abs(f));
	maxabcd = max(maxab,maxcd);
	return(max(maxabcd,maxef));
}

/*---------------------------------------------------------------------*/
/* Finds the absolute max of 8 fract numbers. */

fract	arc_MaxOf8(a,b,c,d,e,f,g,h)
fract	a,b,c,d,e,f,g,h;
{
	fract	maxab, maxcd, maxef, maxgh, maxabcd, maxefgh;

	maxab = max(math_Abs(a),math_Abs(b));
	maxcd = max(math_Abs(c),math_Abs(d));
	maxef = max(math_Abs(e),math_Abs(f));
	maxgh = max(math_Abs(g),math_Abs(h));
	maxabcd = max(maxab,maxcd);
	maxefgh = max(maxef,maxgh);
	return(max(maxabcd,maxefgh));
}

/*---------------------------------------------------------------------*/
/* Finds the absolute max of 10 fract numbers. */

fract	arc_MaxOf10(a,b,c,d,e,f,g,h,i,j)
fract	a,b,c,d,e,f,g,h,i,j;
{
	fract	maxab, maxcd, maxef, maxgh, maxij,  maxabcd, maxefgh;

	maxab = max(math_Abs(a),math_Abs(b));
	maxcd = max(math_Abs(c),math_Abs(d));
	maxef = max(math_Abs(e),math_Abs(f));
	maxgh = max(math_Abs(g),math_Abs(h));
	maxij = max(math_Abs(i),math_Abs(j));
	maxabcd = max(maxab,maxcd);
	maxefgh = max(maxef,maxgh);
	return(max((max(maxabcd,maxefgh)),maxij));
}
/*---------------------------------------------------------------------*/
/*	Fills the arc_tarray with TINIT, (fract)(1). */

void	arc_Inittarray()
{
	int32	i;
	i = 0;
	while (i < MAXTARRAY) {
		arc_tarray[i].t = TINIT;
		arc_tarray[i].sn = TINIT;
		arc_tarray[i].sd = TINIT;
		i++;
	}
}

/*---------------------------------------------------------------------*/
/*
	Puts arc_tarray elements in descending order. A more efficient sort could
	be used, but since the maximum number of elements is 6, this is not
	really necessary.
*/ 

void	arc_Sorttarray(num)
int32	num;
{
	arc_pointinfo	temp;
	int32	i;
	bool	swaps;

	swaps = true;
	while (swaps) {
		i = 0;
		swaps = false;
		while ( i < (num - 1) ) {
			if (arc_tarray[i].t < arc_tarray[i+1].t) { 
				temp = arc_tarray[i];
				arc_tarray[i] = arc_tarray[i+1];
				arc_tarray[i+1] = temp;
				swaps = true;
			}
			i++;
		}
	}
}

/*---------------------------------------------------------------------*/
/*
	Adjusts the shift in the line to be as small as possible. The shift
	could be too large after a line has been split or displaced.
*/

void	arc_AdjustFxLineShift(fxline)
arc_fxLine	*fxline;
{
	fract	limit,maxnum;
	int32	cnt,factor;

	if ( (fxline->shift) == 0 )
		return;
	cnt = 0;
	limit = frrsh(FRHUGE,2);	/* allow a little room for arithmetic */
	maxnum = (arc_MaxOf6(fxline->from.x,fxline->from.y,fxline->to.x,
					fxline->to.y,fxline->disp.x,fxline->disp.y));
	while (maxnum < limit) {
		cnt++;
		limit = frrsh(limit,1);
	}
	factor = min(fxline->shift,cnt);
	if (factor > 0) {
		pair_frShiftLeft(&fxline->from,&fxline->from,factor);
		pair_frShiftLeft(&fxline->to,&fxline->to,factor);
		pair_frShiftLeft(&fxline->disp,&fxline->disp,factor);
		fxline->shift -= factor;
	}
}

/*---------------------------------------------------------------------*/
/*
	Adjusts the shift in the 2D conic to be as small as possible. The shift
	could be too large after a conic has been split or displaced.
*/

void	arc_AdjustFxConic2DShift(fxconic)
arc_fxConic	*fxconic;
{
	fract	limit,maxnum;
	int32	cnt,factor;

	if ( (fxconic->shift) == 0 )
		return;
	cnt = 0;
	limit = frrsh(FRHUGE,2);	/* allow a little room for arithmetic */
	maxnum = (arc_MaxOf8(fxconic->a.x,fxconic->a.y,fxconic->b.x,fxconic->b.y,
				fxconic->c.x,fxconic->c.y,fxconic->disp.x,fxconic->disp.y));
	while (maxnum < limit) {
		cnt++;
		limit = frrsh(limit,1);
	}
	factor = min(fxconic->shift,cnt);
	if (factor > 0) {
		pair_frShiftLeft(&fxconic->a,&fxconic->a,factor);
		pair_frShiftLeft(&fxconic->b,&fxconic->b,factor);
		pair_frShiftLeft(&fxconic->c,&fxconic->c,factor);
		pair_frShiftLeft(&fxconic->disp,&fxconic->disp,factor);
		fxconic->shift -= factor;
	}
}

/*---------------------------------------------------------------------*/
/*
	Adjusts the shift in the 3D conic to be as small as possible. The shift
	could be too large after a conic has been split or displaced.
*/

void	arc_AdjustFxConic3DShift(fxconic3D)
arc_fxConic3D	*fxconic3D;
{
	fract	limit,maxnum;
	int32	cnt,factor;

	if ( (fxconic3D->shift) == 0 )
		return;
	cnt = 0;
	limit = frrsh(FRHUGE,2);	/* allow a little room for arithmetic */
	maxnum = (arc_MaxOf4(fxconic3D->c.x,fxconic3D->c.y,fxconic3D->disp.x,
													fxconic3D->disp.y));
	while (maxnum < limit) {
		cnt++;
		limit = frrsh(limit,1);
	}
	factor = min(fxconic3D->shift,cnt);
	if (factor > 0) {
		pair_frShiftLeft(&fxconic3D->a,&fxconic3D->a,factor);
		pair_frShiftLeft(&fxconic3D->b,&fxconic3D->b,factor);
		pair_frShiftLeft(&fxconic3D->c,&fxconic3D->c,factor);
		pair_frShiftLeft(&fxconic3D->disp,&fxconic3D->disp,factor);
		frlsh(fxconic3D->a.z,factor); frlsh(fxconic3D->b.z,factor);
		frlsh(fxconic3D->c.z,factor);
		fxconic3D->shift -= factor;
	}
}

/*---------------------------------------------------------------------*/
/*
	Adjusts the shift in the bezier to be as small as possible. The shift
	could be too large after a bezier has been split or displaced.
*/

void	arc_AdjustFxBezierShift(fxbezier)
arc_fxBezier	*fxbezier;
{
	fract	limit,maxnum;
	int32	cnt,factor;

	if ( (fxbezier->shift) == 0 )
		return;
	cnt = 0;
	limit = frrsh(FRHUGE,2);	/* allow a little room for arithmetic */
	maxnum = (arc_MaxOf10(fxbezier->a.x,fxbezier->a.y,fxbezier->b.x,
				fxbezier->b.y, fxbezier->c.x,fxbezier->c.y,fxbezier->d.x,
						fxbezier->d.y,fxbezier->disp.x,fxbezier->disp.y));
	while (maxnum < limit) {
		cnt++;
		limit = frrsh(limit,1);
	}
	factor = min(fxbezier->shift,cnt);
	if (factor > 0) {
		pair_frShiftLeft(&fxbezier->a,&fxbezier->a,factor);
		pair_frShiftLeft(&fxbezier->b,&fxbezier->b,factor);
		pair_frShiftLeft(&fxbezier->c,&fxbezier->c,factor);
		pair_frShiftLeft(&fxbezier->d,&fxbezier->d,factor);
		pair_frShiftLeft(&fxbezier->disp,&fxbezier->disp,factor);
		fxbezier->shift -= factor;
	}
}

/*---------------------------------------------------------------------*/
/* 
	Converts an arc represented in 16/16 fixed point and a Left Shift
	into an arc represented in local fract representation.
*/

void	arc_FrArcToFxArc(frarc,fxarc,LShift)
arc_frSegment	*frarc;
arc_fxSegment	*fxarc;
int8			LShift;
{
	pair_frXY		disp;

	fxarc->type = frarc->type;
	fxarc->bbox = frarc->bbox;
	disp.x = fracti(((fxarc->bbox.lox+fxarc->bbox.hix)/2));
	disp.y = fracti(((fxarc->bbox.loy+fxarc->bbox.hiy)/2));
	switch(frarc->type) {
		case arc_LINETYPE:	/* lines are displaced to the center of the bbox */
							fxarc->data.line.from = frarc->data.line.from;
							fxarc->data.line.to = frarc->data.line.to;
							pair_frSubtract(&fxarc->data.line.from,
											&fxarc->data.line.from,&disp);
							pair_frSubtract(&fxarc->data.line.to,
											&fxarc->data.line.to,&disp);
							fxarc->data.line.shift = LShift;
							fxarc->data.line.disp = disp;
							arc_AdjustFxLineShift(&fxarc->data.line);
							break;
		case arc_CONICTYPE:	/* conics are not displaced, will be displaced later */ 
							fxarc->data.conic.a = frarc->data.conic.a;
							fxarc->data.conic.b = frarc->data.conic.b;
							fxarc->data.conic.c = frarc->data.conic.c;
							fxarc->data.conic.sh2 = frarc->data.conic.sh2;
							fxarc->data.conic.disp.x = fracti(0);
							fxarc->data.conic.disp.y = fracti(0);
							fxarc->data.conic.shift = LShift;
							arc_AdjustFxConic2DShift(&fxarc->data.conic);
							break;
		case arc_BEZIERTYPE:
							fxarc->data.bezier.a = frarc->data.bezier.a;
							fxarc->data.bezier.b = frarc->data.bezier.b;
							fxarc->data.bezier.c = frarc->data.bezier.c;
							fxarc->data.bezier.d = frarc->data.bezier.d;
							pair_frSubtract(&fxarc->data.bezier.a,
											&fxarc->data.bezier.a,&disp);
                            pair_frSubtract(&fxarc->data.bezier.b,
											&fxarc->data.bezier.b,&disp);
                            pair_frSubtract(&fxarc->data.bezier.c,
											&fxarc->data.bezier.c,&disp);
                            pair_frSubtract(&fxarc->data.bezier.d,
											&fxarc->data.bezier.d,&disp);
							fxarc->data.bezier.shift = LShift;
							fxarc->data.bezier.disp = disp;
							arc_AdjustFxBezierShift(&fxarc->data.bezier);
							break;
	}
}

/*-----------------------------------------------------------------------------*/
/* 
	Converts an arc in local fract representation into an arc represented in 
	16/16 fixed point and an integer displacement from the origin.
	NOTE: The arc will never be a bezier, since a bezier will always be 
	approximated by a conic. Also the arc will always have a shift of zero, 
	since it cannot be converted into a 16/16 fixed point otherwise.
*/

void	arc_FxArcToFrArc(fxarc,frarc,disp)
arc_fxSegment	*fxarc;
arc_frSegment	*frarc;
pair_iXY		*disp;
{
	pair_frXY		tempdisp;				/* fract version of the disp */

	frarc->type = fxarc->type;
	frarc->bbox = fxarc->bbox;
	switch(fxarc->type) {
		case arc_LINETYPE:
							disp->x = math_iRound((roundfr(fxarc->data.line.disp.x)) - 
									((fxarc->bbox.lox + fxarc->bbox.hix)/2));
							disp->y = math_iRound((roundfr(fxarc->data.line.disp.y)) - 
									((fxarc->bbox.loy + fxarc->bbox.hiy)/2));
							/* express the displacement in fract */
							tempdisp.x = (fract)(fracti(disp->x));
							tempdisp.y = (fract)(fracti(disp->y));
							pair_frSubtract(&frarc->data.line.from,
											&fxarc->data.line.from,&tempdisp);
							pair_frSubtract(&frarc->data.line.to,
											&fxarc->data.line.to,&tempdisp);
							break;
		case arc_CONICTYPE:
							disp->x = math_iRound((roundfr(fxarc->data.conic.disp.x)) -
									((fxarc->bbox.lox + fxarc->bbox.hix)/2));
							disp->y = math_iRound((roundfr(fxarc->data.conic.disp.y)) -
									((fxarc->bbox.loy + fxarc->bbox.hiy)/2));
							/* express the displacement in fract */
							tempdisp.x = (fract)(fracti(disp->x));
							tempdisp.y = (fract)(fracti(disp->y));
							pair_frSubtract(&frarc->data.conic.a,
												&fxarc->data.conic.a,&tempdisp);
							pair_frSubtract(&frarc->data.conic.b,
												&fxarc->data.conic.b,&tempdisp);
							pair_frSubtract(&frarc->data.conic.c,
												&fxarc->data.conic.c,&tempdisp);
							frarc->data.conic.sh2 = fxarc->data.conic.sh2;
							break;
	}
}

/*-----------------------------------------------------------------------------*/
/*
	The following are all the routines that manipulate bezier curves.
*/
/*---------------------------------------------------------------------*/
/*
	Given p1 and p2 calculates p, a point between p1 and p2, using the 
	parameters s and t.
*/

void	arc_BezierPointInterpolation(p1,p2,s,t,p)
pair_frXY	*p1,*p2,*p;
fract	s,t;
{
	p->x = fradd((frmul((p1->x),s)),(frmul((p2->x),t)));
	p->y = fradd((frmul((p1->y),s)),(frmul((p2->y),t)));
}

/*---------------------------------------------------------------------*/
/* Given p1 and p2, calculates their midpoint p */

void	arc_BezierMidPoint(p1,p2,p)
pair_frXY	*p1,*p2,*p;
{
	p->x = ( frdiv(fradd((p1->x),(p2->x)),FR2) );
	p->y = ( frdiv(fradd((p1->y),(p2->y)),FR2) );
}

/*---------------------------------------------------------------------*/
/* 
	Divides the bezier curve b into two beziers b1 and b2 at the point
	t, where t=0 gives the beginning of b and t=1 the end.
*/

void	arc_BezierSplitAtPoint(b0,t,b1,b2)
arc_fxBezier	*b0,*b1,*b2;
fract	t;
{
	arc_fxBezier	b;
	fract	s;
	pair_frXY	tmp;
	b = *b0;
	if (t == FRHALF) {
		b1->a = b.a;
		arc_BezierMidPoint(&b.a,&b.b,&b1->b);
		arc_BezierMidPoint(&b.b,&b.c,&tmp);
		arc_BezierMidPoint(&b1->b,&tmp,&b1->c);
		arc_BezierMidPoint(&b.c,&b.d,&b2->c);
		arc_BezierMidPoint(&tmp,&b2->c,&b2->b);
		arc_BezierMidPoint(&b1->c,&b2->b,&b1->d);
		b2->a = b1->d;
		b2->d = b.d;
	} else {
		s = frsub(FRONE,t);
		b1->a = b.a;
		arc_BezierPointInterpolation(&b.a,&b.b,s,t,&b1->b);
		arc_BezierPointInterpolation(&b.b,&b.c,s,t,&tmp);
		arc_BezierPointInterpolation(&b1->b,&tmp,s,t,&b1->c);
		arc_BezierPointInterpolation(&b.c,&b.d,s,t,&b2->c);
		arc_BezierPointInterpolation(&tmp,&b2->c,s,t,&b2->b);
		arc_BezierPointInterpolation(&b1->c,&b2->b,s,t,&b1->d);
		b2->a = b1->d;
		b2->d = b.d;
	}
}
		
/*---------------------------------------------------------------------*/
/*
	Given the values for A,B,C,D in a bezier curve, this function will 
	calculate the parameter t at the points where the derivative with
	respect to t is zero. It can be passed either the x co-ords or the
	y co-ords for a curve. 
	The function used is:

	f(t) = t*t*t(-A + 3B -3C + D) + t*t(3A - 6B + 3C) + t(-3A + 3B) + A

	f'(t) = 0 gives:

t = (-(A -2B + C) +- sqrt( (A - B)(D - C) + sq(B - C) )) / (-A + 3B -3C + D) 

	There can be at most two values for t; those values calculated which
	do not satisfy  0 <= t <= 1  will be discarded. The function returns 
	the number of solutions of t that it found.
*/

int32	arc_BezierGetTangents(A,B,C,D,tempt)
double	A,B,C,D;
fract	*tempt;
{
	fract	*tptr; 		/* to walk down tempt */
	double	t;			/* to hold each solution */
	double	a,b,sqroot;
	double	help;
	int32	count;
	
	count = 0;
	tptr = tempt;
	a = ((-A) + (3*B) - (3*C) + D);
	b = ((-A) + (2*B) - C);
	help = ( ((A - B)*(D - C)) + ((B - C)*(B - C)) );
	if ( math_Abs(help) < SQRTBOUND )
		sqroot = 0.0;
	else {
		if (help > SQRTBOUND)
			sqroot = mysqrt(help);
		else 
			return(count);
	}
	if (math_Abs(a) < ZEROBOUND) {		/* symmetrical, so linear */
		t = ( (A - B) / (2*(A - 2*B + C)) );
		if ( (t >= MINT) && (t <= MAXT) ) {
			(*tptr) = math_iRound(fractf(t));
			tptr++; count++;
		}
	} else {
		t = ( (b + sqroot) / a);
		if ( (t >= MINT) && (t <= MAXT) ) {
			(*tptr) = math_iRound(fractf(t));
			tptr++; count++;
		}
		t = ( (b - sqroot) / a);
		if ( (t >= MINT) && (t <= MAXT) ) {
			(*tptr) = math_iRound(fractf(t));
			tptr++; count++;
		}
	}
	return(count);
}

/*---------------------------------------------------------------------*/
/*
	Using f'(t) and f''(t) as below,

	f'(t) = 3tt(-A + 3B - 3C + D) + 6t(A - 2B + C) + 3(-A + B)
	f''(t) = 6t(-A + 3B - 3C + D) + 6(A - 2B + C) + 3(-A + B)
 
	and curvature = x'y'' - y'x'' = 0
	gives a, b and c in the solutions of t = -b +- sqrt(bb -4ac)/2a 
	a = (Ax -2Bx + Cx)(-Ay + 3By -3Cy + Dy) - (Ay -2By + Cy)(-Ax + 3Bx - 3Cx + Dx)
	b = (-Ax + Bx)(-Ay + 3By - 3Cy + Dy) - (-Ay + By)(-Ax + 3Bx - 3Cx + Dx)
	c = (-Ax + Bx)(Ay - 2By + Cy) - (-Ay + By)(Ax - 2Bx + Cx)
*/

int32	arc_BezierGetInflectionPt(bez,tempt)
arc_flBezier	*bez;
fract	*tempt;
{
	double t;
	fract *tptr;
	double a,b,c,sqroot;  /* for the solution to the quadratic */
	double ax,bx,cx,dx,ay,by,cy,dy;  	/* to make life easier */
	double	help;
	int32	count;

	count = 0;
	tptr = tempt;
	ax = bez->a.x; bx = bez->b.x; cx = bez->c.x; dx = bez->d.x;
	ay = bez->a.y; by = bez->b.y; cy = bez->c.y; dy = bez->d.y;
	a = ( ((ax - 2*bx + cx)*((-ay) + 3*by - 3*cy + dy)) - 
						((ay - 2*by + cy)*((-ax) + 3*bx - 3*cx + dx)) ); 
	b = ( (((-ax) + bx)*((-ay) + 3*by - 3*cy + dy)) - 
								(((-ay) + by)*((-ax) + 3*bx - 3*cx + dx)) );
	c = ( (((-ax) + bx)*(ay - 2*by + cy)) - (((-ay) + by)*(ax - 2*bx + cx)) ); 
	help = ( (b*b) - (4*a*c) );
	if ( math_Abs(help) < SQRTBOUND )
		sqroot = 0.0;
	else {
		if (help > SQRTBOUND) 
			sqroot = mysqrt(help);
		else 
			return(count);
	}
	if ( math_Abs(a) < ZEROBOUND) {
		t = ( (-c) / b );
		if ( (t >= MINT) && (t < MAXT) ) {
			(*tptr) = math_iRound(fractf(t));
			tptr++; count++;
		}
	} else {
		t = ( ((-b) + sqroot)/(2*a) );
		if ( (t >= MINT) && (t < MAXT) ) {
			(*tptr) = math_iRound(fractf(t));
			tptr++; count++;
		}
		t = ( ((-b) - sqroot)/(2*a) );
		if ( (t >= MINT) && (t < MAXT) ) {
			(*tptr) = math_iRound(fractf(t));
			tptr++; count++;
		}
	}
	return(count);
}

/*---------------------------------------------------------------------*/
/*
	Given A,B,C,D and parameter t, it returns the value for f'(t).
	The equation used for the derivative is:

	f'(t) = 3*t*t(-A + 3B -3C + D) + 6*t(A - 2B + C) + 3(B - A)
*/

fract	arc_BezierFindDeriv(A,B,C,D,t)
fract	A,B,C,D,t;
{
	fract	answer,part1,part2,part3;

	part1 = frmul((frmul(fracti(3),frsq(t))),
                              (fradd(frsub(D,A),frmul(fracti(3),frsub(B,C)))));
	part2 = frmul((frmul(fracti(6),t)),(frsub(fradd(A,C),frmul(fracti(2),B))));
	part3 = frmul((fracti(3)),(frsub(B,A)));
	answer = fradd(part1,fradd(part2,part3));
	return(answer);
}

/*---------------------------------------------------------------------*/
/*
	Given the values for A,B,C,D in a bezier curve and the parameter t,
	this function will calculate the slope at that point by finding
	dy/dt and dy/dx and dividing the two to find dy/dx. It returns the
	slope.
 
	The function used is:

	f(t) = t*t*t(-A + 3B -3C + D) + t*t(3A - 6B + 3C) + t(-3A + 3B) + A
	f'(t) = 3*t*t(-A + 3B -3C + D) + 6*t(A - 2B + C) + 3(B - A)
*/

void	arc_BezierGetSlope(t,b,dydt,dxdt)
fract	t;
arc_fxBezier	*b;
fract	*dydt;
fract	*dxdt;
{
	(*dydt) = arc_BezierFindDeriv(b->a.y,b->b.y,b->c.y,b->d.y,t);
	(*dxdt) = arc_BezierFindDeriv(b->a.x,b->b.x,b->c.x,b->d.x,t);
}

/*---------------------------------------------------------------------*/
/*
	Given a bezier, this function will perform a simple test to determine
	whether there is an inflection point or not. It returns the result.
*/

bool	arc_BezierHasInflectionPt(b)
arc_flBezier	*b;
{
	line_dLine	AD;
	double		BinAD,CinAD;

	/* get the equation for the line AD */
	line_dLineFrom2Points(&b->a,&b->d,&AD);
	/* evaluate B and C in AD */
	BinAD = line_dLineEvaluation(&AD,&b->b);
	CinAD = line_dLineEvaluation(&AD,&b->c);
	/* inflection if BinAD and CinAD have different signs */
	if ((BinAD*CinAD) >= 0.0)
		return(true);
	else
		return(false);
}

/*---------------------------------------------------------------------*/
/*
	Given a bezier curve, it returns, in the global array arc_tarray, the 
	values of the parameter t for which the curve has a horizontal tangent,
	a vertical tangent or a point of inflection. These values must be 
	between 0 and 1; there can be at most 6 values for any bezier curve,
	2 horizontal tangents, 2 vertical tangents and 2 inflection points. 
	It returns a count of how many t values it found. This is all done 
	so that a curve will lie in one quadrant, and go in one direction.
	It returns how many t's it found.
*/

int32	arc_BezierComputeQuadrants(b)
arc_fxBezier	*b;
{
	fract	tempt[2];
	arc_flBezier	flbez;			/* bezier in double */
	int32	i,j,count,keep;

	count = keep = i = 0;
	pair_frXYTodXY(&flbez.a,&b->a); pair_frXYTodXY(&flbez.b,&b->b);
	pair_frXYTodXY(&flbez.c,&b->c); pair_frXYTodXY(&flbez.d,&b->d);
	/* find the vertical tangents */
	count = arc_BezierGetTangents(flbez.a.x,flbez.b.x,flbez.c.x,flbez.d.x,tempt);
	keep = count;
	j = 0;
	while (count) {
		arc_tarray[i].t = tempt[j];
		arc_tarray[i].sn = FRONE;
		arc_tarray[i].sd = 0;
		j++; i++; count--;
	}
	/* find the horizontal tangents */
	count = arc_BezierGetTangents(flbez.a.y,flbez.b.y,flbez.c.y,flbez.d.y,tempt);
	keep += count;
	j = 0;
	while (count) {
		arc_tarray[i].t = tempt[j];	
		arc_tarray[i].sn = 0;
		arc_tarray[i].sd = FRONE;
		j++; i++; count--;
	} 
	/* find the point of inflection, if any */
	if (arc_BezierHasInflectionPt(&flbez)) {
		count = arc_BezierGetInflectionPt(&flbez,tempt);
		keep += count;
		j = 0;
		while (count) {
			arc_tarray[i].t = tempt[j];	
			arc_BezierGetSlope(arc_tarray[i].t,b,&arc_tarray[i].sn,&arc_tarray[i].sd);
			j++; i++; count--;
		} 
	}
	return(keep);
}

/*---------------------------------------------------------------------*/
/*	Pushes one bezier segment onto the stack. */

void	arc_BezierPushStack(bezier)
arc_fxBezier	*bezier;
{
   	arc_stacktop->type = arc_BEZIERTYPE;		
	arc_stacktop->data.bezier = *bezier;   /* the actual curve */
	arc_GetFxArcBBox(arc_stacktop,&arc_stacktop->bbox);
	arc_stacktop++;
}

/*---------------------------------------------------------------------*/
/*
	Given the slope, sn and sd, it will find the two parameters t1 and
	t2 that correspond to points with that slope. It will only return 
	parameters that satisfy 0 <= t <= 1.
*/

void	arc_BezierTFromSlope(sn,sd,bez,t1,t2)
fract	sn,sd;
fract	*t1,*t2;
arc_fxBezier	*bez;
{
	double	a,b,c,sqroot,dblsn,dblsd;			/* for help in solving quadratic */
	double	ax,bx,cx,dx,ay,by,cy,dy;
	double	help;
	double	temp;

	ax = (double)(floatfr(bez->a.x)); bx = (double)(floatfr(bez->b.x));
	cx = (double)(floatfr(bez->c.x)); dx = (double)(floatfr(bez->d.x));
	ay = (double)(floatfr(bez->a.y)); by = (double)(floatfr(bez->b.y));
	cy = (double)(floatfr(bez->c.y)); dy = (double)(floatfr(bez->d.y));
	dblsn = (double)(floatfr(sn)); dblsd = (double)(floatfr(sd));

	a = (3*((dblsn*((-ax)+(3*bx)-(3*cx)+dx)) + (dblsd*(ay-(3*by)+(3*cy)-dy)))); 
	b = (6*((dblsn*(ax - (2*bx) + cx)) + (dblsd*((-ay) + (2*by) - cy)))); 
	c = (3*((dblsn*(bx - ax)) + (dblsd*(ay - by))));
	help = ( (b*b) - (4*a*c) );
	if ( help < 0.0 )
		return;
	else
		sqroot = mysqrt(help);
	if ( math_Abs(a) < ZEROBOUND) {
		temp = ( (-c) / b);
		if ( (temp >= MINT) && (temp <= MAXT) ) 
			(*t1) = math_iRound(fractf(temp)); 
		else 
			(*t1) = TINIT;
		(*t2) = TINIT;							/* just for safety */
	} else {
		temp = ( ((-b) + sqroot)/(2*a) );
		if ( (temp >= MINT) && (temp <= MAXT) ) 
			(*t1) = math_iRound(fractf(temp)); 
		else 
			(*t1) = TINIT;
		if ( math_Abs(help) < SQRTBOUND ) 		/* only have one value */
			(*t2) = TINIT;
		else {
			temp = ( ((-b) - sqroot)/(2*a) );
			if ( (temp >= MINT) && (temp <= MAXT) ) 
				(*t2) = math_iRound(fractf(temp)); 
			else 
				(*t2) = TINIT;
		}
	}
}

/*---------------------------------------------------------------------*/
/* 
	Checks that the given parameter t is a real inflection point for the
	bezier, by verifying that the curvature is zero at this point.
	f(t) = t*t*t(-A + 3B -3C + D) + 3*t*t(A -2B + C) + 3t(B - A) + A
	f'(t) = 3*t*t(-A + 3B - 3C + D) + 6t(A -2B + C) + 3(B - A)
	f''(t) = 6t(-A + 3B -3C + D) + 6(A -2B + C)

		Curvature = x'y'' - y'x'' = 0
*/

bool	arc_BezierVerifyInflection(b,t)
arc_fxBezier	*b;
fract	t;
{
	double	ax, bx, cx, dx, ay, by, cy, dy, dblt;
	double	xprime, xdblprime, yprime, ydblprime;
	double	curvature;

	ax = (double)(floatfr(b->a.x)); bx = (double)(floatfr(b->b.x));
	cx = (double)(floatfr(b->c.x)); dx = (double)(floatfr(b->d.x));
	ay = (double)(floatfr(b->a.y)); by = (double)(floatfr(b->b.y));
	cy = (double)(floatfr(b->c.y)); dy = (double)(floatfr(b->d.y));
	dblt = (double)(floatfr(t));

	xprime = ( (3*dblt*dblt*((-ax) + (3*bx) - (3*cx) + dx))
						+ (6*dblt*(ax - (2*bx) + cx)) + (3*(bx - ax)) );
	xdblprime = ( (6*dblt*((-ax) + (3*bx) - (3*cx) + dx)) 
											+ (6*(ax - (2*bx) + cx)) );
	yprime = ( (3*dblt*dblt*((-ay) + (3*by) - (3*cy) + dy))
						+ (6*dblt*(ay - (2*by) + cy)) + (3*(by - ay)) );
	ydblprime = ( (6*dblt*((-ay) + (3*by) - (3*cy) + dy)) 
											+ (6*(ay - (2*by) + cy)) );
	curvature = ( (xprime*ydblprime) - (yprime*xdblprime) );
	if ( math_Abs(curvature) < 0.1 )
		return(true);
	else
		return(false);
}
	
/*---------------------------------------------------------------------*/
/*
	Given a bezier, it updates the t parameters in the global arc_tarray.
	The old t values referred to another bezier and are not correct for
	this one, but the slopes remain correct. The first t value is not 
	used, since the curve has already been split at that point. It uses 
	dydx = dydt/dxdt to get a quadratic for t, which is then solved to
	give two values of t. The values of t that do not satisfy 0 <= t <= 1 .
	are rejected. If there are two values of slopes equal to zero or 
	infinity, only one value is used, since the quadratic will give the 
	two values of t that are needed. The important thing is that there
	is one less value of t when the function returns, since the curve
	has already been split at one place.
*/

void	arc_BezierUpdatet(b)
arc_fxBezier	*b;
{
	int32	i,cnt;
	double	t1,t2;
	arc_pointinfo	temp[6];
	bool	horizontal,vertical,found;

	i = 1; cnt = 0;
	horizontal = vertical = found = false;

	/* no longer need first entry, already been used */
	arc_tarray[0].t = TINIT; arc_tarray[0].sn = TINIT; arc_tarray[0].sd = TINIT;
	while (arc_tarray[i].t != TINIT) {
		if (arc_tarray[i].sn == 0) {				/* horizontal */
				if (!horizontal) {
					horizontal = true;
					arc_BezierTFromSlope(arc_tarray[i].sn,arc_tarray[i].sd,b,&t1,&t2);
					if (t1 != TINIT) {
						temp[cnt].t = t1; temp[cnt].sn = 0; temp[cnt++].sd = FRONE;
					}
					if (t2 != TINIT) {
					 	temp[cnt].t = t2; temp[cnt].sn = 0; temp[cnt++].sd = FRONE;
					}
				}
		} else if (arc_tarray[i].sd == 0) {			/* vertical */
				if (!vertical) {
					vertical = true;
					arc_BezierTFromSlope(arc_tarray[i].sn,arc_tarray[i].sd,b,&t1,&t2);
					if (t1 != TINIT) {
						temp[cnt].t = t1; temp[cnt].sn = FRONE; temp[cnt++].sd = 0;
					}
					if (t2 != TINIT) {
						temp[cnt].t = t2; temp[cnt].sn = FRONE; temp[cnt++].sd = 0;
					}
				}
		} else {								/* inflection */
			arc_BezierTFromSlope(arc_tarray[i].sn,arc_tarray[i].sd,b,&t1,&t2);
			if (t1 != TINIT) {
				if(arc_BezierVerifyInflection(b,t1)) {
					found = true;
					temp[cnt].t = t1; temp[cnt].sn = arc_tarray[i].sn;
					temp[cnt++].sd = arc_tarray[i].sd;
				}
			}
			if (!found) {
				if (t2 != TINIT) {
					if(arc_BezierVerifyInflection(b,t2)) {
						temp[cnt].t = t2; temp[cnt].sn = arc_tarray[i].sn;
						temp[cnt++].sd = arc_tarray[i].sd;
					}
				}
			}
		}
	i++;
	}

	if (cnt > 0) {
		for (i = 0;i < cnt;i++) {
			arc_tarray[i].t = temp[i].t; arc_tarray[i].sn = temp[i].sn;			
			arc_tarray[i].sd = temp[i].sd;			
		}
		for (i = cnt; i < 6; i++) {	
			arc_tarray[i].t = TINIT; arc_tarray[i].sn = TINIT;
			arc_tarray[i].sd = TINIT;
		}
	}
}

/*---------------------------------------------------------------------*/
/*
	Takes a bezier curve, splits it up into segments and puts them into
	the arc_stack. The curve is traversed correctly if the segments are
	taken from the top of the stack.
*/

void	arc_BezierSplitAndFillStack(bezier)
arc_fxBezier	*bezier;
{
	arc_fxBezier	*btosplit;
	arc_fxBezier	b1,b2;
	fract			t;
	bool			split;
	int32			num;

	split = false;
	btosplit = bezier;
	arc_Inittarray();
	num = arc_BezierComputeQuadrants(bezier);
	/* put t values in descending order so arc_stack is correct */
	arc_Sorttarray(num);
	while (arc_tarray[0].t != TINIT) {
		split = true;
		t = arc_tarray[0].t;
		arc_BezierSplitAtPoint(btosplit,t,&b1,&b2);
		arc_BezierPushStack(&b2);		  /* put second curve on stack */
		btosplit = &b1;			  /* continue to split the first curve */
		arc_BezierUpdatet(&b1);	/* make correct arc_tarray for this bezier */
		num--;
		arc_Sorttarray(num);				 /* new arc_tarray, so re-sort */
	}	
	if (split) 
		arc_BezierPushStack(&b1);		/* push first curve on the top */
	else
		arc_BezierPushStack(bezier);	/* push original curve on the top */
}

/*---------------------------------------------------------------------*/
/*
	Determines whether a bezier curve can be approximated by a conic, 
	by testing whether the lines ad and bc are nearly parallel or not.
	A bezier with A=B or C=D can never be approximated by a conic, only
	by a straight line.
*/

bool	arc_BezierIsNearConic(bez)
arc_fxBezier	*bez;
{
	pair_dXY	a,b,c,d,ad,bc;
	line_dLine	AD;
	double		BinAD,CinAD,temp,nad,nbc;

	pair_frXYTodXY(&a,&bez->a); pair_frXYTodXY(&b,&bez->b);
	pair_frXYTodXY(&c,&bez->c); pair_frXYTodXY(&d,&bez->d);
	pair_XYSubtract(&ad,&d,&a);
	pair_XYSubtract(&bc,&c,&b);
	if ( ((ad.x*bc.x)<0.0) || ((ad.y*bc.y)<0.0) )
		return(false);
	/* test whether AD crosses BC or not */
	line_dLineFrom2Points(&a,&d,&AD);
	/* Evaluate AD at b and c, check that the signs are the same. */
	BinAD = line_dLineEvaluation(&AD,&b);
	CinAD = line_dLineEvaluation(&AD,&c);
	if ( ((BinAD < 0) && (CinAD > 0)) || ((BinAD > 0) && (CinAD < 0)) )
		return(false);

	nad = math_DoubleAbs((ad.x*ad.x)+(ad.y*ad.y));
	nbc = math_DoubleAbs((bc.x*bc.x)+(bc.y*bc.y));
	temp = math_DoubleAbs((ad.y*bc.x)-(ad.x*bc.y));

	return((temp*temp)<(BEZIERPARALLELTOL*nad*nbc));
}

/*---------------------------------------------------------------------*/
/*
	Computes an arc of 3D parabola whose 2D projection approximates a 
	bezier arc and then converts it to a 2D arc. It assumes that the
	bezier arc given as argument is reasonably close to a 2D parabola
	or at least that the lines ad and bc are nearly parallel. The 3D arc
	of parabola is computed so that it has the appropriate endpoints
*/

void	arc_BezierConvertTo2DConic(bezier,conic2D)
arc_fxBezier	*bezier;
arc_fxConic		*conic2D;
{
	line_dLine	ab,cd;
	pair_dXY	a,b,c,d,conicb;
	double		dab,dap,dpd,dcd,sh;
	int32		cnt;
	
	pair_frXYTodXY(&a,&bezier->a); pair_frXYTodXY(&b,&bezier->b);
	pair_frXYTodXY(&c,&bezier->c); pair_frXYTodXY(&d,&bezier->d);
	pair_frXYTodXY(&conicb,&conic2D->b);
	conic2D->a = bezier->a; 
	conic2D->c = bezier->d; 
	line_dLineFrom2Points(&a,&b,&ab);
	line_dLineFrom2Points(&d,&c,&cd);
	line_dLineIntersection(&ab,&cd,&conicb);
	dap = max(math_DoubleAbs(conicb.y - a.y),math_DoubleAbs(conicb.x - a.x));
	dpd = max(math_DoubleAbs(conicb.y - d.y),math_DoubleAbs(conicb.x - d.x));
	dab = max(math_DoubleAbs(b.y - a.y),math_DoubleAbs(b.x - a.x));
	dcd = max(math_DoubleAbs(d.y - c.y),math_DoubleAbs(d.x - c.x));
	cnt = 0;
	sh = 0.0;
	if (dap != 0.0) {
		sh += dab/(((4.0/3.0)*dap) - dab);
		cnt += 1;
	} 
	if (dpd != 0.0) {
		sh += dcd/(((4.0/3.0)*dpd) - dcd);
		cnt += 1;
	}
	conic2D->b.x = (fractf(conicb.x)); conic2D->b.y = (fractf(conicb.y));
	conic2D->sh2 = (cnt == 0)?(fracti(1)):(fractf(math_Square(sh/cnt)));
}

/*---------------------------------------------------------------------*/
/*
	Determines whether a bezier is degenerate or not. A degenerate
	bezier is defined to have A=B or C=D. In this case it can only be
	approximated by a line.
*/

bool	arc_BezierIsDegenerate(bezier)
arc_fxBezier	*bezier;
{
	pair_frXY	diff;

	pair_frSubtract(&diff,&bezier->a,&bezier->b);
	if ((diff.x == 0) && (diff.y == 0))
		return(true);
	pair_frSubtract(&diff,&bezier->c,&bezier->d);
	if ((diff.x == 0) && (diff.y == 0))
		return(true);
	return(false);
}

/*---------------------------------------------------------------------*/
/*
	Determines whether a bezier curve can be approximated by a line. 
	The bezier must be degenerate, that is A=B or C=D, and it can be 
	approximated by a line if:
		if A=B, C has to be less than BEZIERLINETOL away
			from AD.
		if C=D, B has to be less than BEZIERLINETOL away
			from AD.
*/

bool	arc_BezierIsNearLine(bez)
arc_fxBezier	*bez;
{
	line_dLine	line;
	pair_dXY	a,b,c,d;
	double		ptInLine;

	pair_frXYTodXY(&a,&bez->a); pair_frXYTodXY(&b,&bez->b);
	pair_frXYTodXY(&c,&bez->c); pair_frXYTodXY(&d,&bez->d);
	line_dLineFrom2Points(&a,&d,&line);
	if ((bez->a.x == bez->b.x)&&(bez->a.y == bez->b.y)) {  /* A=B */
		ptInLine = line_dLineEvaluation(&line,&c);
		if ( (math_DoubleAbs(ptInLine)) < BEZIERLINETOL )
			return(true);
		else
			return(false);
	} else { 												/* C=D */
		ptInLine = line_dLineEvaluation(&line,&b);
		if ( (math_DoubleAbs(ptInLine)) < BEZIERLINETOL )
			return(true);
		else
			return(false);
	}
}

/*---------------------------------------------------------------------*/
/*
	Computes a line that can be used to approximate the bezier. It
	assumes that the bezier has A=B / C=D and that the point C / B is close
	enough to the line AD.
*/

void	arc_BezierConvertToLine(bezier,line)
arc_fxBezier	*bezier;
arc_fxLine		*line;
{
	line->from = bezier->a;
	line->to = bezier->d;
	line->disp = bezier->disp;
	line->shift = bezier->shift;
}

/*---------------------------------------------------------------------*/
/*
	The following are all the routines that manipulate lines.
*/

/*---------------------------------------------------------------------*/
/* Splits a line at the parameter t. */

void	arc_LineSplitInHalf(l,l1,l2)
arc_fxLine	*l,*l1,*l2;
{
	pair_frXY	splitpt;
	arc_fxLine	line;

	line = *l;
	pair_frAdd(&line.from,&line.from,&line.disp);
	pair_frAdd(&line.to,&line.to,&line.disp);
	splitpt.x = frmul(FRHALF,(fradd(line.to.x,line.from.x)));
	splitpt.y = frmul(FRHALF,(fradd(line.to.y,line.from.y)));
	l1->from = line.from; l1->to = splitpt;
	l2->from = splitpt; l2->to = line.to;
	l1->disp.x = 0; l1->disp.y = 0; l2->disp.x = 0; l2->disp.y = 0;
	l1->shift = line.shift; l2->shift = line.shift;
}

/*---------------------------------------------------------------------*/
/* Pushes one line onto the stack. */

void	arc_LinePushStack(line)
arc_fxLine	*line;
{
	arc_stacktop->type = arc_LINETYPE;	
	arc_stacktop->data.line = *line;  /* the actual curve */
	arc_GetFxArcBBox(arc_stacktop,&arc_stacktop->bbox);
	arc_stacktop++;
}

/*---------------------------------------------------------------------*/
/*
	Takes a line and, since it does not need to split it, pushes it onto
	the arc_stack.
*/

void	arc_LineFillStack(line)
arc_fxLine	*line;
{
	arc_LinePushStack(line);
}

/*---------------------------------------------------------------------*/
/*
	The following are all the routines that manipulate conics.
*/
/*---------------------------------------------------------------------*/
/* converts a point in 2D to a point in 3D */

void	arc_ConicPoint2DTo3D(p2,p3)
pair_frXY		*p2;
arc_fxpoint3D	*p3;
{
	p3->x = p2->x;
	p3->y = p2->y;
	p3->z = FRONE; 
}

/*---------------------------------------------------------------------*/
/*	Scales a 3D point by the given scalefactor. */

void	arc_ConicScale3DPoint(p,f)
arc_fxpoint3D	*p;
fract		f;
{
	p->x = frmul(p->x,f);
	p->y = frmul(p->y,f);
	p->z = frmul(p->z,f);
}

/*---------------------------------------------------------------------*/
/*
	Translates the 3D conic so that the point A is at (0,0,1), and sets
	the conic's displacement accordingly.
*/

void	arc_ConicTranslate3DConic(c3)
arc_fxConic3D	*c3;
{
	pair_frXY	bdisp;

	c3->disp.x = c3->a.x; c3->disp.y = c3->a.y; c3->a.x = 0; c3->a.y = 0;
	bdisp.x = frmul(c3->disp.x,c3->b.z);
	bdisp.y = frmul(c3->disp.y,c3->b.z);
	pair_frSubtract(&c3->b,&c3->b,&bdisp);
	pair_frSubtract(&c3->c,&c3->c,&c3->disp);
}

/*---------------------------------------------------------------------*/
/* 
	Returns the minimum shift needed, leaving room to multiply by the 
	sharpness + extra 2 bits for additional arithmetic if the conic has
	big numbers.
*/
int32	arc_GetShiftForConic(sh,maxnum)
fract	sh;
fract	maxnum;
{
	int32	cnt,bits,shbits,maxnumbits;
	fract	limit;

	shbits = maxnumbits = 0;
	cnt = (maxnum < FR8192) ? 0 : 2;
	limit = FRONE;
	while (sh >= limit) {
		limit = frlsh(limit,1);
		shbits++;
	}
	limit = FRONE;
	while (maxnum >= limit) {
		limit = frlsh(limit,1);
		maxnumbits++;
	}
	bits = shbits + maxnumbits;
	if (bits <= MAXBITS)
		return(cnt);
	return(cnt + bits - MAXBITS);
}

/*---------------------------------------------------------------------*/
/*	Converts a 2D conic into a 3D arc of parabola */

void	arc_Conic2DTo3D(c2,c3)
arc_fxConic	*c2;
arc_fxConic3D	*c3;
{
	arc_fxConic	conic2D;
	fract	sh, maxnum;
	int32	rshift;

	conic2D = *c2;
	sh = frsqrt(conic2D.sh2);
	maxnum = arc_MaxOf6(conic2D.a.x,conic2D.a.y,conic2D.b.x,
							conic2D.b.y,conic2D.c.x,conic2D.c.y);
	rshift = arc_GetShiftForConic(sh,maxnum);
	/* shift conic if necessary */
	pair_frShiftRight(&conic2D.a,&conic2D.a,rshift);
	pair_frShiftRight(&conic2D.b,&conic2D.b,rshift);
	pair_frShiftRight(&conic2D.c,&conic2D.c,rshift);
	/* conic 2D -> 3D */
	arc_ConicPoint2DTo3D(&conic2D.a,&c3->a);
	arc_ConicPoint2DTo3D(&conic2D.b,&c3->b);
	arc_ConicPoint2DTo3D(&conic2D.c,&c3->c);
	arc_ConicScale3DPoint(&c3->b,sh);
	c3->shift = rshift;
	/* translate so that the 3D conic refers to (0,0,1) */
	arc_ConicTranslate3DConic(c3);
}	

/*---------------------------------------------------------------------*/
/* project a 3D point into 2D */ 

void	arc_Point3DTo2D(pt3D,pt2D,disp)
arc_fxpoint3D	*pt3D;
pair_frXY		*pt2D;
pair_frXY		*disp;
{
	pt2D->x = fradd((frdiv((pt3D->x),(pt3D->z))),disp->x);
	pt2D->y = fradd((frdiv((pt3D->y),(pt3D->z))),disp->y);
}

/*---------------------------------------------------------------------*/
/*
	Ensures that B's value is between A and C. i.e.
				A <= B <= C      or A >= B >= C
*/

void	arc_BBetweenAC(A,B,C)
fract	A,*B,C;
{
	fract	arc_min, arc_max;

	if (A < C) {
		arc_min = A; arc_max = C;
	} else {
		arc_min = C; arc_max = A;
	}
	if (*B > arc_max)
		*B = arc_max;
	if (*B < arc_min)
		*B = arc_min;
}

/*---------------------------------------------------------------------*/
/*
	Projects a 3D arc of parabola onto a 2D conic, always contained in 
	a quadrant.
*/

void	arc_Conic3DTo2D(c3,c2)
arc_fxConic3D	*c3;
arc_fxConic		*c2;
{
	c2->sh2 = frsq(c3->b.z);
	pair_frAdd((&c2->a),(&c3->a),(&c3->disp));
	c2->b.x = (fradd(frdiv(c3->b.x,c3->b.z),c3->disp.x));
	c2->b.y = (fradd(frdiv(c3->b.y,c3->b.z),c3->disp.y));
	pair_frAdd((&c2->c),(&c3->c),(&c3->disp));
	c2->disp.x = 0; c2->disp.y = 0;
	c2->shift = c3->shift;
	arc_BBetweenAC(c2->a.x,&c2->b.x,c2->c.x);
	arc_BBetweenAC(c2->a.y,&c2->b.y,c2->c.y);
}

/*---------------------------------------------------------------------*/
/*
	Splits a 3D parabolic arc into two 3D parabolic arcs, c1 and c2, at
	a specified point. The point of division is specified by a number t
	between 0 and 1 which yields the desired point when plugged into the
	parametric equation of the parabola given below.
			A(1-t)(1-t) + B(1-t)t + Ctt
*/

void	arc_ConicSplitInHalf(c0,c1,c2)
arc_fxConic3D	*c0,*c1,*c2;
{
	fract	root,root_times_2;			/* sqrt(1+Bz), sqrt(2*(1+bz)) */
	fract	one_BZ,two_one_BZ;					  /* (1+Bz), 2*(1+Bz) */
	pair_frXY	bdisp;
	arc_fxConic3D	cc,*c;
	
	cc = *c0; c = &cc;
	root = frsqrt(fradd(FRONE,c->b.z));
	root_times_2 = frmul(FRSQRT2,root);
	one_BZ = fradd(FRONE,c->b.z);
	two_one_BZ = frmul(FR2,one_BZ);
	/* split into two 3D conics and normalize */
	c1->a = c->a;
	c1->b.x = frdiv(fradd(c->a.x,c->b.x),root_times_2);
	c1->b.y = frdiv(fradd(c->a.y,c->b.y),root_times_2);
	c1->b.z = frdiv(root,FRSQRT2);
	c1->c.x = frdiv(fradd(fradd(c->a.x,c->c.x),frmul(FR2,c->b.x)),two_one_BZ);
	c1->c.y = frdiv(fradd(fradd(c->a.y,c->c.y),frmul(FR2,c->b.y)),two_one_BZ);
	c1->c.z = FRONE;
	c2->a = c1->c;
	c2->b.x = frdiv(fradd(c->b.x,c->c.x),root_times_2);
	c2->b.y = frdiv(fradd(c->b.y,c->c.y),root_times_2);
	c2->b.z = c1->b.z;
	c2->c = c->c;
	/* translate so that the 3D conics refer to (0,0,1) */
	c1->disp = c->disp;
	c2->disp.x = (c->disp.x + c2->a.x); c2->disp.y = (c->disp.y + c2->a.y);
	bdisp.x = frmul(c2->a.x,c2->b.z);
	bdisp.y = frmul(c2->a.y,c2->b.z);
	pair_frSubtract(&c2->b,&c2->b,&bdisp);
	pair_frSubtract(&c2->c,&c2->c,&c2->a);
	c2->a.x = 0; c2->a.y = 0;

	c1->shift = c0->shift;
	c2->shift = c0->shift;
	arc_AdjustFxConic3DShift(c1);
	arc_AdjustFxConic3DShift(c2);
}
		
/*---------------------------------------------------------------------*/
/*
	Given the values for A,B,C in a 2D conic, this function will calculate  
	the parameter t at the points where the derivative with respect to
	t is zero. It can be passed either the x co-ords or the y co-ords
	for a curve. The solution presented in Vaughan Pratt's paper is used. 
	The solution for horizontal and vertical tangents is when
		sh(A - B)ss + (A - C)st + sh(B - C)tt = 0
	for A, B, C either x or y values, s+t = 1, and sh is the sharpness.
	There can be only one value for t; since a conic cannot have more 
	than one horizontal and one vertical tangent. Solutions for t that do
	not satisfy  0 <= t <= 1  will be discarded. The function returns 
	the number of solutions of t that it found.
*/

int32	arc_ConicGetTangents(A,B,C,S,t)
double	A,B,C,S;
fract	*t;
{
	double	a, b, c, sqroot, help;
	double	tempt;
	fract	*tptr;
	int32	count;
	
	tptr = t;
	count = 0;
	a = ( (1 - S) * (C - A) );
	b = ( A*(1 - (2*S)) + 2*S*B - C );
	c = ( S*(A - B) );
	if ( math_Abs(a) < ZEROBOUND ) {		 /* symmetrical, so linear */
		if ( math_Abs(b) < ZEROBOUND )		/* no solutions */
			return(count);
		tempt = ( (-c) / b );
		if ( (tempt >= MINT) && (tempt <= MAXT) ) {
			(*tptr) = math_iRound(fractf(tempt));
			tptr++; count++;
		}
	} else {
		help = ( (b*b) - (4*a*c) );
		if ( math_Abs(help) < SQRTBOUND )
			sqroot = 0.0;
		else {
			if (help > SQRTBOUND)
				sqroot = mysqrt(help);
			else
				return(count);
		}
		tempt = ( ((-b) + sqroot) / (2*a) );
		if ( (tempt >= MINT) && (tempt <= MAXT) ) {
			(*tptr) = math_iRound(fractf(tempt));
			tptr++; count++;
		}
		tempt = ( ((-b) - sqroot) / (2*a) );
		if ( (tempt >= MINT) && (tempt <= MAXT) ) {
			(*tptr) = math_iRound(fractf(tempt));
			tptr++; count++;
		}
	}
	return(count);
}	

/*---------------------------------------------------------------------*/
/*
	Given a conic, it returns, in the global array arc_tarray, the values 
	of the parameter t for which the curve has a horizontal tangent or a
	vertical tangent. These values must be 0 and 1; there can be at most 
	2 values for any conic. This is all done so that a curve will lie in 
    one quadrant, and go in one direction. It returns the number of t's
	it found.
*/

int32	arc_ConicComputeQuadrants(A,B,C,S2)
pair_dXY	*A,*B,*C;
fract		S2;
{
	fract		tempt[2], sh;
	double		dblsh;
	int32		count,i,j,keep;

	count = keep = i = 0;
	sh = frsqrt(S2);
	dblsh = (double)(floatfr(sh));
	arc_Inittarray();
	/* get the vertical tangent, if any */
	count = arc_ConicGetTangents(A->x,B->x,C->x,dblsh,tempt);
	keep = count;
	j = 0;
	while (count) {
		arc_tarray[i].t = tempt[j];
		arc_tarray[i].sn = FRONE;
		arc_tarray[i].sd = 0;
		i++; j++; count--;
	}
	/* get the horizontal tangent, if any */
	count = arc_ConicGetTangents(A->y,B->y,C->y,dblsh,tempt);
	keep += count;
	j = 0;
	while (count) {
		arc_tarray[i].t = tempt[j];
		arc_tarray[i].sn = 0;
		arc_tarray[i].sd = FRONE;
		i++; j++; count--;
	}
	return(keep);
}

/*---------------------------------------------------------------------*/
/* Pushes one 2D conic segment onto the stack. */

void	arc_Conic2DPushStack(conic)
arc_fxConic		*conic;
{
	arc_stacktop->type = arc_CONICTYPE;		
	arc_stacktop->data.conic = *conic;   /* the actual curve */
	arc_GetFxArcBBox(arc_stacktop,&arc_stacktop->bbox);
	arc_stacktop++;
}
/*---------------------------------------------------------------------*/
/* Pushes one 3D conic segment onto the stack. */

void	arc_Conic3DPushStack(conic)
arc_fxConic3D		*conic;
{
	arc_stacktop->type = arc_CONIC3DTYPE;		
	arc_stacktop->data.conic3D = *conic;   /* the actual curve */
	arc_GetFxArcBBox(arc_stacktop,&arc_stacktop->bbox);
	arc_stacktop++;
}

/*---------------------------------------------------------------------*/
/*
	Evaluate function at the given parameter.
	Function:
				A(1-t)(1-t) + Bt(1-t) +Ctt
*/

fract	arc_EvalFunction(A,B,C,t)
fract	A,B,C,t;
{
	fract	answer;
	answer = fradd((frmul(A,frsq(frsub(FRONE,t)))),
					(fradd((frmul((frmul(B,FR2)),
					(frmul(t,frsub(FRONE,t))))),
					(frmul(C,frsq(t))))));
	return(answer);
}	

/*---------------------------------------------------------------------*/
/*
	Evaluate a point on the conic at the given parameter t.
	Equation for the conic is:
		A(1-t)(1-t) + Bt(1-t) +Ctt

*/

void	arc_GetPointOnConic(conic,t,pt)
arc_fxConic3D	*conic;
fract			t;
arc_fxpoint3D	*pt;
{
	pt->x = arc_EvalFunction(conic->a.x,conic->b.x,conic->c.x,t);
	pt->y = arc_EvalFunction(conic->a.y,conic->b.y,conic->c.y,t);
	pt->z = arc_EvalFunction(conic->a.z,conic->b.z,conic->c.z,t);
}

/*---------------------------------------------------------------------*/
/*
	Calculates the sharpness for the given conic from the endpoints and
	the control point and an extra point on the curve.
*/

void	arc_GetConicSharpness(conic,pt)
arc_fxConic	*conic;
pair_frXY	*pt;
{
	/* 
		The computation that follows is based on the fact that the
		quadratic form q*AB*BC+AC*AC obtained using the equations of the
		lines AB, BC and AC is exactly the same quadratic form that we 
		obtain using Vaughan Pratt's formulation with q replaced by 
		-4*SH*SH... Simple substitution with a lot of pencil and paper is 
		all it takes to show it.
	*/
	line_dLine	AB,BC,AC;
	double		ptinBC,ptinAB,ptinAC;
	pair_dXY	a,b,c,dblpt;
	
	pair_frXYTodXY(&a,&conic->a); pair_frXYTodXY(&b,&conic->b);
	pair_frXYTodXY(&c,&conic->c); pair_frXYTodXY(&dblpt,pt);
	line_dLineFrom2Points(&a,&b,&AB);
	line_dLineFrom2Points(&b,&c,&BC);
	line_dLineFrom2Points(&a,&c,&AC);
	ptinBC = line_dLineEvaluation(&BC,&dblpt);
	ptinAB = line_dLineEvaluation(&AB,&dblpt);
	ptinAC = line_dLineEvaluation(&AC,&dblpt);
	conic->sh2 = fractf((ptinAC*ptinAC)/(4*ptinAB*ptinBC));
}

/*---------------------------------------------------------------------*/
/*
	Takes a conic, splits it up into segments and puts them onto the 
	arc_stack. The curve is traversed correctly if the segments are taken
	from the top of the stack.
	NOTE :
		In order to split up the conic; it has to be converted into a 3D 
		arc of parabola, split up, and pushed onto the arc_stack. The use
		of the 3D arc of parabola need not be known or understood by the
		user.
*/ 

void	arc_ConicSplitAndFillStack(conic)
arc_fxConic	*conic;
{
	arc_fxConic3D	conic3D,c3D1,c3D2,c3D3;	
	arc_fxConic		c2D1,c2D2,c2D3;	/* push on stack in this order */
	arc_fxpoint3D	c3Dpt;					/* a curve point in 3D */
	pair_frXY		c2Dpt1,c2Dpt2,c2Dpt3;	 /* curve points in 2D */
	line_dLine		AB,BC;					  /* for the big conic */
	line_dLine		htan,vtan;	  /* horizontal, vertical tangents */
	arc_fxLine		line;
	fract			splitpoint;
	bool			threeconics,BnearA,BnearC;
	bbox_iBBox		bbox;
	int32			num;
	pair_dXY		conica,conicb,conicc,dblpt;

	/* check if in one quadrant */
	bbox.lox = min(conic->a.x,conic->c.x);
	bbox.hix = max(conic->a.x,conic->c.x);
	bbox.loy = min(conic->a.y,conic->c.y);
	bbox.hiy = max(conic->a.y,conic->c.y);
	/* if B is in bbox, within one quadrant */
	if ((conic->b.x >= bbox.lox)&&(conic->b.x <= bbox.hix)&&(conic->b.y >= bbox.loy)&&(conic->b.y <= bbox.hiy)) {
		arc_Conic2DPushStack(conic);
		return;
	}
	/* check if conic is degenerate (a line AC, A==B or B==C) */
	BnearA = ((math_Abs(frsub(conic->b.x,conic->a.x)) < DEGCONICBOUND) && 
			(math_Abs(frsub(conic->b.y,conic->a.y)) < DEGCONICBOUND));
	BnearC = ((math_Abs(frsub(conic->b.x,conic->c.x)) < DEGCONICBOUND) && 
			(math_Abs(frsub(conic->b.y,conic->c.y)) < DEGCONICBOUND));
	if ( BnearA || BnearC ) {
		line.from = conic->a; line.to = conic->c;
		line.disp = conic->disp; line.shift = conic->shift;
		arc_LinePushStack(&line);
		return;
	}
	threeconics = false;
	pair_frXYTodXY(&conica,&conic->a); pair_frXYTodXY(&conicb,&conic->b);
	pair_frXYTodXY(&conicc,&conic->c);
	num = arc_ConicComputeQuadrants(&conica,&conicb,&conicc,conic->sh2);
	/* put t values in descending order so arc_stack is correct */
	arc_Sorttarray(num);
	if (arc_tarray[0].t != TINIT) {
		arc_Conic2DTo3D(conic,&conic3D);
		splitpoint = frdiv(fradd(arc_tarray[0].t,FRONE),FR2);
		arc_GetPointOnConic(&conic3D,splitpoint,&c3Dpt);
		arc_Point3DTo2D(&c3Dpt,&c2Dpt1,&conic3D.disp);		/* pt on c2D1 */
		pair_frShiftLeft(&c2Dpt1,&c2Dpt1,conic3D.shift);
		splitpoint = (arc_tarray[0].t);
		arc_GetPointOnConic(&conic3D,splitpoint,&c3Dpt);
		arc_Point3DTo2D(&c3Dpt,&c2D1.a,&conic3D.disp);
		pair_frShiftLeft(&c2D1.a,&c2D1.a,conic3D.shift);
		c2D1.c = conic->c; c2D2.c = c2D1.a;
		if (arc_tarray[1].t != TINIT) {
			threeconics = true;
			splitpoint = frdiv(fradd(arc_tarray[1].t,arc_tarray[0].t),FR2);
			arc_GetPointOnConic(&conic3D,splitpoint,&c3Dpt);
			arc_Point3DTo2D(&c3Dpt,&c2Dpt2,&conic3D.disp);		/* pt on c2D2 */
			pair_frShiftLeft(&c2Dpt2,&c2Dpt2,conic3D.shift);
			splitpoint = (arc_tarray[1].t);
			arc_GetPointOnConic(&conic3D,splitpoint,&c3Dpt);
			arc_Point3DTo2D(&c3Dpt,&c2D3.c,&conic3D.disp);
			pair_frShiftLeft(&c2D3.c,&c2D3.c,conic3D.shift);
			splitpoint = frdiv((arc_tarray[1].t),FR2);
			arc_GetPointOnConic(&conic3D,splitpoint,&c3Dpt);
			arc_Point3DTo2D(&c3Dpt,&c2Dpt3,&conic3D.disp); 	/* pt on c2D3 */
			pair_frShiftLeft(&c2Dpt3,&c2Dpt3,conic3D.shift);
			c2D2.a = c2D3.c; c2D3.a = conic->a;
		} else {
			splitpoint = frdiv((arc_tarray[0].t),FR2);
			arc_GetPointOnConic(&conic3D,splitpoint,&c3Dpt);
			arc_Point3DTo2D(&c3Dpt,&c2Dpt2,&conic3D.disp);		/* pt on c2D2 */
			pair_frShiftLeft(&c2Dpt2,&c2Dpt2,conic3D.shift);
			c2D2.a = conic->a;
		}
		/* now get the B points for the conics */
		line_dLineFrom2Points(&conicb,&conicc,&BC);
		line_dLineFrom2Points(&conica,&conicb,&AB);
		pair_frXYTodXY(&dblpt,&c2D1.a);
		if (arc_tarray[0].sn == 0) {		/* horizontal tangent */
			line_dLineHorizontal(dblpt.y,&htan);
			line_dLineIntersection(&htan,&BC,&dblpt);
		} else {							/* vertical tangent */
			line_dLineVertical(dblpt.x,&vtan);
			line_dLineIntersection(&vtan,&BC,&dblpt);
		}
		pair_dXYTofrXY(&c2D1.b,&dblpt);
		if (threeconics) {
			pair_frXYTodXY(&dblpt,&c2D3.c);
			if (arc_tarray[1].sn == 0) {			/* horizontal tangent */
				line_dLineHorizontal(dblpt.y,&htan);
				line_dLineIntersection(&htan,&AB,&dblpt);
			} else {
				line_dLineVertical(dblpt.x,&vtan);
				line_dLineIntersection(&vtan,&AB,&dblpt);
			}
			pair_dXYTofrXY(&c2D3.b,&dblpt);
			line_dLineIntersection(&vtan,&htan,&dblpt);
		} else {
			if (arc_tarray[0].sn == 0)
				line_dLineIntersection(&htan,&AB,&dblpt);
			else
				line_dLineIntersection(&vtan,&AB,&dblpt);
		}
		pair_dXYTofrXY(&c2D2.b,&dblpt);
		/* get the sharpnesses, and push on stack */
		c2D1.disp.x = c2D1.disp.y = 0;
		c2D1.shift = 0;
		arc_GetConicSharpness(&c2D1,&c2Dpt1);
		arc_Conic2DPushStack(&c2D1);
		c2D2.disp.x = c2D2.disp.y = 0;
		c2D2.shift = 0;
		arc_GetConicSharpness(&c2D2,&c2Dpt2);
		arc_Conic2DPushStack(&c2D2);
		if (threeconics) {
			c2D3.disp.x = c2D3.disp.y = 0;
			c2D3.shift = 0;
			arc_GetConicSharpness(&c2D3,&c2Dpt3);
			arc_Conic2DPushStack(&c2D3);
		}
	} else {
		arc_Conic2DPushStack(conic);
	}
}

/*---------------------------------------------------------------------*/
/*
	General arc functions.
*/
/*---------------------------------------------------------------------*/
/*
	Given an arc in local fract representation , this function returns 
	its bounding box rounded to integers. The bounding box could be part
	of the arc's structure or a different bbox. The bounding box includes
	the displacement applied to the arc.
*/

void    arc_GetFxArcBBox(fxarc,bbox)
arc_fxSegment	*fxarc;
bbox_iBBox		*bbox;
{
    pair_iXY        p1,p2,p3,p4;

    switch(fxarc->type) {
        case arc_LINETYPE:
							p1.x = ( frlsh(((floorfr(fxarc->data.line.from.x)) +
								     		(floorfr(fxarc->data.line.disp.x))),
													fxarc->data.line.shift) );
							p1.y = ( frlsh(((floorfr(fxarc->data.line.from.y)) +
								     		(floorfr(fxarc->data.line.disp.y))),
													fxarc->data.line.shift) );
							p2.x = ( frlsh(((ceilingfr(fxarc->data.line.to.x)) +
								     		(ceilingfr(fxarc->data.line.disp.x))),
													fxarc->data.line.shift) );
							p2.y = ( frlsh(((ceilingfr(fxarc->data.line.to.y)) +
								     		(ceilingfr(fxarc->data.line.disp.y))),
													fxarc->data.line.shift) );
                            bbox_FillFrom2Points(bbox,&p1,&p2);
                            break;
        case arc_CONICTYPE:
							p1.x = ( frlsh(((floorfr(fxarc->data.conic.a.x)) +
								     		(floorfr(fxarc->data.conic.disp.x))),
													fxarc->data.conic.shift) );
							p1.y = ( frlsh(((floorfr(fxarc->data.conic.a.y)) +
								     		(floorfr(fxarc->data.conic.disp.y))),
													fxarc->data.conic.shift) );
							p2.x = ( frlsh(((ceilingfr(fxarc->data.conic.b.x)) +
								     		(ceilingfr(fxarc->data.conic.disp.x))),
													fxarc->data.conic.shift) );
							p2.y = ( frlsh(((ceilingfr(fxarc->data.conic.b.y)) +
								     		(ceilingfr(fxarc->data.conic.disp.y))),
													fxarc->data.conic.shift) );
							p3.x = ( frlsh(((roundfr(fxarc->data.conic.c.x)) +
								     		(roundfr(fxarc->data.conic.disp.x))),
													fxarc->data.conic.shift) );
							p3.y = ( frlsh(((roundfr(fxarc->data.conic.c.y)) +
								     		(roundfr(fxarc->data.conic.disp.y))),
													fxarc->data.conic.shift) );
                            bbox_FillFrom2Points(bbox,&p1,&p2);
                            bbox_iExpandToIncludePoint(bbox,&p3);
                            break;
        case arc_CONIC3DTYPE:
							bbox->lox = ( frlsh((floorfr(fxarc->data.conic3D.disp.x)),
												fxarc->data.conic3D.shift) );
							bbox->loy = ( frlsh((floorfr(fxarc->data.conic3D.disp.y)),
												fxarc->data.conic3D.shift) );
							bbox->hix = ( frlsh(((ceilingfr(fxarc->data.conic3D.c.x)) +
								     		(ceilingfr(fxarc->data.conic3D.disp.x))),
													fxarc->data.conic3D.shift) );
							bbox->hiy = ( frlsh(((ceilingfr(fxarc->data.conic3D.c.y)) +
								     		(ceilingfr(fxarc->data.conic3D.disp.y))),
													fxarc->data.conic3D.shift) );
                            break;
        case arc_BEZIERTYPE:
							p1.x = ( frlsh(((floorfr(fxarc->data.bezier.a.x)) +
								     		(floorfr(fxarc->data.bezier.disp.x))),
													fxarc->data.bezier.shift) );
							p1.y = ( frlsh(((floorfr(fxarc->data.bezier.a.y)) +
								     		(floorfr(fxarc->data.bezier.disp.y))),
													fxarc->data.bezier.shift) );
							p2.x = ( frlsh(((ceilingfr(fxarc->data.bezier.b.x)) +
								     		(ceilingfr(fxarc->data.bezier.disp.x))),
													fxarc->data.bezier.shift) );
							p2.y = ( frlsh(((ceilingfr(fxarc->data.bezier.b.y)) +
								     		(ceilingfr(fxarc->data.bezier.disp.y))),
													fxarc->data.bezier.shift) );
							p3.x = ( frlsh(((roundfr(fxarc->data.bezier.c.x)) +
								     		(roundfr(fxarc->data.bezier.disp.x))),
													fxarc->data.bezier.shift) );
							p3.y = ( frlsh(((roundfr(fxarc->data.bezier.c.y)) +
								     		(roundfr(fxarc->data.bezier.disp.y))),
													fxarc->data.bezier.shift) );
							p4.x = ( frlsh(((roundfr(fxarc->data.bezier.d.x)) +
								     		(roundfr(fxarc->data.bezier.disp.x))),
													fxarc->data.bezier.shift) );
							p4.y = ( frlsh(((roundfr(fxarc->data.bezier.d.y)) +
								     		(roundfr(fxarc->data.bezier.disp.y))),
													fxarc->data.bezier.shift) );
                            bbox_FillFrom2Points(bbox,&p1,&p2);
                            bbox_iExpandToIncludePoint(bbox,&p3);
                            bbox_iExpandToIncludePoint(bbox,&p4);
                            break;
    }
}
/*---------------------------------------------------------------------*/
/*
	Returns true if the arc is small enough not to overflow the integer
	arithmetic used in tracing arcs and false otherwise. 
*/

bool	arc_SimpleArc(fxarc)
arc_fxSegment	*fxarc;
{
	int32   xdim,ydim;                      /* dimensions of the arc */
	
	switch(fxarc->type) {
                case arc_LINETYPE:
						if (fxarc->data.line.shift != 0)
							return(false);
						xdim = (fxarc->bbox.hix - fxarc->bbox.lox);
						ydim = (fxarc->bbox.hiy - fxarc->bbox.loy);
                        if ( (max(xdim,ydim)) >= seg_MAXLINEDIMENSION )
							return(false);
                        break;
				case arc_CONICTYPE:
						if (fxarc->data.conic.shift != 0)
							return(false);
						xdim = (fxarc->bbox.hix - fxarc->bbox.lox);
						ydim = (fxarc->bbox.hiy - fxarc->bbox.loy);
                        if ( (max(xdim,ydim)) >= seg_MAXCONICDIMENSION )
							return(false);
                        break;
				case arc_CONIC3DTYPE:
						if (fxarc->data.conic3D.shift != 0)
							return(false);
						xdim = math_Abs(fxarc->data.conic3D.c.x);
						ydim = math_Abs(fxarc->data.conic3D.c.y);
                        if ( (max(xdim,ydim)) >= fx_seg_MAXCONICDIMENSION )
                            return(false);
                        break;
				case arc_BEZIERTYPE:
                        return(false);
                        break;
	}
	return(true);
}
	
/*---------------------------------------------------------------------*/
/* 
	Checks the arc on the top of the stack. If pop is true, it will 
	actually pop the top element, otherwise, it will just return a 
	copy of it to the caller. A pop also returns the top element to
	the caller.
*/

bool    arc_CheckFxarcStack(arc,pop)
arc_fxSegment     *arc;
bool            pop;
{
	if (arc_stacktop == arc_stack)
		return(false);
	*arc = *(--arc_stacktop);
	if (!pop)
		arc_stacktop++;			/* don't pop the stack */
	return(true);
}
/*---------------------------------------------------------------------*/
/* 
	Will return the arc on the top of the stack to the caller. This arc
	will be in 16/16 fixed point, contained in a quadrant and small 
	enough not to overflow the integer arithmetic used in tracing arcs.
	It will return false when there are no more arcs on the stack, and
	true otherwise.
*/

bool	arc_PopStack(frarc,disp)
arc_frSegment	*frarc;
pair_iXY		*disp;
{
	arc_fxSegment	fxarc;
	arc_fxConic		conic;
	arc_fxLine		line;
	if (arc_stacktop == arc_stack)
		return(false);
	else {
		arc_CheckFxarcStack(&fxarc,FALSE);
		switch(fxarc.type) {
			case arc_LINETYPE:
					while(!arc_SimpleArc(&fxarc)) {
						arc_SplitStackTop();
						arc_CheckFxarcStack(&fxarc,FALSE);
					}
					break;
			case arc_CONICTYPE:
					while(!arc_SimpleArc(&fxarc)) {
						arc_SplitStackTop();
						arc_CheckFxarcStack(&fxarc,FALSE);
					}
					if (fxarc.type == arc_CONIC3DTYPE) {
						arc_Conic3DTo2D(&fxarc.data.conic3D,&conic);
						fxarc.type = arc_CONICTYPE;
						fxarc.data.conic = conic;
					}
					break;
			case arc_CONIC3DTYPE:
					while(!arc_SimpleArc(&fxarc)) {
						arc_SplitStackTop();
						arc_CheckFxarcStack(&fxarc,FALSE);
					}
					arc_Conic3DTo2D(&fxarc.data.conic3D,&conic);
					fxarc.type = arc_CONICTYPE;
					fxarc.data.conic = conic;
					break;
			case arc_BEZIERTYPE:
					if (!arc_BezierIsDegenerate(&fxarc.data.bezier)) {
						while (!arc_BezierIsNearConic(&fxarc.data.bezier)) {
   							arc_SplitStackTop();
							arc_CheckFxarcStack(&fxarc,FALSE);
						}
						arc_BezierConvertTo2DConic(&fxarc.data.bezier,&conic);
						fxarc.type = arc_CONICTYPE;
						fxarc.data.conic = conic;
					} else {		/* A=B or C=D */
						while (!arc_BezierIsNearLine(&fxarc.data.bezier)) {
   							arc_SplitStackTop();
							arc_CheckFxarcStack(&fxarc,FALSE);
						}
						arc_BezierConvertToLine(&fxarc.data.bezier,&line);
						fxarc.type = arc_LINETYPE;
						fxarc.data.line = line;
					}
					break;
		}
		arc_GetFxArcBBox(&fxarc,&fxarc.bbox);
		arc_FxArcToFrArc(&fxarc,frarc,disp);
		arc_CheckFxarcStack(&fxarc,TRUE);	/* pop top of stack */
		return(true);
	}
}

/*---------------------------------------------------------------------*/
/*
	This function will divide the arc on the top of the stack into two
	arcs of equal size, and pushes them back onto the stack.
*/

void	arc_SplitStackTop()
{
	arc_fxSegment	oldarc,newarc1,newarc2;
	arc_fxConic3D	oldconic3D,new3D1,new3D2;

		arc_CheckFxarcStack(&oldarc,(bool)true);			/* pop the top arc */
		switch(oldarc.type) {
			case arc_LINETYPE:
						newarc1.type = newarc2.type = arc_LINETYPE;
						arc_LineSplitInHalf(&oldarc.data.line,
								&newarc1.data.line,&newarc2.data.line);
						arc_AdjustFxLineShift(&newarc2.data.line);
						arc_AdjustFxLineShift(&newarc1.data.line);
						arc_LinePushStack(&newarc2.data.line);
						arc_LinePushStack(&newarc1.data.line);
						break;
			case  arc_CONICTYPE:
						newarc1.type = newarc2.type = arc_CONIC3DTYPE;
						arc_Conic2DTo3D(&oldarc.data.conic,&oldconic3D);
						arc_ConicSplitInHalf(&oldconic3D,&new3D1,&new3D2);
						arc_Conic3DPushStack(&new3D2);
						arc_Conic3DPushStack(&new3D1);
						break;
			case  arc_CONIC3DTYPE:
						newarc1.type = newarc2.type = arc_CONIC3DTYPE;
						arc_ConicSplitInHalf(&oldarc.data.conic3D,
													 &new3D1,&new3D2);
						arc_Conic3DPushStack(&new3D2);
						arc_Conic3DPushStack(&new3D1);
						break;
			case arc_BEZIERTYPE:
						newarc1.type = newarc2.type = arc_BEZIERTYPE;
						arc_BezierSplitAtPoint(&oldarc.data.bezier,FRHALF,
								&newarc1.data.bezier,&newarc2.data.bezier);
						arc_AdjustFxBezierShift(&newarc1.data.bezier);
						arc_BezierPushStack(&newarc2.data.bezier);
						arc_BezierPushStack(&newarc1.data.bezier);
						break;
		}
}

/*---------------------------------------------------------------------*/
/*
	Takes an arc, splits it up if necessary, and puts the segments on the
	stack. At any time, the stack will have segments of one type of curve;
	bezier, conic or line.
*/

void	arc_SplitArcAndFillStack(arc,LShift)
arc_frSegment	*arc;
int8			LShift;
{
	arc_fxSegment	fxarc;
	arc_FrArcToFxArc(arc,&fxarc,LShift);
	arc_stacktop = arc_stack;			/* initialize arc_stack */
	switch(fxarc.type) {
		case arc_LINETYPE:
					arc_LineFillStack(&fxarc.data.line);
					break;
		case arc_CONICTYPE:
					arc_ConicSplitAndFillStack(&fxarc.data.conic);
					break;
		case arc_BEZIERTYPE:
					arc_BezierSplitAndFillStack(&fxarc.data.bezier);
					break;
	}
}
	
/*---------------------------------------------------------------------*/
/* 
	Given an arc, this function returns its endpoints.
*/

void	arc_GetArcEndpoints(arc,p1,p2)
arc_frSegment	*arc;
pair_frXY		*p1;	
pair_frXY		*p2;	
{
	switch(arc->type) {
		case arc_LINETYPE:
					*p1 = arc->data.line.from;
					*p2 = arc->data.line.to;
					break;
		case arc_CONICTYPE:
					*p1 = arc->data.conic.a;
					*p2 = arc->data.conic.c;
					break;
		case arc_BEZIERTYPE:
					*p1 = arc->data.bezier.a;
					*p2 = arc->data.bezier.d;
					break;
	}
}

/*---------------------------------------------------------------------*/
/*
	Given an arc, this function returns the slopes at the endpoints.
*/

void	arc_GetArcSlopes(arc,beginslope,endslope)
arc_frSegment	*arc;
pair_frXY		*beginslope;
pair_frXY		*endslope;
{
	switch(arc->type) {
		case arc_LINETYPE:
					pair_frSubtract(beginslope,&arc->data.line.to,
												&arc->data.line.from);
					*endslope = *beginslope;
					break;
		case arc_CONICTYPE:    	    	
					pair_frSubtract(beginslope,&arc->data.conic.b,
												&arc->data.conic.a);
					pair_frSubtract(endslope,&arc->data.conic.b,
												&arc->data.conic.c);
					break;
		case arc_BEZIERTYPE:
					pair_frSubtract(beginslope,&arc->data.bezier.b,
												&arc->data.bezier.a);
					pair_frSubtract(endslope,&arc->data.bezier.c,
												&arc->data.bezier.d);
					break;
	}
}


