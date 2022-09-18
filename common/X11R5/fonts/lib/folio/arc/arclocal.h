/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:arc/arclocal.h	1.1"
/*
 * @(#)arclocal.h 1.7 89/06/09
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
	that are used in the archandler module, but need not be made 
	public. The file "arc.h" needs to be included before this one.
*/


/* DEFINITIONS */

#define	MAXTARRAY			((int32)6)
#define	BEZIERPARALLELTOL	0.005	/* tolerance for parallelism */
#define	BEZIERLINETOL		0.005	/* for bezier approx by line */
#define	MINFRACT			((FRONE)-(FRALMOST1))	/* min fract # */
#define	MINT				0.00001		/* lower bound for param t */
#define	MAXT				(1 - MINT) 		/* upper bound for t */
#define	TINIT				FRHUGENEG		  /* to init the t's */
#define	ZEROBOUND			(1.0e-8)
#define	SQRTBOUND			(1.0e-8)
#define	DEGCONICBOUND		((fract)0x00001000)	/* 1/16 in fract */
#define	ScratchSIZE			((int32)5000)
#define	MAXBITS				13		/* max # bits of int in fract */

#define	arc_CONIC3DTYPE		((int32)3)

/* some useful numbers in fract */
#define	FR2			((fract)0x00020000)
#define	FR4			((fract)0x00040000)
#define	FR8			((fract)0x00080000)
#define	FR16		((fract)0x00100000)
#define	FR32		((fract)0x00200000)
#define	FR64		((fract)0x00400000)
#define	FR128		((fract)0x00800000)
#define	FR256		((fract)0x01000000)
#define	FR512		((fract)0x02000000)
#define	FR1024		((fract)0x04000000)
#define	FR2048		((fract)0x08000000)
#define	FR4096		((fract)0x10000000)
#define	FR8192		((fract)0x20000000)


/* STRUCTURES */

/* lines in local fract representation */

typedef struct _arc_fxLine{
	pair_frXY    from;                     /* starting point of line */
	pair_frXY    to;                             /* endpoint of line */
	pair_frXY	disp;				  /* displacement for this line */
	int8		shift;		  		  /* shift applied to this line */
} arc_fxLine;

/* conics in local fract representation */

typedef struct _arc_fxConic{
	pair_frXY        a,b,c;    /* three points that define the conic */
	fract          sh2;                        /* sharpness squared */
	pair_frXY	disp;			  /* displacement for this 2D conic */
	int8		shift;		  	  /* shift applied to this 2D conic */
} arc_fxConic;


typedef struct _arc_fxpoint3D{
	fract	x,y,z;			/* point in 3D space */
} arc_fxpoint3D;

/* Arcs of parabolas in 3D space from which all 2D conics are derived */
 
typedef struct _arc_conic3D{
	arc_fxpoint3D	a,b,c;
	pair_frXY	disp;			/* displacement for this 3D conic */
	int8		shift;		  	/* shift applied to this 3D conic */
} arc_fxConic3D;


/* beziers in local fract representation */

typedef struct _arcfxBezier{
	pair_frXY        a,b,c,d;    /* four points that define a bezier */
	pair_frXY	disp;				/* displacement for this bezier */
	int8		shift;		  		/* shift applied to this bezier */
} arc_fxBezier;


/*  Can hold a line, 2Dconic, 3Dconic or bezier in local fract representation */

typedef union _arc_fxData{
	arc_fxLine      line; 
	arc_fxConic   	conic;
	arc_fxConic3D   conic3D;
	arc_fxBezier    bezier;
} arc_fxData;


/* The actual arc structure */

typedef struct _arc_fxSegment{
	int8        type;					/* line, conic or bezier */
	bbox_iBBox  bbox;					 /* integer bbox for arc */
	arc_fxData  data;					/* holds one type of arc */
} arc_fxSegment;


/* beziers in double floating point representation */

typedef struct _arcflBezier{
	pair_dXY    a,b,c,d;    /* four points that define a bezier */
	pair_dXY	disp;		/* displacement for this bezier */
	int8		shift;		/* shift applied to this bezier */
} arc_flBezier;


/* the info needed to split curves at a point; parameter and slope */

typedef	struct _arc_pointinfo{
	fract	t;				/* the parameter t value */
	fract	sn;				/* the slope numerator at that point */
	fract	sd;				/* the slope denominator at that point */
} arc_pointinfo;


/* GLOBALS */

arc_pointinfo	arc_tarray[MAXTARRAY];	/* to store the values of t */

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

extern	fract	arc_MaxOf4(/*a,b,c,d*/);
/* fract   a,b,c,d;	*/
/*
	Finds the absolute max of 4 fract numbers.
*/


extern	fract	arc_MaxOf6(/*a,b,c,d,e,f*/);
/* fract   a,b,c,d,e,f;	*/
/*
	Finds the absolute max of 6 fract numbers.
*/


extern	fract	arc_MaxOf8(/*a,b,c,d,e,f,g,h*/);
/* fract   a,b,c,d,e,f,g,h;	*/
/*
	Finds the absolute max of 8 fract numbers.
*/


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
/*pair_frXY	*p1,*p2,*p;*/
/*fract	s,t;*/
/*
	Given p1 and p2 calculates p, a point between p1 and p2, using the 
	parameters s and t.
*/


