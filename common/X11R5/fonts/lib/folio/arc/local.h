/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:arc/local.h	1.1"

/*
 * @(#)local.h 1.3 89/03/28
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
	This file contains structures, globals, definitions and functions 
	that are used in the archandler module, but need not be made public.
	The file "arc.h" needs to be included before this one.
*/


/* DEFINITIONS */

#define	MAXTARRAY			((int32)6)
#define	BEZIERPARALLELTOL	0.005		/* tolerance for parallelism */
#define	BEZIERLINETOL		0.005	/* for bezier approximated by line */
#define	MINT				0.00001		/* lower bound for parameter t */
#define	MAXT				(1 - MINT)	/* upper bound for parameter t */
#define	ZEROBOUND			(1.0e-8)
#define	SQRTBOUND			(1.0e-8)
#define	DEGCONICBOUND		0.0001
#define	ScratchSIZE			((int32)5000)
#define arc_MINSQRT			0.0
#define arc_MAXSQRT			1.0e+6
#define arc_ROOTACC			1.0e-6
#define arc_MINTROOT		0.0
#define arc_MAXTROOT		2.0

/* STRUCTURES */

/* lines in double precision floating point */

typedef struct _arc_flLine{
	pair_dXY    from;                     /* starting point of line */
	pair_dXY    to;                             /* endpoint of line */
} arc_flLine;

/* conics in double precision floating point */

typedef struct _arc_flConic{
	pair_dXY        a,b,c;    /* three points that define the conic */
	double          sh2;                       /* sharpness squared */
} arc_flConic;

/* beziers in double precision floating point */

typedef struct _arcflBezier{
	pair_dXY        a,b,c,d;        /* four points that define a bezier */
} arc_flBezier;


/*  Can hold a line, conic or bezier in double precision floating point */

typedef union _arc_flData{
	arc_flLine      line; 
	arc_flConic     conic;
	arc_flBezier    bezier;
} arc_flData;

/* The actual arc structure */


typedef struct _arc_flSegment{
	int8        type;                            /* line, conic or bezier */
	bbox_iBBox  bbox;                             /* integer bbox for arc */
	arc_flData  data;                            /* holds one type of arc */
} arc_flSegment;



typedef struct _arc_dpoint3D{
	double	x,y,z;			/* point in 3D space */
} arc_dpoint3D;

/* Arcs of parabolas in 3D space from which all 2D conics are derived */
 
typedef struct _arc_conic3D{
	arc_dpoint3D	a,b,c;
} arc_conic3D;


/* the info needed to split curves at a point; parameter and slope */

typedef	struct _arc_pointinfo{
	double	t;				/* the parameter t value */
	double	sn;				/* the slope numerator at that point */
	double	sd;				/* the slope denominator at that point */
} arc_pointinfo;


/* GLOBALS */

/*
	This variable is used to store an arc which, when split up, is going
	to fill the ArcStack. It can be retrieved and split up a different 
	way to avoid filling up the stack.
*/ 
	
arc_flSegment		arc_lastarc;				/* look above  */
int32			arc_savedarc;				/* did we save an arc in arc_lastarc */
arc_pointinfo	arc_tarray[MAXTARRAY];			/* to store the values of t */

/* FUNCTIONS */

extern	void	RigPushTransContext();
extern	void	RigPopTransContext();
extern	void	RigSetCurrentTrans();
extern	void	RigSetTranslation();
extern	void	RigSetScale();
extern	void	RigSetRotation();
extern	void	RigSetFont();
extern	void	RigSetSize();
extern	void	RigBeginPath();
extern	void	RigSetPathAutoRel();
extern	void	RigBeginContour();
extern	void	RigAppendLine();
extern	void	RigAppendConic();
extern	void	RigAppendBezier();
extern	void	RigEndPath();
extern	void	RigSetStrokeMode();
extern	void	RigSetStencilTransformation();
extern	void	RigGetStencilTransformation();
extern	void	RigApply();


/*---------------------------------------------------------------------*/
/*
	Miscellaneous routines that lines, conics and beziers use.
*/
/*---------------------------------------------------------------------*/

extern	void	Punt(/*err,s*/);
/* char	*err,*s; */
/*
	Error Function 
*/


