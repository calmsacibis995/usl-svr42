/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:segment/conics.c	1.1"
#ifndef lint
static char sccsid[] = "@(#)conics.c 1.2 89/03/10";
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

/* conic - FIXEDPOINT integer representation */
typedef struct _seg_iConic{
    pair_iXY	a,b,c;	    
    int32   	sh2num,sh2den;  	/* n/d rational representation of sharpnes squared */
}   seg_iConic;

/* coefficients and differences for the conic equation */
typedef struct _seg_conicEq{
    int32   A,B,C,D,E,F,    /* fn = Ax^2 + 2Bxy + Cy^2 + Dx + Ey + F */
    	    fn,fx,fy,
    	    fxx,fxy,fyy,hfxx;
} seg_conicEq;



/* ----------------------------------------------------------------- */

/*
    Transformation of 2D conics prior to tracing. The idea is to apply
    some simple transformation (rotations by multiples of 90 degrees )
    so that the control points of the arc are in the first quadrant.
    In addition, we make sure that AC is in the first octant so
    that if the arc may be confined to an octant, it will be all
    in the first octant.
*/

int32	seg_ConicFindOp(c)
seg_iConic    *c;
{
    pair_iXY ab;
    slope_SlopeFrom2Points(&ab,&c->a,&c->c);
    return(slope_FindOpToQuad1(&ab));
}


/* Applies the given operation to the points of the conic */

#define	seg_ConicApplyOp(cnc,op)	    	    	    	    \
    {   	    	    	    	    	    	    	    \
    	slope_ApplyOp(&(cnc)->a,op);	    	    	    	    \
    	slope_ApplyOp(&(cnc)->b,op);	    	    	    	    \
    	slope_ApplyOp(&(cnc)->c,op);	    	    	    	    \
    }

/* ----------------------------------------------------------------- */

/* 
    Approximation of sharpness squared by a rational
*/

/* used for calculating TOLERANCE */
#define	TOL	6

void	seg_ConicApproximateByRational(s,n,d,maxq)
int32	s;	/* the number to be approximated in 16/16 fixed point */
int32	*n,*d;	/* the numerator and denominator of the rational */
int32	maxq;	/* stopping condition for rational */
{
    /*
    	Approximates a real number by a rational with bounded numerator
    	and denominator using Farey's sequence. The maximum values for the
    	numerator and denominator of the Farey rational used to approximate
    	the real number are constants.
    */
    Rint32   hn, ln, mn, hd, ld, md;	 /* really int32's */
    Rint32   hsd, lsd, msd, sh, tmp;	/* in 24/8 fixed point */
    int32    TOLERANCE;	/* stopping condition, in 24/8 fixed point */

    /* convert sharpness to 24.8 fixed point from 16.16 fixed point*/
    sh = (s+(1<<7)>>8);
    /* calculate tolerance */
    TOLERANCE = (sh >> TOL);
	/* search in the Farey sequence */
		ln = hd = 0;
		hn = ld = 1;
		hsd = 0;
		lsd = sh;
		mn = hn + ln;
		md = hd + ld;
		do {
	    	msd = hsd + lsd;
	    	tmp = (msd - (mn << 8));
	    	if ( abs(tmp) < TOLERANCE ) break;
	    	if ( tmp <= 0 ) 						 /* middle to low */
				{ hn = mn; hd = md; hsd = msd;} 
	    	else 									/* middle to high */
				{ln = mn; ld = md; lsd = msd;}
			mn = hn + ln;
			md = hd + ld;
		} while ( (md + (mn<<2)) <= maxq );
		*n = mn;
		*d = md;
}

/* ----------------------------------------------------------------- */

void	seg_SetConicCoefficients(c,ceq,df2dx2)
    seg_iConic	*c;
    seg_conicEq	*ceq;
    int32    	*df2dx2;

