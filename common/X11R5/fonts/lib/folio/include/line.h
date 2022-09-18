/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/line.h	1.1"
/*
 * @(#)line.h 1.2 89/03/10
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


/*-------------------------------------------------------------------
  	MANIPULATION OF STRAIGHT LINES
-------------------------------------------------------------------*/

/*
    Simple manipulations on lines of the form mx+ny+p
*/


/* the lines */

typedef	struct _line_fLine{
    float   m,n,p;
} line_fLine;


typedef	struct _line_iLine{
    int32   m,n,p;
} line_iLine;


typedef	struct _line_dLine{
    double   m,n,p;
} line_dLine;

/*-----------------------------------------------------------------*/

/* creating a vertical line x = v*/

extern	void	line_iLineVertical(/*v,l*/);
/* int32	v; */
/* line_iLine	*l; */


extern	void	line_dLineVertical(/*v,l*/);
/* double	v; */
/* line_dLine	*l; */

/*-----------------------------------------------------------------*/

/* creating a horizontal line  y = h */

extern	void	line_iLineHorizontal(/*h,l*/);
/*int32	h;*/
/*line_iLine	*l;*/


extern	void	line_dLineHorizontal(/*h,l*/);
/*double	h;*/
/*line_dLine	*l;*/

/*-----------------------------------------------------------------*/

/* creating a line from two points */

extern void	line_iLineFrom2Points(/*p1,p2,l*/);
/*  pair_iXY	*p1;	*/
/*  pair_iXY	*p2;	*/
/*  line_iLine	*l; 	*/

extern	void	line_dLineFrom2Points(/*p1,p2,l*/);
/*pair_dXY	*p1;*/
/*pair_dXY	*p2;*/
/*line_dLine	*l;*/

/*-----------------------------------------------------------------*/

/* creating a line from a point and a slope */

extern	void	line_iLineFromPointAndSlope(/*pt,sl,l*/);
/*pair_iXY	*pt;*/
/*pair_iXY	*sl;*/
/*line_iLine	*l;*/

extern	void	line_dLineFromPointAndSlope(/*pt,sl,l*/);
/*pair_dXY	*pt;*/
/*pair_dXY	*sl;*/
/*line_dLine	*l;*/

/*-----------------------------------------------------------------*/

/* to scale a line equation */

extern	void	line_iLineScaling(/*r,l,f*/);
/*line_iLine	*r;*/
/*line_iLine	*l;*/
/*int32	f;*/


extern	void	line_dLineScaling(/*r,l,f*/);
/*line_dLine	*r;*/
/*line_dLine	*l;*/
/*double	f;*/

/*-----------------------------------------------------------------*/
/* particular case of the above */

extern	void	line_iLineFlip(/*l*/);
/*line_iLine	*l;*/

extern	void	line_dLineFlip(/*l*/);
/*line_dLine	*l;*/

/*-----------------------------------------------------------------*/

/* to evaluate a line of the above form at a point */

extern	int32	line_iLineEvaluation(/*l,pt*/);
/*line_iLine	*l;*/
/*pair_iXY	*pt;*/

extern	double	line_dLineEvaluation(/*l,pt*/);
/*line_dLine	*l;*/
/*pair_dXY	*pt;*/

/*-----------------------------------------------------------------*/

/* line intersection */

extern	void	line_iLineIntersection(/*l1,l2,pt*/);
/*line_iLine	*l1;*/
/*line_iLine	*l2;*/
/*pair_iXY	*pt;*/

extern	void	line_dLineIntersection(/*l1,l2,pt*/);
/*line_dLine	*l1;*/
/*line_dLine	*l2;*/
/*pair_dXY	*pt;*/

/*-----------------------------------------------------------------*/

/* manhattan distance between a point and a line */

extern	float	line_iLineManhattanDistance(/*l,p*/);
/*line_iLine	*l;*/
/*pair_iXY	*p;*/

extern	double	line_dLineManhattanDistance(/*l,p*/);
/*line_dLine	*l;*/
/*pair_dXY	*p;*/

/*-----------------------------------------------------------------*/