extern	void	arc_Inittarray();
/*	
	Fills the arc_tarray with NIL, -1. 
*/


extern	void	arc_Sorttarray();
/*
	Puts arc_tarray elements in descending order. A more efficient sort could
	be used, but since the maximum number of elements is 6, this is not
	really necessary.
*/ 


/*---------------------------------------------------------------------*/
/*
	The following are all the routines that manipulate bezier curves.
*/

/*---------------------------------------------------------------------*/


extern	void	arc_BezierPointInterpolation(/*p1,p2,s,t,p*/);
/*pair_dXY	*p1,*p2,*p;*/
/*double	s,t;*/
/*
	Given p1 and p2 calculates p, a point between p1 and p2, using the 
	parameters s and t.
*/


extern	void	arc_BezierMidPoint(/*p1,p2,p*/);
/*pair_dXY	*p1,*p2,*p;*/
/* 
	Given p1 and p2, calculates their midpoint p 
*/


extern	void	arc_BezierSplitAtPoint(/*b0,t,b1,b2*/);
/*arc_bezier	*b0,*b1,*b2;*/
/*double	t; */
/* 
	Divides the bezier curve b into two beziers b1 and b2 at the point
	t, where t=0 gives the beginning of b and t=1 the end.
*/


extern	int32	arc_BezierGetTangents(/*A,B,C,D,tempt*/);
/*double	A,B,C,D; */
/*double	*tempt; */
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


extern	int32	arc_BezierGetInflectionPt(/*bez,tempt*/);
/*arc_bezier	*bez;*/
/*double	*tempt;*/
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


extern	double	arc_BezierFindDeriv(/*A,B,C,D,t*/);
/*double	A,B,C,D,t;*/
/*
	Given A,B,C,D and parameter t, it returns the value for f'(t).
	The equation used for the derivative is:

	f'(t) = 3*t*t(-A + 3B -3C + D) + 6*t(A - 2B + C) + 3(B - A)
*/


extern	void	arc_BezierGetSlope(/*t,b,dydt,dxdt*/);
/*double	t;*/
/*arc_bezier	*b;*/
/*double	*dydt;*/
/*double	*dxdt;*/
/*
	Given the values for A,B,C,D in a bezier curve and the parameter t,
	this function will calculate the slope at that point by finding
	dy/dt and dy/dx and dividing the two to find dy/dx. It returns the
	slope.
 
	The function used is:

	f(t) = t*t*t(-A + 3B -3C + D) + t*t(3A - 6B + 3C) + t(-3A + 3B) + A
	f'(t) = 3*t*t(-A + 3B -3C + D) + 6*t(A - 2B + C) + 3(B - A)
*/


extern	int32	arc_BezierComputeQuadrants(/*b*/);
/*arc_bezier	*b;*/
/*
	Given a bezier curve, it returns, in the global array arc_tarray, the 
	values of the parameter t for which the curve has a horizontal tangent,
	a vertical tangent or a point of inflection. These values must be 
	between 0 and 1; there can be at most 6 values for any bezier curve,
	2 horizontal tangents, 2 vertical tangents and 2 inflection points. 
	It returns a count of how many t values it found. This is all done 
	so that a curve will lie in one quadrant, and go in one direction.
*/


extern	void	arc_BezierPushStack(/*bezier*/);
/*arc_bezier	*bezier;*/
/*	
	Pushes one bezier segment onto the stack. 
*/


extern	void	arc_BezierTFromSlope(/*sn,sd,bez,t1,t2*/);
/*double	sn,sd;*/
/*double	*t1,*t2;*/
/*arc_bezier	*bez;*/
/*
	Given the slope, sn and sd, it will find the two parameters t1 and
	t2 that correspond to points with that slope. It will only return 
	parameters that satisfy 0 <= t <= 1.
*/