{
    /* compute the implicit equation */
    int32	n,d,n4; 
    line_iLine	ab,bc,ac,abxn4,acxd,dfdxeq0;
    int32   	dfdxa,dfdxc;

    
    /*
        The implicit equation is computed as
    	   	-4*n*ab*bc+d*ac*ac
    	where n/d is the sharpness squared and ab,ac and bc are the
    	equations of the obvious lines.
    	CAREFUL with precision for each coefficient ...
    */
    n = c->sh2num;
    d = c->sh2den;
    n4 = 4*n;
    line_iLineFrom2Points(&c->a,&c->b,&ab);
    line_iLineFrom2Points(&c->a,&c->c,&ac);
	if (!line_iLineEvaluation(&ac,&c->b)) {		/* conic is a straight line */
		ceq->A = 0; ceq->B = 0; ceq->C = 0;
		ceq->D = ac.m; ceq->E = ac.n; ceq->F = ac.p;
	} else {
    	line_iLineFrom2Points(&c->b,&c->c,&bc);
    	line_iLineScaling(&abxn4,&ab,n4);
    	line_iLineScaling(&acxd,&ac,d);  	    	    	    /* --precision-- */
    	ceq->A = ac.m*acxd.m - abxn4.m*bc.m;     	    	    /* 2 * fractions */
    	ceq->C = ac.n*acxd.n - abxn4.n*bc.n;     	    	    /* 2 * fractions */
    	ceq->F = ac.p*acxd.p - abxn4.p*bc.p;    	    	    /* 4 * fractions */
    	ceq->B = ac.m*acxd.n - 2*n*(ab.m*bc.n + ab.n*bc.m);	    /* 2 * fractions */
    	ceq->D = 2*ac.m*acxd.p - abxn4.m*bc.p - abxn4.p*bc.m;   /* 3 * fractions */
    	ceq->E = 2*ac.n*acxd.p - abxn4.n*bc.p - abxn4.p*bc.n;   /* 3 * fractions */
	}
    /*
        Now compute the derivatives wrt x in a and c (at least one
        must be non-zero and if both are non zero they must have
        the same sign) and use them to normalize the signs of the 
        coefficients so that they produce non-negative derivatives
    */
    dfdxeq0.m = 2*ceq->A; dfdxeq0.n = 2*ceq->B; dfdxeq0.p = ceq->D; 
    dfdxa = line_iLineEvaluation(&dfdxeq0,&c->a);
    dfdxc = line_iLineEvaluation(&dfdxeq0,&c->c);
    if  ( (dfdxa < 0 ) || (dfdxc < 0 ) ) {
        ceq->A = -ceq->A; ceq->B = -ceq->B; 
    	ceq->C = -ceq->C; ceq->D = -ceq->D; 
    	ceq->E = -ceq->E; ceq->F = -ceq->F;
        dfdxeq0.m = -dfdxeq0.m; 
        /* dfdxeq0.n = -dfdxeq0.n; dfdxeq0.p = -dfdxeq0.p; */
    }

    *df2dx2 = dfdxeq0.m;
}

/* ----------------------------------------------------------------- */