extern	void	arc_BezierMidPoint(/*p1,p2,p*/);
/*pair_frXY	*p1,*p2,*p;*/
/* 
	Given p1 and p2, calculates their midpoint p 
*/


extern	void	arc_BezierSplitAtPoint(/*b0,t,b1,b2*/);
/*arc_fxbezier	*b0,*b1,*b2;*/
/*fract	t; */
/* 
	Divides the bezier curve b into two beziers b1 and b2 at the point
	t, where t=0 gives the beginning of b and t=1 the end.
*/


extern	int32	arc_BezierGetTangents(/*A,B,C,D,tempt*/);
/*double	A,B,C,D; */
/*fract	*tempt; */
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
/*arc_fxbezier	*bez;*/
/*fract	*tempt;*/
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


extern	fract	arc_BezierFindDeriv(/*A,B,C,D,t*/);
/*fract	A,B,C,D,t;*/
/*
	Given A,B,C,D and parameter t, it returns the value for f'(t).
	The equation used for the derivative is:

	f'(t) = 3*t*t(-A + 3B -3C + D) + 6*t(A - 2B + C) + 3(B - A)
*/


extern	void	arc_BezierGetSlope(/*t,b,dydt,dxdt*/);
/*fract	t;*/
/*arc_fxbezier	*b;*/
/*fract	*dydt;*/
/*fract	*dxdt;*/
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
/*arc_fxbezier	*b;*/
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
/*arc_fxbezier	*bezier;*/
/*	
	Pushes one bezier segment onto the stack. 
*/


extern	void	arc_BezierTFromSlope(/*sn,sd,bez,t1,t2*/);
/*fract	sn,sd;*/
/*fract	*t1,*t2;*/
/*arc_fxbezier	*bez;*/
/*
	Given the slope, sn and sd, it will find the two parameters t1 and
	t2 that correspond to points with that slope. It will only return 
	parameters that satisfy 0 <= t <= 1.
*/


extern	bool	arc_BezierVerifyInflection(/*b,t*/);
/*arc_fxbezier	*b;*/
/*fract	t;*/
/* 
	Checks that the given parameter t is a real inflection point for the
	bezier, by verifying that the curvature is zero at this point.
	f(t) = t*t*t(-A + 3B -3C + D) + 3*t*t(A -2B + C) + 3t(B - A) + A
	f'(t) = 3*t*t(-A + 3B - 3C + D) + 6t(A -2B + C) + 3(B - A)
	f''(t) = 6t(-A + 3B -3C + D) + 6(A -2B + C)

		Curvature = x'y'' - y'x'' = 0
*/

	
extern	void	arc_BezierUpdatet(/*b*/);
/*arc_fxbezier	*b;*/
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
/*arc_fxbezier	*bezier;*/
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
/*arc_fxline	*l;*/
/*fract		t;*/
/*arc_fxline	*l1,*l2;*/
/* 
	Splits a line at the parameter t. 
*/


extern	void	arc_LinePushStack(/*line*/);
/*arc_fxline	*line;*/
/* 
	Pushes one line onto the stack. 
*/


extern	void	arc_LineFillStack(/*line*/);
/*arc_fxline	*line;*/
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
/*pair_frXY		*p2;*/
/*arc_fxpoint3D	*p3;*/
/* 
	converts a point in 2D to a point in 3D 
*/


extern	void	arc_ConicScale3DPoint(/*p,f*/);
/*arc_fxpoint3D	*p;*/
/*fract		f;*/
/*	
	Scales a 3D point by the given scalefactor. 
*/


extern	void    arc_ConicTranslate3DConic(/*c3*/);
/*arc_fxConic3D   *c3;*/
/*
	Translates the 3D conic so that the point A is at (0,0,1), and sets
	the conic's displacement accordingly.
*/


extern	int32   arc_GetShiftForConic(/*sh,maxnum*/);
/*fract   sh;*/
/*fract   maxnum;*/
/*
	Returns the minimum shift needed, leaving room to multiply by the
	sharpness + extra 2 bits for additional arithmetic if the conic has
	big numbers.
*/


extern	void	arc_Conic2DTo3D(/*c2,c3*/);
/*arc_fxconic	*c2;*/
/*arc_fxconic3D	*c3;*/
/*	
	Converts a 2D conic into a 3D arc of parabola 
*/


extern	void    arc_Point3DTo2D(/*pt3D,pt2D*/);
/*arc_fxpoint3D   *pt3D;*/
/*pair_frXY       *pt2D;*/
/* 
	project a 3D point into 2D
*/


extern	void	arc_Conic3DTo2D(/*c3,c2*/);
/*arc_fxconic3D	*c3;*/
/*arc_fxconic	*c2*/
/* 
	projects a 3D arc of parabola onto a 2D conic 
*/