extern	bool	arc_BezierVerifyInflection(/*b,t*/);
/*arc_bezier	*b;*/
/*double	t;*/
/* 
	Checks that the given parameter t is a real inflection point for the
	bezier, by verifying that the curvature is zero at this point.
	f(t) = t*t*t(-A + 3B -3C + D) + 3*t*t(A -2B + C) + 3t(B - A) + A
	f'(t) = 3*t*t(-A + 3B - 3C + D) + 6t(A -2B + C) + 3(B - A)
	f''(t) = 6t(-A + 3B -3C + D) + 6(A -2B + C)

		Curvature = x'y'' - y'x'' = 0
*/

	
extern	void	arc_BezierUpdatet(/*b*/);
/*arc_bezier	*b;*/
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


extern	void	arc_BezierSplitAndFillStack(/*bezier*/);
/*arc_bezier	*bezier;*/
/*
	Takes a bezier curve, splits it up into segments and puts them into
	the arc_stack. The curve is traversed correctly if the segments are
	taken from the top of the stack.
*/


/*---------------------------------------------------------------------*/
/*
	The following are all the routines that manipulate lines.
*/

/*---------------------------------------------------------------------*/


extern	void	arc_LineSplitAtPoint(/*l,t,l1,l2*/);
/*arc_line	*l;*/
/*double		t;*/
/*arc_line	*l1,*l2;*/
/* 
	Splits a line at the parameter t. 
*/


extern	void	arc_LinePushStack(/*line*/);
/*arc_line	*line;*/
/* 
	Pushes one line onto the stack. 
*/


extern	void	arc_LineFillStack(/*line*/);
/*arc_line	*line;*/
/*
	Takes a line and, since it does not need to split it, pushes it onto
	the arc_stack.
*/


/*---------------------------------------------------------------------*/
/*
	The following are all the routines that manipulate conics.
*/

/*---------------------------------------------------------------------*/

extern	void	arc_ConicPoint2DTo3D(/*p2,p3*/);
/*pair_dXY		*p2;*/
/*arc_dpoint3D	*p3;*/
/* 
	converts a point in 2D to a point in 3D 
*/


extern	void	arc_ConicScale3DPoint(/*p,f*/);
/*arc_dpoint3D	*p;*/
/*double		f;*/
/*	
	Scales a 3D point by the given scalefactor. 
*/


extern	void	arc_Conic2DTo3D(/*c2,c3*/);
/*arc_conic	*c2;*/
/*arc_conic3D	*c3;*/
/*	
	Converts a 2D conic into a 3D arc of parabola 
*/


extern	void	arc_Conic3DTo2D(/*c3,c2*/);
/*arc_conic3D	*c3;*/
/*arc_conic	*c2*/
/* 
	projects a 3D arc of parabola onto a 2D conic 
*/


extern	void	arc_Conic3DNormalize(/*c*/);
/*arc_conic3D		*c;*/
/*
	Transforms a 3D arc of parabola to another arc of parabola having the
	same line of sight ( from the origin ) 2D projection to the plane 
	z = 1.0 as the original arc and so that the endpoints of the resulting
	arc are in the z = 1.0 plane.
*/


extern	void	arc_Conic3DPointInterpolation(/*p1,p2,s,t,p*/);
/*arc_dpoint3D	*p1,*p2,*p;*/
/*double	s,t;*/
/* 
	It interpolates between p1 and p2, using s and t, to find p 
*/


extern	void	arc_Conic3DMidPoint(/*p1,p2,p*/);
/*arc_dpoint3D	*p1,*p2,*p;*/
/* 
	From p1 and p2 it finds p, the midpoint of the line from p1 to p2 
*/


extern	void	arc_ConicSplitAtPoint(/*c0,t,c1,c2*/);
/*arc_conic3D	*c0,*c1,*c2;*/
/*double	t;*/
/*
	Splits a 3D parabolic arc into two 3D parabolic arcs, c1 and c2, at
	a specified point. The point of division is specified by a number t
	between 0 and 1 which yields the desired point when plugged into the
	parametric equation of the parabola given below.
			A(1-t)(1-t) + B(1-t)t + Ctt
*/
		

extern	int32	arc_ConicGetTangents(/*A,B,C,S,t*/);
/*double	A,B,C,S;*/
/*double	*t;*/
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