void    seg_SetConicTracingDifferences(ceq,begpt,bitsoffract)
seg_conicEq	*ceq;
pair_iXY    	*begpt;
int32	    	bitsoffract;
{
    /*
    	Compute the tracing differences
    	The Pratt differences (obtained readily by algebraic manipulation
    	of the implicit equation of the conic) are :
		fn = ((A*x+2*B*y+D)*x)+((C*y+E)*y)+F
		fx = ((2*x+1)*A)+(2*B*y)+D
		fy = ((2*y+1)*C)+(2*B*x)+E
		fxx = 2*A
		fxy = 2*B
		fyy = 2*C
    	In addition, the increments to evaluate the function at the center
    	of the pixel above and to the right of the current position, (i.e., 
    	at (x+1/2,y+1/2) when deciding where to go from (x,y)) are the 
    	following:
    	   	fn += ((A+B)*x)+((B+C)*y)+((((A+C)/2)+B+D+E)/2)
    	    	fx += A+B
    	    	fy += B+C
    	***************** THIS IS INOPERATIVE **********************
    	********* IT DOES NOT WORK WHEN bitsoffract != 0 *************
    	ANTI-ALIASING. We use the method suggested by Vaughan Pratt to
    	deal with aliasing but with a small but important detail that
    	he does not describe. Because we are evaluating the function
    	at (x+0.5,y+0.5) when at (x,y), we can exceed the band in which
    	df/dx retains its sign (between endpoints). This forces us to
    	evaluate df/dx at (x,y) even though fx is evaluated at
    	(x+0.5,y+0.5). This we achieve by using the following 
    	relationships:
    	    df/df(x,y) = fx(x,y) - fxx/2 (fxx is constant)
    	    fx(x,y) = fx(x+0.5,y+0.5) - A - B
    	Therefore, we have
    	    df/dx(x,y) = fx(x+0.5,y+0.5) - A - B - fxx/2;
    	and the test df/dx(x,y)>0 translates into the following
    	    fx(x+0.5,y+0.5) > A + B + fxx/2
    	which can be evaluated just as efficiently as the test
    	suggested by Pratt because the right hand side is constant.
    */
    int32   A,B,C,D,E,F;
    A = ceq->A; B = ceq->B; C = ceq->C; 
    D = ceq->D; E = ceq->E; F = ceq->F;

    if ( bitsoffract == 0 ) {
        int32	x0,y0;
        x0 = begpt->x; y0 = begpt->y;
        ceq->fxx = 2*A;
        ceq->fxy = 2*B;
        ceq->fyy = 2*C;
        ceq->hfxx = A;
        ceq->fx = ((2*x0+1)*A)+(2*B*y0)+D+(A+B);
        ceq->fy = ((2*y0+1)*C)+(2*B*x0)+E+(B+C);
        ceq->fn = (((((A+B)*x0)+((B+C)*y0))<<2)+(A+C+((B+D+E)<<1))) >> 2;
    } else {
    	/* 
    	    Align all finite differences to have 3*bitsoffract bits
    	    of fraction. All differences are exact on that precision
    	    except fn which needs 4*bitsoffract bits. To compute
    	    a valid fn, we compute it exactly modulo 2^31 (ignoring the
    	    inevitable overflows) and then use the fact that fn is only 
    	    tested agains zero to discard bits so that (i) the result
    	    is aligned with all the other values and (ii) the results
    	    of tests with zero are unaffected.
    	*/
    	int32	one,x0,y0;
    	one = 1 << bitsoffract;
    	x0 = begpt->x << bitsoffract;
    	y0 = begpt->y << bitsoffract;
        ceq->fxx = (2*A) << bitsoffract;
        ceq->fxy = (2*B) << bitsoffract;
        ceq->fyy = (2*C) << bitsoffract;
        ceq->hfxx = A << bitsoffract;
        ceq->fx = ((2*x0+one)*A)+(2*B*y0)+D+((A+B)<<bitsoffract);
        ceq->fy = ((2*y0+one)*C)+(2*B*x0)+E+((B+C)<<bitsoffract);
    	x0 += one>>1; y0 += one>>1;
    	ceq->fn = ((((A*x0+2*B*y0+D)*x0)+((C*y0+E)*y0)+F)) >> bitsoffract;
    }
}

/* ----------------------------------------------------------------- */

    /*
    	Implement Bresenham/Pitteway/Pratt tracing in the first quadrant. 
    	It is given the initial value of the tracing function
    	and the first and second order increments for moves in the main 
    	(positive x) and secondary (positive y) directions with the 
    	assumption that a main move changes the function by a non-negative
    	value, that is that df/dx is non-negative on the curve. 
    	All the values used in the tracing are computed so that the
    	computation can be performed using 32-bit integer variables 
    	without overflow. 
    	    Ties (fn=0) are resolved the same way (Secondary Move)
    	by always using the same test (fn < 0). The 90 degree
    	rotations to transform the conic to the first octant does not
    	modify the effect on ties, no special adjustments to fn is
    	necessary before calling <GenerateMoves> . 
    */    	

