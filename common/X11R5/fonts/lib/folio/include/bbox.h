/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/bbox.h	1.1"
/*
 * @(#)bbox.h 1.2 89/03/10
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


/*---------------------------------------------------------------
    	BOUNDING BOXES
---------------------------------------------------------------*/

#define bbox_IsEmpty(b)     ((b->loy > b->hiy) && (b->lox > b->hix))

#define bbox_PointIsInBBox(b,p)  ( (p->x <= b->hix) && (p->x >= b->lox) && \
    	    	    	    	   (p->y <= b->hiy) && (p->y >= b->loy) )

/* bounding box <*b> of the rectangle defined by two points <*p1>,<*p2> */

#define	bbox_FillFrom2Points(b,p1,p2)									\
{																		\
    (b)->lox = min((p1)->x, (p2)->x);											\
    (b)->loy = min((p1)->y, (p2)->y);										\
    (b)->hix = max((p1)->x, (p2)->x);											\
    (b)->hiy = max((p1)->y, (p2)->y);											\
}


/*-------------------------------------------------------------*/

/* 
    filling in a bounding box <*b> given the lox <x>, loy <y>,
    x dimension <xsz> and y dimension <ysz>
*/

extern void	bbox_dFill(/*b, x, y, xsz, ysz*/);
/* bbox_dBBox  *b;  	    	*/
/* double	x, y, xsz, ysz;	*/


extern void	bbox_iFill(/*b,x,y,xsz,ysz*/);
/* bbox_iBBox  *b;   	    	*/
/* int32	x, y, xsz, ysz;	*/


/*-------------------------------------------------------------*/


/* 
    expanding a bounding box <*b> to include a point <*p>
*/

extern void	bbox_dExpandToIncludePoint(/*b, p*/);
/* bbox_dBBox  *b;  */
/* pair_dXY    *p;  */

extern void	bbox_iExpandToIncludePoint(/*b, p*/);
/* bbox_iBBox  *b;  */
/* pair_iXY    *p;  */


/*-------------------------------------------------------------*/

/* 
    displace bbox <*b1> by a pair XY <*v> to get new bbox <*b2> 
*/

extern void	bbox_dDisplace(/*b2, b1, v*/);
/* bbox_dBBox  *b2, *b1;    */
/* pair_dXY    *v;  	    */


extern void	bbox_iDisplace(/*b2, b1, v*/);
/* bbox_iBBox  *b2, *b1;    */
/* pair_iXY    *v;  	    */


/*-------------------------------------------------------------*/

/*
    scale bbox <*b1> by a scale factor <s> to get new bbox <*b2> 
*/
    
extern void	bbox_dScale(/*b2, b1, s*/);
/* bbox_dBBox  *b2, *b1;    */
/* double	s;  	    */

extern void	bbox_iScale(/*b2, b1, s*/);
/* bbox_iBBox  *b2, *b1;    */
/* int32	s;  	    */


/*-------------------------------------------------------------*/

/*
    enlarge/scale  bbox <*b1> by a value <d>  to get new bbox <*b2> 
*/

extern void	bbox_dEnlarge(/*b2, b1, d*/);
/* bbox_dBBox  *b2, *b1;    */
/* double	d;  	    */


extern void	bbox_iEnlarge(/*b2, b1, d*/);
/* bbox_iBBox  *b2, *b1;    */
/* int32	    d;	    */


extern void	bbox_dShrink(/*b2, b1, d*/);
/* bbox_dBBox  *b2, *b1;    */
/* double	d;  	    */


extern void	bbox_iShrink(/*b2, b1, d*/);
/* bbox_iBBox  *b2, *b1;    */
/* int32	d;  	    */


/*-------------------------------------------------------------*/

/*
    union of  bboxes <*b1> and <*b2> get new bbox <*b3> 
*/


extern void	bbox_dUnion(/*b3, b1, b2*/);
/* bbox_dBBox  *b3, *b2, *b1;	*/


extern void	bbox_iUnion(/*b3, b1, b2*/);
/* bbox_iBBox  *b3, *b2, *b1;	*/


/*-------------------------------------------------------------*/

/*
    intersection of  bboxes <*b1> and <*b2> get new bbox <*b3> 
*/


extern void	bbox_dIntersect(/*b3, b1, b2*/);
/* bbox_dBBox  *b3, *b2, *b1;	*/


extern void	bbox_iIntersect(/*b3, b1, b2*/);
/* bbox_iBBox  *b3, *b2, *b1;	*/


/*-------------------------------------------------------------*/

/*
    gridfitting of an double bbox <*fb> to integer bbox <*ib> 
*/

extern void	bbox_GridFit(/*ib, fb*/);
/* bbox_iBBox  *ib; */
/* bbox_dBBox  *fb; */


/*
    gridfitting of an fixed point bbox <*fixb> to integer bbox <*ib> 
    using <f> bits of fraction
*/

extern void	bbox_GridFitFixedPoint(/*ib, fixb, f*/);
/* bbox_iBBox  *ib, *fixb;  */
/* int32	    f;	    */

/*-------------------------------------------------------------*/

/* 
    transform a bounding box <*b1> to bbox <*b2> using
    a transformation <*t>
*/

extern void	bbox_Transform(/*b2, b1, t*/);
/* bbox_dBBox  	*b2, *b1;   */
/* trans_dTrans	*t; 	    */


extern void	bbox_TransformFloating(/*b2, b1, t*/);
/* bbox_dBBox  	*b2, *b1;   */
/* trans_dTrans	*t; 	    */