extern	void	arc_Conic3DNormalize(/*c*/);
/*arc_fxconic3D		*c;*/
/*
	Transforms a 3D arc of parabola to another arc of parabola having the
	same line of sight ( from the origin ) 2D projection to the plane 
	z = 1.0 as the original arc and so that the endpoints of the resulting
	arc are in the z = 1.0 plane.
*/


extern	void	arc_Conic3DPointInterpolation(/*p1,p2,s,t,p*/);
/*arc_fxpoint3D	*p1,*p2,*p;*/
/*fract	s,t;*/
/* 
	It interpolates between p1 and p2, using s and t, to find p 
*/


extern	void	arc_Conic3DMidPoint(/*p1,p2,p*/);
/*arc_fxpoint3D	*p1,*p2,*p;*/
/* 
	From p1 and p2 it finds p, the midpoint of the line from p1 to p2 
*/


extern	void	arc_ConicSplitAtPoint(/*c0,t,c1,c2*/);
/*arc_fxconic3D	*c0,*c1,*c2;*/
/*fract	t;*/
/*
	Splits a 3D parabolic arc into two 3D parabolic arcs, c1 and c2, at
	a specified point. The point of division is specified by a number t
	between 0 and 1 which yields the desired point when plugged into the
	parametric equation of the parabola given below.
			A(1-t)(1-t) + B(1-t)t + Ctt
*/
		

extern	int32	arc_ConicGetTangents(/*A,B,C,S,t*/);
/*double	A,B,C,S;*/
/*fract	*t;*/
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


extern	int32	arc_ConicComputeQuadrants(/*A,B,C,S2*/);
/*pair_dXY    *A,*B,*C;*/
/*fract       S2;*/
/*
	Given a conic, it returns, in the global array arc_tarray, the values 
	of the parameter t for which the curve has a horizontal tangent or a
	vertical tangent. These values must be 0 and 1; there can be at most 
	2 values for any conic. This is all done so that a curve will lie in 
    	one quadrant, and go in one direction.
*/


extern	void	arc_Conic2DPushStack(/*conic*/);
/*arc_fxconic	*conic;*/
/* 
	Pushes one conic segment onto the stack. 
*/


extern	void    arc_Conic3DPushStack(/*conic*/);
/*arc_fxConic3D       *conic;*/
/* 
	Pushes one 3D conic segment onto the stack.
*/


extern	fract   arc_EvalFunction(/*A,B,C,t*/);
/*fract   A,B,C,t;*/
/*
	Evaluate function at the given parameter.
	Function:
			A(1-t)(1-t) + Bt(1-t) +Ctt
*/


extern	void    arc_GetPointOnConic(/*conic,t,pt*/);
/*arc_fxConic3D   *conic;*/
/*arc_fxpoint3D   *pt;*/
/*
	Evaluate a point on the conic at the given parameter t.
	Equation for the conic is:
			A(1-t)(1-t) + Bt(1-t) +Ctt
*/


extern	void    arc_GetConicSharpness(/*conic,pt*/);
/*arc_fxConic *conic;*/
/*pair_frXY   *pt;*/
/*
	Calculates the sharpness for the given conic from the endpoints and
	the control point and an extra point on the curve.
*/


extern	void	arc_ConicSplitAndFillStack(/*conic*/);
/*arc_fxconic	*conic;*/
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

extern	void    arc_FrArcToFxArc(/*frarc,fxarc,LShift*/);
/*arc_frSegment   *frarc; */
/*arc_fxSegment   *fxarc; */
/*int8            LShift; */

/* 
        Converts an arc represented in 16/16 fixed point and a Left Shift
        into an arc represented in double precision floating point.
*/



extern	void    arc_FxArcToFrArc(/*fxarc,frarc,disp*/);
/*arc_fxSegment   *fxarc;*/
/*arc_frSegment   *frarc;*/
/*pair_iXY        *disp;*/
/* 
    Converts an arc represented in double precision floating point into an arc 
    represented in 16/16 fixed point and an integer displacement from the origin
    for the arc.
    NOTE: The arc will never be a bezier, since beziers will always be 
    approximated by a conic.
*/

 
extern	bool    arc_SimpleArc(/*fxarc*/);
/*arc_fxSegment   *fxarc; */
/*
    Returns true if the arc is small enough not to overflow the integer
    arithmetic used in tracing arcs and false otherwise. 
*/


extern	bool    arc_CheckFlarcStack(/*arc,pop*/);
/*arc_fxSegment     *arc;*/
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


extern	void    arc_GetFxArcBBox(/*fxarc,bbox*/);
/*arc_fxSegment             *fxarc;*/
/*bbox_iBBox              *bbox;*/
/*
    Given an arc in local fract representation, this function returns 
	its bounding box rounded to integers. The bounding box could be 
	part of the arc's structure or a different bbox.
*/
        

extern	void    arc_SplitStackTop();
/*
    This function will divide the arc on the top of the stack into two
    arcs of equal size, and pushes them back onto the stack. If the stack
    is about to be filled, that is this split will fill the stack, the
    original arc is saved in 'lastarc' so that if the top of the stack is
    still not small enough, 'lastarc' can be split up into 1/4, 3/4 etc.
*/