#define seg_ConicGenerateMoves(MainMove,SecondaryMove,ceq,df2dx2,xcnt,ycny,movesp)  \
{	    	    	    	    	    	    	    	    	\
    Rint32  cnt;	    	    	    	    	    	    	\
    Rint32  fn,fx,fy;	    	    	    	    	    	    	\
    int32   fxx,fxy,fyy, hfxx;	    	    	    	    	    	\
    cnt = xcnt + ycnt;  	    	    	    	    	    	\
    /* restore differences for tracing */	    	    	    	\
    fn = ceq.fn; fx = ceq.fx; fy = ceq.fy;   	    	    	    	\
    fxx = ceq.fxx; fxy = ceq.fxy; fyy = ceq.fyy;	    	    	\
    hfxx = ceq.hfxx;   	    	    	    	    	    	    	\
    	    	    	    	    	    	    	    	    	\
    if ( xcnt && ycnt ) { /* if conic is not a line */	    	    	\
    	if (df2dx2 > 0) { /* if df/dx = 0 line is above conic segment */\
    	    while (cnt-- > 0) {	    	    	    	    	    	\
    	    	if (fn < 0) {   /* x move */	    	    	    	\
    	    	    fn += fx; fx += fxx; fy += fxy; 	    	    	\
    	    	    MainMove(movesp);	    	    	    	    	\
    	    	    if (! --xcnt ) break;    	    	    	    	\
    	    	} else {    /* y move -- anti-aliasing test needed */	\
    	    	    if (fx<hfxx) {/* crossed both sized of the curve */	\
    	    	    	fn += fx; fx += fxx; fy += fxy;	    	    	\
    	    	    	MainMove(movesp);   	    	    	    	\
    	    	    	if (! --xcnt ) break;    	    	    	\
    	    	    } else {	    	    	    	    	    	\
    	    	    	fn += fy; fx += fxy; fy += fyy;	    	    	\
    	    	    	SecondaryMove(movesp);	    	    	    	\
    	    	    	if (! --ycnt ) break;    	    	    	\
    	    	    }	    	    	    	    	    	    	\
    	    	}   	    	    	    	    	    	    	\
    	    }	    	    	    	    	    	    	    	\
    	} else {  /* df/dx = 0 line is below conic segment */	    	\
    	    while (cnt-- > 0) {	    	    	    	    	    	\
    	    	if (fn < 0) {   /* x move  anti-aliasing test needed */	\
    	    	    if (fx<hfxx) { /* crossed both sized of the curve */\
    	    	    	fn += fy; fx += fxy; fy += fyy;	    	    	\
    	    	    	SecondaryMove(movesp);	    	    	    	\
    	    	    	if (! --ycnt ) break;    	    	    	\
    	    	    } else {	    	    	    	    	    	\
    	    	    	fn += fx; fx += fxx; fy += fxy;	    	    	\
    	    	    	MainMove(movesp);   	    	    	    	\
    	    	    	if (! --xcnt ) break;    	    	    	\
    	    	    }	    	    	    	    	    	    	\
    	    	} else {	    /* y move */    	    	    	\
    	            fn += fy; fx += fxy; fy += fyy; 	    	    	\
    	            SecondaryMove(movesp);  	    	    	    	\
    	    	    if (! --ycnt ) break;    	    	    	    	\
    	    	}   	    	    	    	    	    	    	\
    	    }	    	    	    	    	    	    	    	\
    	}   	    	    	    	    	    	    	    	\
    }	    	    	    	    	    	    	    	    	\
    while ( xcnt-- > 0 ) {	    	    	    	    	    	\
        MainMove(movesp);	    	    	    	    	    	\
    }   	    	    	    	    	    	    	    	\
    while ( ycnt-- > 0 ) {	    	    	    	    	    	\
        SecondaryMove(movesp);	    	    	    	    	    	\
    }   	    	    	    	    	    	    	    	\
}  


