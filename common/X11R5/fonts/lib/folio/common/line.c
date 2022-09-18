/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:common/line.c	1.1"
#ifndef lint
static char sccsid[] = "@(#)line.c 1.2 89/03/10";
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

/*-------------------------------------------------------------------
  	MANIPULATION OF STRAIGHT LINES
-------------------------------------------------------------------*/

/*
    Simple manipulations on lines of the form mx+ny+p
*/

/*-----------------------------------------------------------------*/

/* creating a vertical line x = v*/

void	line_dLineVertical(v,l)
double	v;
line_dLine	*l;
{
   	l->m = 1.0; l->n = 0.0; l->p = -v;
}


void	line_iLineVertical(v,l)
int32	v;
line_iLine	*l;
{
   	l->m = 1; l->n = 0; l->p = -v;
}

/*-----------------------------------------------------------------*/

/* creating a horizontal line  y = h */

void	line_iLineHorizontal(h,l)
int32	h;
line_iLine	*l;
{
   	l->m = 0; l->n = 1; l->p = -h;
}


void	line_dLineHorizontal(h,l)
double	h;
line_dLine	*l;
{
   	l->m = 0.0; l->n = 1.0; l->p = -h;
}

/*-----------------------------------------------------------------*/

/* creating a line from two points */

void	line_iLineFrom2Points(p1,p2,l)
pair_iXY	*p1;
pair_iXY	*p2;
line_iLine	*l;
{
   	l->m = (p2->y - p1->y);
   	l->n = (p1->x - p2->x);
    	l->p = (p1->y)*(p2->x) - (p2->y)*(p1->x);
}

void	line_dLineFrom2Points(p1,p2,l)
pair_dXY	*p1;
pair_dXY	*p2;
line_dLine	*l;
{
   	l->m = ((p2->y) - (p1->y));
   	l->n = ((p1->x) - (p2->x));
    	l->p = (p1->y)*(p2->x) - (p2->y)*(p1->x);
}

/*-----------------------------------------------------------------*/

/* creating a line from a point and a slope */

void	line_iLineFromPointAndSlope(pt,sl,l)
pair_iXY	*pt;
pair_iXY	*sl;
line_iLine	*l;
{
   	l->m = sl->y;
   	l->n = -(sl->x);
   	l->p = -( ((l->m)*(pt->x)) + ((l->n)*(pt->y)) );
}

void	line_dLineFromPointAndSlope(pt,sl,l)
pair_dXY	*pt;
pair_dXY	*sl;
line_dLine	*l;
{
   	l->m = sl->y;
   	l->n = -(sl->x);
   	l->p = -( ((l->m)*(pt->x)) + ((l->n)*(pt->y)) );
}

/*-----------------------------------------------------------------*/

/* to scale a line equation */

void	line_iLineScaling(r,l,f)
line_iLine	*r;
line_iLine	*l;
int32	f;
{
   	r->m = f*(l->m); r->n = f*(l->n); r->p = f*(l->p);
}

void	line_dLineScaling(r,l,f)
line_dLine	*r;
line_dLine	*l;
double	f;
{
   	r->m = f*(l->m); r->n = f*(l->n); r->p = f*(l->p);
}

/*-----------------------------------------------------------------*/
/* particular case of the above */

void	line_iLineFlip(l)
line_iLine	*l;
{
   	l->m = -(l->m); l->n = -(l->n); l->p = -(l->p);
}

void	line_dLineFlip(l)
line_dLine	*l;
{
   	l->m = -(l->m); l->n = -(l->n); l->p = -(l->p);
}

/*-----------------------------------------------------------------*/

/* to evaluate a line of the above form at a point */

int32	line_iLineEvaluation(l,pt)
line_iLine	*l;
pair_iXY	*pt;
{
    return( ((l->m) * (pt->x)) + ((l->n) * (pt->y)) + (l->p) );
}

double	line_dLineEvaluation(l,pt)
line_dLine	*l;
pair_dXY	*pt;
{
    return( ((l->m) * (pt->x)) + ((l->n) * (pt->y)) + (l->p) );
}

/*-----------------------------------------------------------------*/

/* line intersection */

void	line_iLineIntersection(l1,l2,pt)
line_iLine	*l1;
line_iLine	*l2;
pair_iXY	*pt;
{
   	int32   den;
   	den = ( ((l1->m) * (l2->n)) - ((l2->m) * (l1->n)) );
    if ( den == 0 )
  	 	pt->x = pt->y = 0;
   	else {
   	    pt->x = ( ((l1->n) * (l2->p)) - ((l2->n)*(l1->p)) )/den;
   	    pt->y = -( ((l1->m) * (l2->p)) - ((l2->m) * (l1->p)) )/den;
   	}
}

void	line_dLineIntersection(l1,l2,pt)
line_dLine	*l1;
line_dLine	*l2;
pair_dXY	*pt;
{
   	double   den;
   	den = ( ((l1->m) * (l2->n)) - ((l2->m) * (l1->n)) );
    if ( den == 0 )
  	 	pt->x = pt->y = 0;
   	else {
   	    pt->x = ( ((l1->n) * (l2->p)) - ((l2->n)*(l1->p)) )/den;
   	    pt->y = -( ((l1->m) * (l2->p)) - ((l2->m) * (l1->p)) )/den;
   	}
}

/*-----------------------------------------------------------------*/

/* manhattan distance between a point and a line */

float	line_iLineManhattanDistance(l,p)
line_iLine	*l;
pair_iXY	*p;
{	
	float	result;

	result = (math_FloatAbs(line_iLineEvaluation(l,p)) /
    			max(math_FloatAbs(l->m),math_FloatAbs(l->n)) );
	return(result);

}

double	line_dLineManhattanDistance(l,p)
line_dLine	*l;
pair_dXY	*p;
{	
	double	result;

	result = (math_FloatAbs(line_dLineEvaluation(l,p)) /
    			max(math_FloatAbs(l->m),math_FloatAbs(l->n)) );
	return(result);

}

/*-----------------------------------------------------------------*/