extern	int32	arc_ConicComputeQuadrants(/*c*/);
/*arc_conic	*c;*/
/*
	Given a conic, it returns, in the global array arc_tarray, the values 
	of the parameter t for which the curve has a horizontal tangent or a
	vertical tangent. These values must be 0 and 1; there can be at most 
	2 values for any conic. This is all done so that a curve will lie in 
    	one quadrant, and go in one direction.
*/


extern	void	arc_ConicPushStack(/*conic*/);
/*arc_conic	*conic;*/
/* 
	Pushes one conic segment onto the stack. 
*/


extern	void	arc_ConicUpdatet(/*c*/);
/*arc_conic	*c;*/
/*
	Given a conic, it updates the t parameters in the global arc_tarray.
	The old t values referred to another conic and are not correct for
	this one, but the slopes remain correct. The first t value is not 
	used, since the curve has already been split at that point. It calls
	ArcConicGetTangents to get the new t values. The important thing is
	that there is one less value of t when the function returns, since 
	the curve has already been split at one place.
*/


extern	void	arc_ConicSplitAndFillStack(/*conic*/);
/*arc_conic	*conic;*/
/*
	Takes a conic, splits it up into segments and puts them onto the 
	arc_stack. The curve is traversed correctly if the segments are taken
	from the top of the stack.
	NOTE :
		In order to split up the conic; it has to be converted into a 3D 
		arc of parabola, split up, converted back to 2D and pushed onto 
		the arc_stack. The use of the 3D arc of parabola need not be 
		known or understood by the user.
*/ 

/*-------------------------------------------------------------------------*/

extern	void    arc_FrArcToFlArc(/*frarc,flarc,LShift*/);
/*arc_frSegment   *frarc; */
/*arc_flSegment   *flarc; */
/*int8                    LShift; */

/* 
        Converts an arc represented in 16/16 fixed point and a Left Shift
        into an arc represented in double precision floating point.
*/



extern	void    arc_FlArcToFrArc(/*flarc,frarc,disp*/);
/*arc_flSegment   *flarc;*/
/*arc_frSegment   *frarc;*/
/*pair_iXY                *disp;*/
/* 
    Converts an arc represented in double precision floating point into an arc 
    represented in 16/16 fixed point and an integer displacement from the origin
    for the arc.
    NOTE: The arc will never be a bezier, since beziers will always be 
    approximated by a conic.
*/

 
extern	bool    arc_SimpleArc(/*flarc*/);
/*arc_flSegment   *flarc; */
/*
    Returns true if the arc is small enough not to overflow the integer
    arithmetic used in tracing arcs and false otherwise. 
*/


extern	bool    arc_CheckFlarcStack(/*arc,pop*/);
/*arc_flSegment     *arc;*/
/*bool            pop;*/
/* 
    Checks the arc on the top of the stack. If pop is true, it will 
    actually pop the top element, otherwise, it will just return a 
    copy of it to the caller. A pop also returns the top element to
    the caller.
*/


extern	bool    arc_PopStack(/*frarc,disp*/);
/*arc_frSegment   *frarc;*/
/*pair_iXY                *disp;*/
/* 
    Will return the arc on the top of the stack to the caller. This arc
    will be in 16/16 fixed point, contained in a quadrant and small 
    enough not to overflow the integer arithmetic used in tracing arcs.
    It will return false when there are no more arcs on the stack, and
    true otherwise.
*/


extern	void    arc_GetFlArcBBox(/*flarc,bbox*/);
/*arc_flSegment             *flarc;*/
/*bbox_iBBox              *bbox;*/
/*
    Given an arc, this function returns its bounding box rounded to 
    integers. The bounding box could be part of the arc's structure
    or a different bbox.
*/
        

extern	void    arc_SplitStackTop();
/*
    This function will divide the arc on the top of the stack into two
    arcs of equal size, and pushes them back onto the stack. If the stack
    is about to be filled, that is this split will fill the stack, the
    original arc is saved in 'lastarc' so that if the top of the stack is
    still not small enough, 'lastarc' can be split up into 1/4, 3/4 etc.
*/


extern	void    arc_GetFlarcBBox(/*flarc,bbox*/);
/*arc_flSegment   *flarc;*/
/*bbox_iBBox      *bbox;*/

/*
Given an arc represented in double precision floating point, this
function returns its bounding box rounded to integers.
*/