/*
------------------------------------------------------------------------
        C O N I C    T R A C I N G 
------------------------------------------------------------------------
*/

/* 
   Values for the rhs of the inequality used to compute the precision with
   which the conic can be traced (see computation of <dim> and <bitsoffract> below)
*/
#define	K0	97
#define	K1	50
#define	K2	26
#define K3	13
#define	K4	6


/*
    Given a <conic> segment that is already in a quadrant and a handle
    <bhdl> to a large enough rook move buffer, traces the conic and
    fills the buffer accordingly.
    Assumes that the conic is traceable (see below).
*/
void	seg_TraceConic( conic, bhdl )
arc_frConic    	    *conic; 	/* conic data in FIXEDPOINT 16.16  */ 
rm_bufferHandle	    bhdl;
{
    /*
    	Computes the tracing differences needed for the Pittway-Pratt 
    	algorithm using the conic <c> and generates the rook move sequence
    	to represent it. The conic <c> is a sanitized version of <conic>
    	whose tangents lie all in the first quadrant and whose control
    	points have coordinates small enough in magnitude not to
    	overflow the integer arithmetic performed in this routine.
    	The coordinates of <c> may be not be grid points. If they aren't, they
    	are represented as fixed point integers with a uniform number of
    	bits of fract computed from the size of the conic. The starting
    	point for the trace (needed to compute the initial value of the
    	finite differences) is <conic->a> rounded to a GRID point.

    	In the first quadrant, P&P's algorithm uses the finite differences 
    	as follows:
    	    Main:   fn+=fx; fx+=fxx; fy+=fxy;
    	    Sec.:   fn+=fy; fx+=fxy; fy+=fyy;
    	In what follows we peform the following tasks:
    	(i) 	compute the EXACT implicit equation of the conic referred 
    	    	to the grid coordinates (careful with fracts!),
    	(ii) 	compute the initial values of the finite differences 
    	    	from the coefficients of the implicit equation of the conic
    	    	and the coordinates of the starting point (careful with 
    	    	fixed point alignement!),
    	(iii) 	adjust the initial values of the finite differences so 
    	    	that when the tracing is at (x,y) the finite differences 
    	    	have the values corresponding to (x+0.5,y+0.5);
    	    	this is done because of the convention we use to relate
    	    	grid points to pixels and how rook move sequences are 
    	    	defined, and so that the tie-breaking policy is
    	    	correctly implemented without duplicating the loop,
    	(iv) 	determine to which side of the arc (x direction or
    	    	y direction) lies the line df/dx = 0 so that we know which 
    	    	side of the tacing loop should have the anti-alias test, 
    	and (v) adjust the sign of the differences so that a move in the 
    	    	x direction increments the value of the function 
    	    	(an assumption built into the tracing code).
    */

    seg_iConic  c; 	    	/* conic in FIXEDPOINT */
    pair_iXY	begpt,endpt;	/* endpoints for tracing */
    pair_iXY 	disp;		/* local origin for tracing */
    pair_iXY	disp16;
    Rint32   	xcnt,ycnt;    	/* number of moves to trace */
    Rbyte    	*movesp;	/* pointer of sequence of rook moves */
    int32   	bitsoffract;   	/* bits of precision for FIXEDPOINT #s */
    int32   	dim;   		/* max dimension of conic->bbox in pixels */
    int32   	op; 	    	/* operation to map to 1st quadrant */
    seg_conicEq ceq;	    	/* coefficients & diffs for the conic equation */
    int32    	df2dx2;  	/* to distinguish the two tracing cases */
    
    c.a.x = conic->a.x; c.a.y = conic->a.y;
    c.b.x = conic->b.x; c.b.y = conic->b.y;
    c.c.x = conic->c.x; c.c.y = conic->c.y;
	
    /* initialize rook move endpoints before translating conic */
    pair_RoundFixedToInt(&begpt,&c.a);
    pair_RoundFixedToInt(&endpt,&c.c);
    xcnt = abs(endpt.x - begpt.x);
    ycnt = abs(endpt.y - begpt.y);


    /* translate conic around origin to minimize magnitude of control points */
    	/* set disp at (begpt+endpt)/2 */
    pair_XYAdd(&disp,&begpt,&endpt);
    pair_XYAddScalar(&disp,&disp,1);
    pair_XYShiftRight(&disp,&disp,1)
    	/* translate control points */
    disp16.x = disp.x<<FIXED16;
    disp16.y = disp.y<<FIXED16;
    pair_XYSubtract(&c.a,&c.a,&disp16);
    pair_XYSubtract(&c.b,&c.b,&disp16);
    pair_XYSubtract(&c.c,&c.c,&disp16);
    	/* translate trace endpoints */
    pair_XYSubtract(&begpt,&begpt,&disp);
    pair_XYSubtract(&endpt,&endpt,&disp);

    /*
    	Compute the precision for the FIXEDPOINT representation of
    	the conic AND the precision for representing the sharpness 
    	squared as rational number (n/d) based on the dimension of 
    	the conic. The values to be computed are
    	    <bitsoffract> , 
	    <maxnum> and <maxden>
	Let sigma = 4 * maxnum + maxden;
    	dim = largest dimension of conic;
   	    bitsoffract = bits of bitsoffraction to represent conic equation <= 4;
		Assume :
			sigma <= (2 * dim)
    	Then the inequality to satisfy is:
		(dim^4) <= ((2^31 - 1)/(8*2^(3*bitsoffract)*(2^(bitsoffract+1)+1)))
	Or,
		dim <= (((2^31 - 1)/(8*2^(3*bitsoffract)*(2^(bitsoffract+1)+1)))^(1/4))
	Let the right hand side be K(bitsoffract), then
			K(0) = 97.2589678
			K(1) = 50.8973266
			K(2) = 26.1278906
			K(3) = 13.2519641
			K(4) = 6.67561832
	
    */
    dim = max(xcnt, ycnt);
    if (dim <= K4)
	bitsoffract = 4;
    else if (dim <= K3)
	bitsoffract = 3;
    else if (dim <= K2)
	bitsoffract = 2;
    else if (dim <= K1)
	bitsoffract = 1;
    else /* if (dim <= K0) */
	bitsoffract = 0;

    /* approximate sh2 with a limit of 2*dim */
    seg_ConicApproximateByRational(conic->sh2,&c.sh2num,&c.sh2den,(2*dim));

    /* convert the conic into FIXEDPOINT (32-bitsoffract).bitsoffract */
    pair_Convert16FixedToVarFixed(&c.a,&c.a,bitsoffract);
    pair_Convert16FixedToVarFixed(&c.b,&c.b,bitsoffract);
    pair_Convert16FixedToVarFixed(&c.c,&c.c,bitsoffract);

    /* map conic to first quadrant */
    op = seg_ConicFindOp(&c);
    seg_ConicApplyOp(&c,op);
    slope_ApplyOp(&begpt,op);
    slope_ApplyOp(&endpt,op);

    /* compute the implicit equation */    
    seg_SetConicCoefficients(&c,&ceq,&df2dx2);

    /* set the tracing differences */
    seg_SetConicTracingDifferences(&ceq,&begpt,bitsoffract);

    /* generate moves for conic */
    rm_InitMovesp(bhdl,movesp);

    if (slope_OpSwapsXY(op)) {
    	int32 tmp;
    	tmp = xcnt; xcnt = ycnt; ycnt = tmp;
    	seg_ConicGenerateMoves(rm_PutY,rm_PutX,ceq,df2dx2,xcnt,ycnt,movesp);
    } else {
    	seg_ConicGenerateMoves(rm_PutX,rm_PutY,ceq,df2dx2,xcnt,ycnt,movesp);
    }
}
